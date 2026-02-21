
#include "./network.h"
#include "./util.h"
#include "./url_parser.h"
#include "./http_parser.h"

#include <openssl/ssl.h>
#include <openssl/err.h>

#define BUFFER_SIZE 64

int create_tcp_socket(const char* hostname, const char* service) {
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
    freeaddrinfo(addrinfo_result);

    return sockfd;
}

int download_http(const char* url, int timeout_sec, struct HttpPage* out) {
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
        sockfd = create_tcp_socket(url_parts.host, "80");

        // --- Send request ---
        int bytes_sent = send(sockfd, request, strlen(request), 0);

        // --- Receive response ---
        while ((bytes_received = recv(sockfd, recv_buff, BUFFER_SIZE - 1, 0)) > 0) {
            vec_append(tcp_data_vec, recv_buff, bytes_received);
        }
        vec_append(tcp_data_vec, "\0", 1);
    } else if (strcmp(url_parts.protocol, "https") == 0) {
        sockfd = create_tcp_socket(url_parts.host, "443");

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
        struct timeval tv;
        tv.tv_sec = timeout_sec;
        tv.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

        while ((bytes_received = SSL_read(ssl, recv_buff, BUFFER_SIZE - 1)) > 0) {
            vec_append(tcp_data_vec, recv_buff, bytes_received);
        }
        vec_append(tcp_data_vec, "\0", 1);
    } else {
        fprintf(stderr, "=== Protocol not supported: %s\n", url);
        return -1;
    }

    if (bytes_received < 0) {
        fprintf(stderr, "=== Error getting response: %s\n", url);
        return -1;
    }

    const char* tcp_data = tcp_data_vec->ptr;

    // --- Parsing HTTP response ---
    int status_code = 0;
    sscanf(tcp_data, "HTTP/%*d.%*d %d", &status_code);
    if (status_code == 0) {
        fprintf(stderr, "=== Error parsing status_code: %.*s...\n", 20, tcp_data);
        return -1;
    } else if (status_code == 301) {
        char redirect_url[256];
        if (get_location_header(tcp_data, url, redirect_url) != 0) {
            return -1;
        }
        // printf("--- FOUND REDIRECT Location: %s\n", redirect_url);
        return download_http(redirect_url, timeout_sec, out);
    } else if (status_code == 200) {
        // char filename[256];
        // sprintf(filename, "%s22.http", url_parts.host);
        // write_file(filename, tcp_data, tcp_data_vec->size);
        out->data_vec = tcp_data_vec;
        out->content_offset = get_content_start(tcp_data) - tcp_data;
        int len = strlen(url);
        memcpy(out->effective_url, url, len);
        out->effective_url[len] = '\0';
        return 0;
    } else {
        fprintf(stderr, "=== Bad status_code: %d\n", status_code);
        return -1;
    }
}

