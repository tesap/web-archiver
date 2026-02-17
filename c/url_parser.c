#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

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
    char* url_ptr = url;
    char* url_end = url + strlen(url); // TODO maybe -1

    memcpy(parts->protocol, "\0", 1);
    memcpy(parts->host, "\0", 1);
    memcpy(parts->path, "\0", 1);

    char *protocol_end = strstr(url_ptr, "://");
    if (protocol_end) {
        int len = protocol_end - url_ptr;
        memcpy(parts->protocol, url_ptr, len);
        parts->protocol[len] = '\0';
        url_ptr = protocol_end + 3;
    }

    char *host_end = strchr(url_ptr, '/');
    if (!host_end) {
        host_end = strchr(url_ptr, '#');
        if (!host_end) {
            host_end = strchr(url_ptr, '?');
            if (!host_end) {
                host_end = url_end;
            }
        }
    }

    int len = host_end - url_ptr;
    memcpy(parts->host, url_ptr, len);
    parts->host[len] = '\0';
    url_ptr = host_end;

    char *path_end = strchr(url_ptr, '#');
    if (!path_end) {
        path_end = strchr(url_ptr, '?');
        if (!path_end) {
            path_end = url_end;
        }
    }

    len = path_end - url_ptr;
    memcpy(parts->path, url_ptr, len);
    parts->path[len] = '\0';
}
