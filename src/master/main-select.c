#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#include "util.h"

#define SOCKET_PATH "/tmp/example.sock"
#define MAX_CLIENTS 1024
#define MAX_CHILDREN 256
#define ALL_SCRIPT_PATH "src/all.sh"

// Structure to track child processes
typedef struct {
    pid_t pid;
    int client_fd;          // Which client this child belongs to
    char log_file[256];     // Log file path
    int completed;          // 0 = running, 1 = completed
    int exit_status;        // Exit status when completed
} child_tracker_t;

// Global variables for signal handling
volatile sig_atomic_t signal_received = 0;
volatile sig_atomic_t running = 1;
volatile sig_atomic_t child_exited = 0;  // Flag for SIGCHLD
int server_fd = -1;
int client_fds[MAX_CLIENTS];
child_tracker_t children[MAX_CHILDREN];
int sigchld_pipe[2] = {-1, -1};  // Pipe for SIGCHLD notifications

// Signal handler - only sets flags, does minimal work
static void signal_handler(int sig) {
    switch (sig) {
        case SIGHUP:
            signal_received = SIGHUP;
            break;
        case SIGTERM:
        case SIGINT:
            signal_received = sig;
            running = 0;
            break;
        case SIGCHLD:
            child_exited = 1;
            // Write to pipe to wake up select()
            if (sigchld_pipe[1] != -1) {
                char byte = 1;
                write(sigchld_pipe[1], &byte, 1);
            }
            break;
        default:
            break;
    }
}

// Cleanup function for graceful shutdown
static void cleanup_server(void) {
    printf("\nShutting down server...\n");
    
    // Terminate any running child processes
    for (int i = 0; i < MAX_CHILDREN; i++) {
        if (children[i].pid != 0 && !children[i].completed) {
            printf("Terminating child process: %d\n", children[i].pid);
            kill(children[i].pid, SIGTERM);
            // Give it a moment to die gracefully
            usleep(100000);
            kill(children[i].pid, SIGKILL);
            waitpid(children[i].pid, NULL, WNOHANG);
        }
    }
    
    // Close all client connections
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_fds[i] != -1) {
            printf("Closing client connection: %d\n", client_fds[i]);
            close(client_fds[i]);
            client_fds[i] = -1;
        }
    }
    
    // Close server socket
    if (server_fd != -1) {
        close(server_fd);
        server_fd = -1;
    }
    
    // Close signal pipe
    if (sigchld_pipe[0] != -1) close(sigchld_pipe[0]);
    if (sigchld_pipe[1] != -1) close(sigchld_pipe[1]);
    
    // Remove socket file
    unlink(SOCKET_PATH);
    printf("Server shutdown complete.\n");
}

// Reset all active sessions (SIGHUP handler)
static void reset_active_sessions(void) {
    printf("\nSIGHUP received - Resetting all active sessions...\n");
    
    // Terminate all running child processes
    for (int i = 0; i < MAX_CHILDREN; i++) {
        if (children[i].pid != 0 && !children[i].completed) {
            printf("Terminating child process: %d\n", children[i].pid);
            kill(children[i].pid, SIGTERM);
            waitpid(children[i].pid, NULL, WNOHANG);
            children[i].pid = 0;
            children[i].completed = 1;
        }
    }
    
    // Close all client connections
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_fds[i] != -1) {
            printf("Resetting client connection: %d (slot %d)\n", client_fds[i], i);
            
            // Send reset notification to client
            const char *reset_msg = "SERVER RESET: Connection terminated by server\n";
            send(client_fds[i], reset_msg, strlen(reset_msg), MSG_NOSIGNAL);
            
            // Close the connection
            close(client_fds[i]);
            client_fds[i] = -1;
        }
    }
    
    printf("All sessions reset. Server continues listening...\n");
}

// Find an empty slot in children array
static int find_empty_child_slot(void) {
    for (int i = 0; i < MAX_CHILDREN; i++) {
        if (children[i].pid == 0 || children[i].completed) {
            return i;
        }
    }
    return -1;
}

// Start a command asynchronously
static int start_command_async(char **argv, const char* out_file, int client_fd) {
    int slot = find_empty_child_slot();
    if (slot == -1) {
        fprintf(stderr, "Too many concurrent child processes\n");
        return -1;
    }
    
    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork");
        return -1;
    }
    
    if (pid == 0) {
        // Child process
        
        // Reset signal handlers for child (inherit default behavior)
        signal(SIGHUP, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        signal(SIGINT, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);

        // Redirect stdout and stderr to log file
        int fd = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("open");
            exit(1);
        }
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);

        execvp(argv[0], argv);
        perror("execvp");
        exit(1);
    } else {
        // Parent process - track the child
        children[slot].pid = pid;
        children[slot].client_fd = client_fd;
        strncpy(children[slot].log_file, out_file, sizeof(children[slot].log_file) - 1);
        children[slot].completed = 0;
        children[slot].exit_status = -1;
        
        printf("Started child process %d for client %d (slot %d)\n", pid, client_fd, slot);
        return 0;
    }
}

// Check for completed child processes
static void reap_children(void) {
    int status;
    pid_t pid;
    
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // Find this child in our tracking array
        for (int i = 0; i < MAX_CHILDREN; i++) {
            if (children[i].pid == pid) {
                children[i].completed = 1;
                if (WIFEXITED(status)) {
                    children[i].exit_status = WEXITSTATUS(status);
                } else if (WIFSIGNALED(status)) {
                    children[i].exit_status = -WTERMSIG(status);
                } else {
                    children[i].exit_status = -1;
                }
                
                printf("Child process %d completed with status %d (client %d)\n", 
                       pid, children[i].exit_status, children[i].client_fd);
                
                // Check if client is still connected
                int client_still_connected = 0;
                for (int j = 0; j < MAX_CLIENTS; j++) {
                    if (client_fds[j] == children[i].client_fd) {
                        client_still_connected = 1;
                        break;
                    }
                }
                
                if (client_still_connected) {
                    // Send result to client
                    char send_buff[512];
                    sprintf(send_buff, "RESULT: %d\nLOG FILE: %s\n", 
                            children[i].exit_status, children[i].log_file);
                    send(children[i].client_fd, send_buff, strlen(send_buff), MSG_NOSIGNAL);
                    
                    // Optional: Close connection after sending result
                    // (Remove this if you want to keep connection open for more commands)
                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (client_fds[j] == children[i].client_fd) {
                            close(client_fds[j]);
                            client_fds[j] = -1;
                            printf("Closed client connection %d after command completion\n", 
                                   children[i].client_fd);
                            break;
                        }
                    }
                } else {
                    printf("Client %d disconnected before child %d completed\n", 
                           children[i].client_fd, pid);
                }
                
                // Clear the slot
                children[i].pid = 0;
                break;
            }
        }
    }
    
    child_exited = 0;
}

int main() {
    struct sigaction sa;
    
    // Create pipe for SIGCHLD notifications
    if (pipe(sigchld_pipe) == -1) {
        perror("pipe");
        return 1;
    }
    
    // Set up signal handlers
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;  // Automatically restart interrupted system calls
    
    // Register signal handlers
    if (sigaction(SIGHUP, &sa, NULL) == -1) {
        perror("sigaction SIGHUP");
        return 1;
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction SIGTERM");
        return 1;
    }
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction SIGINT");
        return 1;
    }
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction SIGCHLD");
        return 1;
    }
    
    // Block SIGPIPE to prevent crashes when writing to closed sockets
    signal(SIGPIPE, SIG_IGN);
    
    printf("Signal handlers installed:\n");
    printf("  SIGHUP  - Reset active sessions\n");
    printf("  SIGTERM - Graceful shutdown\n");
    printf("  SIGINT  - Graceful shutdown (Ctrl+C)\n");
    printf("  SIGCHLD - Async child process tracking\n");
    printf("  SIGKILL - Cannot be caught (immediate termination)\n");
    printf("  SIGSTOP - Cannot be caught (immediate stop)\n\n");
    
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0); // Create UDS
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }
    
    struct sockaddr_un loc;
    loc.sun_family = AF_UNIX;
    struct vec _v1 = (struct vec){loc.sun_path, 0};
    vec_append(&_v1, false, vec_wrap(SOCKET_PATH));
    vec_terminate(&_v1);

    unlink(loc.sun_path); // Remove old socket file if it exists
    if (bind(server_fd, (struct sockaddr*)&loc, sizeof(loc)) == -1) {
        perror("bind");
        close(server_fd);
        return 1;
    }
    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        unlink(SOCKET_PATH);
        return 1;
    }

    // Initialize client management array
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_fds[i] = -1;
    }
    
    // Initialize child tracking array
    for (int i = 0; i < MAX_CHILDREN; i++) {
        children[i].pid = 0;
        children[i].completed = 0;
    }

    fd_set readfds;
    int max_fd;

    printf("Server listening on %s\n", SOCKET_PATH);
    printf("Send SIGHUP to PID %d to reset sessions\n", getpid());
    printf("Send SIGTERM to PID %d for graceful shutdown\n\n", getpid());

    while (running) {
        // Check for pending signals
        if (signal_received != 0) {
            if (signal_received == SIGHUP) {
                reset_active_sessions();
                signal_received = 0;
            } else if (signal_received == SIGTERM || signal_received == SIGINT) {
                printf("\nReceived termination signal. Shutting down...\n");
                break;
            }
        }
        
        // Check for completed children (non-blocking)
        if (child_exited) {
            reap_children();
        }
        
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        FD_SET(sigchld_pipe[0], &readfds);  // Add signal pipe to select
        max_fd = server_fd;
        
        if (sigchld_pipe[0] > max_fd) max_fd = sigchld_pipe[0];

        // Add all active client sockets to the set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_fds[i] != -1) {
                FD_SET(client_fds[i], &readfds);
                if (client_fds[i] > max_fd) {
                    max_fd = client_fds[i];
                }
            }
        }

        // Wait for activity on any socket
        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        
        // Check for signal during select()
        if (signal_received != 0) {
            if (signal_received == SIGHUP) {
                reset_active_sessions();
                signal_received = 0;
                continue;
            } else if (signal_received == SIGTERM || signal_received == SIGINT) {
                printf("\nReceived termination signal during select. Shutting down...\n");
                break;
            }
        }
        
        if (activity == -1) {
            if (errno == EINTR) {
                // Interrupted by signal - check signal flag and continue
                continue;
            }
            perror("select");
            break;
        }
        
        // Check SIGCHLD pipe (wake-up from signal handler)
        if (FD_ISSET(sigchld_pipe[0], &readfds)) {
            char byte;
            read(sigchld_pipe[0], &byte, 1);  // Clear the pipe
            reap_children();
        }

        // Check for new connections
        if (FD_ISSET(server_fd, &readfds)) {
            struct sockaddr_un remote;
            socklen_t len = sizeof(remote);
            int client_fd = accept(server_fd, (struct sockaddr*)&remote, &len);
            if (client_fd == -1) {
                if (errno == EINTR) {
                    continue;  // Interrupted by signal
                }
                perror("accept");
                continue;
            }

            // Add new client to the array
            int added = 0;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_fds[i] == -1) {
                    client_fds[i] = client_fd;
                    added = 1;
                    printf("---- New client: %d (slot %d)\n", client_fd, i);
                    break;
                }
            }

            if (!added) {
                printf("---- Maximum clients reached, rejecting: %d\n", client_fd);
                const char *msg = "Server full, try again later\n";
                send(client_fd, msg, strlen(msg), MSG_NOSIGNAL);
                close(client_fd);
            }
        }

        // Check for data from existing clients
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int client_fd = client_fds[i];
            if (client_fd != -1 && FD_ISSET(client_fd, &readfds)) {
                char buff[1024];
                int bytes_read = recv(client_fd, buff, sizeof(buff) - 1, 0);  // Leave room for null
                
                if (bytes_read <= 0) {
                    // Client disconnected or error
                    if (bytes_read == 0) {
                        printf("---- Client disconnected: %d (slot %d)\n", client_fd, i);
                    } else {
                        if (errno != EINTR) {
                            perror("recv");
                        }
                    }
                    close(client_fd);
                    client_fds[i] = -1;
                } else {
                    // Properly null-terminate the buffer
                    buff[bytes_read] = '\0';
                    
                    // Remove trailing newline
                    char *newline = strchr(buff, '\n');
                    if (newline) *newline = '\0';
                    
                    printf("--> data from client %d: '%s'\n", client_fd, buff);
                    char *args[] = {
                        "bash", ALL_SCRIPT_PATH, buff, NULL
                    };

                    char cwd[256];
                    if (getcwd(cwd, sizeof(cwd)) != NULL) {
                        printf("Current working dir: %s\n", cwd);
                    } else {
                        perror("getcwd() error");
                        strcpy(cwd, ".");
                    }

                    time_t now = time(NULL);
                    struct tm *t = localtime(&now);

                    char timestamp[20];
                    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H-%M-%S", t);

                    char filename[256];
                    sprintf(filename, "%s/all_%s_%d.log", cwd, timestamp, client_fd);

                    char send_buff[256];
                    sprintf(send_buff, "COMMAND STARTED\nLOG FILE: %s\n", filename);
                    
                    // Use MSG_NOSIGNAL to prevent SIGPIPE
                    send(client_fd, send_buff, strlen(send_buff), MSG_NOSIGNAL);

                    // ====== RUN BASH SCRIPT ======
                    int result = start_command_async(args, filename, client_fd);
                    // =============================
                    
                    if (result == -1) {
                        sprintf(send_buff, "ERROR: Failed to start command (max processes reached)\n");
                        send(client_fd, send_buff, strlen(send_buff), MSG_NOSIGNAL);
                    } else {
                        // Send confirmation that process started
                        sprintf(send_buff, "STATUS: Running (PID monitoring active)\n");
                        send(client_fd, send_buff, strlen(send_buff), MSG_NOSIGNAL);
                        
                        // Note: We keep the connection open! Result will be sent when child completes.
                        // If you want to accept more commands on the same connection, don't close it.
                    }
                }
            }
        }
    }

    // Graceful cleanup
    cleanup_server();
    return 0;
}
