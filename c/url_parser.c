#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "./util.h"

struct UrlParts {
    char protocol[16];
    char host[256];
    char path[1024];
    // TODO: int port;
};

bool is_url_relative(char* url) {
    /*
     * Tells whether a URL refers relative path.
     * which means it is either refers relative path or an http/https URL.
     */
    return strncmp(url, "/", 1) == 0;
}

bool is_url_http(char* url) {
    /*
     * Tells whether a URL is http/https.
     */
    return strncmp(url, "http", 4) == 0;
}

void parse_url(const char* url, struct UrlParts* parts) {
    const char* url_ptr = url;
    const char* url_end = url + strlen_with_delims(url); // TODO maybe -1

    memcpy(parts->protocol, "\0", 1);
    memcpy(parts->host, "\0", 1);
    memcpy(parts->path, "\0", 1);

    const char *protocol_end = strstr_with_delims(url_ptr, "://");
    if (protocol_end) {
        int len = protocol_end - url_ptr;
        memcpy(parts->protocol, url_ptr, len);
        parts->protocol[len] = '\0';
        url_ptr = protocol_end + 3;
    }

    const char *host_end = strchr_with_delims(url_ptr, '/');
    if (!host_end) {
        host_end = strchr_with_delims(url_ptr, '#');
        if (!host_end) {
            host_end = strchr_with_delims(url_ptr, '?');
            if (!host_end) {
                host_end = url_end;
            }
        }
    }

    int len = host_end - url_ptr;
    memcpy(parts->host, url_ptr, len);
    parts->host[len] = '\0';
    url_ptr = host_end;

    const char *path_end = strchr_with_delims(url_ptr, '#');
    if (!path_end) {
        path_end = strchr_with_delims(url_ptr, '?');
        if (!path_end) {
            path_end = url_end;
        }
    }

    len = path_end - url_ptr;
    memcpy(parts->path, url_ptr, len);
    parts->path[len] = '\0';
}
