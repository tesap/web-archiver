#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "util.h"

#define SOCKET_PATH "/tmp/example.sock"
#define MAX_CLIENTS 1024
#define ALL_SCRIPT_PATH "src/all.sh"


int run_command(char **argv, char* out_file) {
    printf("run_command START: %s\n", argv[0]);
    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork");
        return -1;
    }
    
    if (pid == 0) {
        // Child process

        int fd = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("open");
            return -1;
        }
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);

        execvp(argv[0], argv);

        perror("execvp");
        exit(1);
    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        
        printf("run_command EXIT: %d\n", status);
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        return -1;
    }
}

int main() {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0); // Create UDS
    if (fd == -1) {
        perror("socket");
        return 1;
    }
    struct sockaddr_un loc;
    loc.sun_family = AF_UNIX;
    struct vec _v1 = (struct vec){loc.sun_path, 0};
    vec_append(&_v1, false, vec_wrap(SOCKET_PATH));
    vec_terminate(&_v1);

    unlink(loc.sun_path); // Remove old socket file if it exists
    if (bind(fd, (struct sockaddr*)&loc, sizeof(loc)) == -1) {
        perror("bind");
        return 1;
    }
    if (listen(fd, 5) == -1) {
        perror("listen");
        return 1;
    }

    // Client management
    int client_fds[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_fds[i] = -1;
    }

    fd_set readfds;
    int max_fd;

    printf("Server listening on %s\n", SOCKET_PATH);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        max_fd = fd;

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
        if (activity == -1) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        }

        // Check for new connections
        if (FD_ISSET(fd, &readfds)) {
            struct sockaddr_un remote;
            socklen_t len = sizeof(remote);
            int client_fd = accept(fd, (struct sockaddr*)&remote, &len);
            if (client_fd == -1) {
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
                close(client_fd);
            }
        }

        // Check for data from existing clients
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int client_fd = client_fds[i];
            if (client_fd != -1 && FD_ISSET(client_fd, &readfds)) {
                char buff[1024];
                int bytes_read = recv(client_fd, buff, sizeof(buff), 0);
                struct vec v1 = {buff, bytes_read};
                if (ends_with(v1, vec_wrap("\n"))) {
                    v1.size--;
                }
                vec_terminate(&v1);

                if (bytes_read <= 0) {
                    // Client disconnected or error
                    if (bytes_read == 0) {
                        printf("---- Client disconnected: %d (slot %d)\n", client_fd, i);
                    } else {
                        perror("recv");
                    }
                    close(client_fd);
                    client_fds[i] = -1;
                } else {
                    printf("--> data from client %d: '%.*s'\n", client_fd, bytes_read, buff);
                    char *args[] = {
                        "bash", ALL_SCRIPT_PATH, buff, NULL
                    };

                    char cwd[256];
                    if (getcwd(cwd, sizeof(cwd)) != NULL) {
                        printf("Current working dir: %s\n", cwd);
                    } else {
                        perror("getcwd() error");
                    }

                    time_t now = time(NULL);
                    struct tm *t = localtime(&now);

                    char timestamp[20];
                    // Format: YYYY-MM-DD_HH-MM-SS (e.g., 2024-05-20_14-30-05)
                    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H-%M-%S", t);

                    char filename[256];
                    sprintf(filename, "%s/all_%s.log", cwd, timestamp);

                    char send_buff[256];
                    sprintf(send_buff, "LOG FILE: %s\n", filename);
                    send(client_fd, send_buff, strlen(send_buff), 0);

                    // ====== RUN BASH SCRIPT ======
                    int status = run_command(args, filename);
                    // =============================

                    sprintf(send_buff, "RESULT: %d\n", status);
                    send(client_fd, send_buff, strlen(send_buff), 0);
                }
            }
        }
    }

    // Cleanup
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_fds[i] != -1) {
            close(client_fds[i]);
        }
    }
    close(fd);
    unlink(SOCKET_PATH);
    return 0;
}
