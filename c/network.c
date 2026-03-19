
#include "./network.h"
#include "./util.h"
#include "./url_parser.h"
#include "./http_parser.h"

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <assert.h>

#define BUFFER_SIZE 64

void print_sockaddr(struct sockaddr* sa) {
    char ip_string[INET6_ADDRSTRLEN];
    void *addr_ptr;
    in_port_t port;

    if (sa->sa_family == AF_INET) {
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)sa;
        addr_ptr = &(ipv4->sin_addr);
        port = ntohs(ipv4->sin_port);

        if (inet_ntop(AF_INET, addr_ptr, ip_string, INET6_ADDRSTRLEN) == NULL) {
            perror("inet_ntop");
            return;
        }
        printf("IPv4 Address: %s, Port: %d\n", ip_string, port);

    } else if (sa->sa_family == AF_INET6) {
        struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)sa;
        addr_ptr = &(ipv6->sin6_addr);
        port = ntohs(ipv6->sin6_port);

        if (inet_ntop(AF_INET6, addr_ptr, ip_string, INET6_ADDRSTRLEN) == NULL) {
            perror("inet_ntop");
            return;
        }
        printf("IPv6 Address: %s, Port: %d\n", ip_string, port);

    } else {
        printf("Unknown address family: %d\n", sa->sa_family);
    }
}

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
        // print_sockaddr(p->ai_addr);

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
    parse_url_parts(url, &url_parts);

    char request[1024];

    assert(url_parts.path != NULL);
    assert(url_parts.host != NULL);
    assert(strlen(url_parts.host) > 0);

    char* path;
    if (strlen(url_parts.path) > 0) {
        path = url_parts.path;
    } else {
        path = "/";
    }
    // Mimic to cURL
    const char* request_template = ""
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: curl/8.18.0\r\n"
        "Connection: close\r\n\r\n";
    snprintf(request, sizeof(request), request_template, path, url_parts.host);
    // printf("REQUEST: %s\n", request);
    // fflush(stdout);

    int sockfd;
    char recv_buff[BUFFER_SIZE];
    int bytes_received;
    bool content_started = false;
    struct vec* headers_vec = vec_init(0);
    struct vec* content_vec = vec_init(0);
        
    if (strcmp(url_parts.protocol, "http://") == 0 || strlen(url_parts.protocol) == 0) {
        sockfd = create_tcp_socket(url_parts.host, "80");

        // --- Send request ---
        int bytes_sent = send(sockfd, request, strlen(request), 0);

        // --- Receive response ---
        while ((bytes_received = recv(sockfd, recv_buff, BUFFER_SIZE - 1, 0)) > 0) {
            parse_http_stream_chunk(recv_buff, bytes_received, headers_vec, content_vec, &content_started);
        }
    } else if (strcmp(url_parts.protocol, "https://") == 0) {
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
            parse_http_stream_chunk(recv_buff, bytes_received, headers_vec, content_vec, &content_started);
        }
    } else {
        fprintf(stderr, "=== Protocol not supported: %s\n", url);
        return -1;
    }

    if (bytes_received < 0) {
        fprintf(stderr, "=== Error getting response: %s\n", url);
        return -1;
    }

    // --- Parsing HTTP response ---
    int status_code = 0;
    sscanf(headers_vec->ptr, "HTTP/%*d.%*d %d", &status_code);
    if (status_code == 0) {
        fprintf(stderr, "=== Error parsing status_code: %.*s...\n", 20, headers_vec->ptr);
        return -1;
    } else if (status_code == 301 || status_code == 302) {
        char redirect_url[256];
        vec_append(headers_vec, "\0", 1);
        if (get_location_header(headers_vec->ptr, url, redirect_url) != 0) {
            return -1;
        }
        // printf("--- FOUND REDIRECT Location: %s\n", redirect_url);
        return download_http(redirect_url, timeout_sec, out);
    } else if (status_code == 200) {
        out->headers_vec = headers_vec;
        out->content_vec = content_vec;

        int len = strlen(url);
        memcpy(out->effective_url, url, len);
        out->effective_url[len] = '\0';

        return 0;
    } else {
        fprintf(stderr, "RESPONSE: %.*s\n", headers_vec->size, headers_vec->ptr);
        fprintf(stderr, "=== Bad status_code: %d\n", status_code);
        write_file("err.http", headers_vec->ptr, headers_vec->size);
        return -1;
    }
}

