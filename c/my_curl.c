// #include<stddef.h>
// #include<stdio.h>
// #include<string.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>


void __print_debug(struct addrinfo* fetched) {
    char ipstr[INET6_ADDRSTRLEN];

    for (struct addrinfo* p = fetched; p != NULL; p = p->ai_next) {
        void *addr;
        char *ipver;

        if (p->ai_family == AF_INET) { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        } else { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }

        inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
        printf("  %s: %s\n", ipver, ipstr);
    }
}

int connect_by_ip(const char* hostname) {
    // const char* hostname = "archlinux.org";

    struct addrinfo hints, *addrinfo_result;
    int status;
    
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

    int rv = getaddrinfo(hostname, "443", &hints, &addrinfo_result);
    if (rv != 0) {
        fprintf(stderr, "=== getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }
    
    // struct sockaddr_storage* result = (struct sockaddr_storage*)addrinfo_result->ai_addr;

    int sockfd = -1;
    for (struct addrinfo* p = addrinfo_result; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {
			perror("socket");
			continue;
		}

        int ok = connect(sockfd, p->ai_addr, p->ai_addrlen);
		if (ok == -1) {
			close(sockfd);
			perror("connect");
			continue;
		}
		break;
    }
    // __print_debug(addrinfo_result);
    freeaddrinfo(addrinfo_result);

    return sockfd;
}
