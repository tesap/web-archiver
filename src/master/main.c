
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>

#include "util.h"

#define SOCKET_PATH "/tmp/example.sock"

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
    // strncpy(loc.sun_path, SOCKET_PATH, sizeof(loc.sun_path)-1);
    // loc.sun_path = SOCKET_PATH;

    unlink(loc.sun_path); // Remove old socket file if it exists
    bind(fd, (struct sockaddr*)&loc, sizeof(loc));
    if (listen(fd, 5) == -1) {
        perror("listen");
        return 1;
    }

    while (true) {
        struct sockaddr_un remote;
        int len = sizeof(remote);
        int client_fd = accept(fd, (struct sockaddr*) &remote, &len); // Wait for client
        if (client_fd == -1) {
            perror("accept");
            return 1;
        }
        printf("---- New client: %d\n", client_fd);

        int bytes_read;
        char buff[1024];
        while ((bytes_read = recv(client_fd, &buff, 1024, 0)) > 0) {
            printf("--> data: '%.*s'\n", bytes_read, buff);
        }

        close(client_fd);
        printf("---- EXIT client: %d\n", client_fd);
    }

    close(fd);
    return 0;
}

