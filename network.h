#ifndef NETWORK_H
#define NETWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

struct HttpPage {
    char effective_url[256];
    struct vec* headers_vec;
    struct vec* content_vec;
};

int create_tcp_socket(const char* hostname, const char* service);
int download_http(const char* url, int timeout_sec, struct HttpPage* res_page);

#endif
