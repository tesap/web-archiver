
#include "./network.h"
#include "./util.h"
#include "./url_parser.h"

#include <openssl/ssl.h>
#include <openssl/err.h>

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

const char* find_content_start(const char* http_response) {
    // TODO Better handling of corner cases
    return strstr(http_response, "\r\n\r\n") + 4;
}

const char* find_redirect_location(const char* http_response) {
}

int create_socket(const char* hostname, const char* service) {
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

int download_http(const char* url, struct HttpPage* out) {
    printf("--- URL: %s\n", url);
    struct UrlParts url_parts;
    parse_url(url, &url_parts);

    char request[1024];

    snprintf(request, sizeof(request), "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", url_parts.path, url_parts.host);
    // printf("REQUEST: %s\n", request);
    fflush(stdout);

    int sockfd;
    char recv_buff[BUFFER_SIZE];
    int bytes_received;
    struct vec* tcp_data_vec = vec_init(0);
        
    if (strcmp(url_parts.protocol, "http") == 0 || strlen(url_parts.protocol) == 0) {
        sockfd = create_socket(url_parts.host, "80");

        // --- Send request ---
        int bytes_sent = send(sockfd, request, strlen(request), 0);

        // --- Receive response ---
        while ((bytes_received = recv(sockfd, recv_buff, BUFFER_SIZE - 1, 0)) > 0) {
            vec_append(tcp_data_vec, recv_buff, bytes_received);
        }
        vec_append(tcp_data_vec, "\0", 1);
    } else if (strcmp(url_parts.protocol, "https") == 0) {
        sockfd = create_socket(url_parts.host, "443");

        SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
        if (!ctx) {
            fprintf(stderr, "SSL_CTX_new() failed.\n");
            return -1;
        }
        SSL* ssl = SSL_new(ctx);
        if (!ssl)
        {
            fprintf(stderr, "SSL_new() failed.\n");
            close(sockfd);
            SSL_CTX_free(ctx);
            return -1;
        }

        SSL_set_fd(ssl, sockfd);

        // IMPORTANT: Set SNI hostname
        SSL_set_tlsext_host_name(ssl, url_parts.host);

        if (SSL_connect(ssl) <= 0)
        {
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            close(sockfd);
            SSL_CTX_free(ctx);
            return -1;
        }

        // --- Send request ---
        SSL_write(ssl, request, strlen(request));

        // --- Receive response ---
        while ((bytes_received = SSL_read(ssl, recv_buff, BUFFER_SIZE - 1)) > 0) {
            vec_append(tcp_data_vec, recv_buff, bytes_received);
        }
        vec_append(tcp_data_vec, "\0", 1);
    } else {
        fprintf(stderr, "=== Protocol not supported: %s\n", url);
        exit(1);
    }

    // TODO handle this possibility?
    // if (sockfd == -1) {
    //     fprintf(stderr, "=== Socket connection error: %s\n", url);
    //     exit(1);
    // }

    const char* tcp_data = tcp_data_vec->ptr;

    // --- Parsing HTTP response ---
    int status_code = 0;
    sscanf(tcp_data, "HTTP/%*d.%*d %d", &status_code);
    printf("--- \tstatus_code: %d\n", status_code);
    if (status_code == 0) {
        fprintf(stderr, "=== Error parsing status_code: %.*s...\n", 20, tcp_data);
    } else if (status_code == 301) {
        printf("|%s|\n", tcp_data);
        const char* location_substr = strstr(tcp_data, "Location: ");
        if (!location_substr) {
            fprintf(stderr, "=== Error searching for 'Location: ' pattern\n");
        } else {
            // TODO move to function + tests
            const char* location_url_start = location_substr + 10;
            size_t len = strlen_with_delims(location_url_start);
            char redirect_url[256];

            memcpy(redirect_url, location_url_start, len);
            redirect_url[len] = '\0';
            printf("--- FOUND REDIRECT Location: %s\n", redirect_url);

            return download_http(redirect_url, out);
        }
    } else if (status_code == 200) {
        out->data_vec = tcp_data_vec;
        out->content_offset = tcp_data - find_content_start(tcp_data);
        memcpy(out->effective_url, url, strlen(url));
        return 0;
    } else if (status_code == 400) {
        // printf("Response: %s\n", tcp_data);
    }
}

