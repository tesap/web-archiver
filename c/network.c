
#include "./network.h"
#include "./util.h"
#include "./url_parser.h"

#define BUFFER_SIZE 64

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

char* find_http_content_start(char* data) {
    return strstr(data, "\r\n\r\n") + 4;
}

int create_socket(const char* hostname, const char* service) {
    printf("=== hostname: %s\n", hostname);
    struct addrinfo hints, *addrinfo_result;
    int status;
    
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

    int rv = getaddrinfo(hostname, service, &hints, &addrinfo_result);
    if (rv != 0) {
        fprintf(stderr, "=== getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

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
            sockfd = -1;
			perror("connect");
			continue;
		}
		break;
    }
    // __print_debug(addrinfo_result);
    freeaddrinfo(addrinfo_result);

    return sockfd;
}

int download_http(const char* url, struct HttpPage* res_page) {
    printf("==== URL: %s\n", url);
    struct UrlParts url_parts;
    parse_url(url, &url_parts);

    char request[1024];
    snprintf(request, sizeof(request), "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", url_parts.path, url_parts.host);
    // printf("=== REQ: %s\n", request);

    int sockfd;
    // if (strcmp(url_parts.protocol, "https") == 0) {
    //     fprintf(stderr, "==== https download not supported\n");
    //     exit(1);
    if (strcmp(url_parts.protocol, "http") == 0 || strlen(url_parts.protocol) == 0) {
        sockfd = create_socket(url_parts.host, "80");
    } else {
        // TODO support https
        fprintf(stderr, "==== download not supported: %s\n", url);
        exit(1);
    }

    if (sockfd == -1) {
        fprintf(stderr, "==== Http is actually not supported: %s\n", url);
        exit(1);
    }

    int bytes_sent = send(sockfd, request, strlen(request), 0);
    printf("==== bytes_sent: %d\n", bytes_sent);
    char recv_buff[BUFFER_SIZE];
    int bytes_received;
    struct vec* tcp_data_vec = vec_init(0);
    
    while ((bytes_received = recv(sockfd, recv_buff, BUFFER_SIZE - 1, 0)) > 0) {
        vec_append(tcp_data_vec, recv_buff, bytes_received);
        printf("==== bytes_received: %d\n", bytes_received);
    }
    vec_append(tcp_data_vec, "\0", 1);

    const char* tcp_data = tcp_data_vec->ptr;

    int status_code = 0;
    sscanf(tcp_data, "HTTP/%*d.%*d %d", &status_code);
    printf("==== status_code: %d\n", status_code);
    if (status_code == 0) {
        fprintf(stderr, "=== Error parsing status_code: %.*s...\n", 20, tcp_data);
    } else if (status_code == 301) {
        char* location_substr = strstr(tcp_data, "Location: ");
        if (!location_substr) {
            fprintf(stderr, "=== Error searching for 'Location: ' pattern\n");
        }
        char* location_url_end = strstr(location_substr, "\n");
        char* location_url_start = location_substr + 10;
        char redirect_url[64];
        int len = location_url_end - location_url_start;

        memcpy(redirect_url, location_url_start, len);
        redirect_url[len] = '\0';
        printf("--- FOUND REDIRECT Location: %s\n", redirect_url);

        download_http(redirect_url, res_page);
    } else if (status_code == 200) {
        // res_page->data = content_vec->ptr;
        res_page->data_vec = tcp_data_vec;
        // TODO: res_page->content_start = ...
    }

    // TODO cleanup
    // } else {
    //     SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    //     if (!ctx) {
    //         fprintf(stderr, "SSL_CTX_new() failed.\n");
    //         return;
    //     }
    //     SSL* ssl = SSL_new(ctx);
    //     if (!ssl)
    //     {
    //         fprintf(stderr, "SSL_new() failed.\n");
    //         close(sockfd);
    //         SSL_CTX_free(ctx);
    //         return;
    //     }
    // }
}

