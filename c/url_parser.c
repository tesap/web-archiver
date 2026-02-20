#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "./url_parser.h"
#include "./util.h"

bool is_url_relative(const char* url) {
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

struct UrlPtrs get_url_pointers(const char* url) {
    struct UrlPtrs res = { NULL, NULL, NULL, NULL, NULL, NULL };

    // TODO Better parsing and handling of corner cases
    const char* i = url;
    for (i = url; *i != '\0' && (strchr("$?#\n\r ", *i) == 0); i++) {
        if (strncmp(i, "://", 3) == 0) {
            res.protocol_start = url;
            res.protocol_end = i;
            i += 3;
            res.host_start = i;
        // Have not been encountered '/' before
        } else if (*i == '/' && !res.path_start) {
            if (!res.host_start && i > url) {
                res.host_start = url;
            }
            if (res.host_start) {
                res.host_end = i;
            }
            res.path_start = i;
        }
    }
    if (res.host_start && !res.host_end) {
        res.host_end = i;
    } else if (res.path_start) {
        res.path_end = i;
    }

    if (!res.protocol_start && !res.path_start) {
        res.host_start = url;
        res.host_end = i;
    }

    return res;
}


void parse_url(const char* url, struct UrlParts* parts) {
    struct UrlPtrs ptrs = get_url_pointers(url);

    parts->protocol[0] = '\0';
    parts->host[0] = '\0';
    parts->path[0] = '\0';

    if (ptrs.protocol_start && ptrs.protocol_end) {
        int len = ptrs.protocol_end - ptrs.protocol_start;
        memcpy(parts->protocol, ptrs.protocol_start, len);
        parts->protocol[len] = '\0';
    }

    if (ptrs.host_start && ptrs.host_end) {
        int len = ptrs.host_end - ptrs.host_start;
        memcpy(parts->host, ptrs.host_start, len);
        parts->host[len] = '\0';
    }

    if (ptrs.path_start != NULL && ptrs.path_end != NULL) {
        int len = ptrs.path_end - ptrs.path_start;
        memcpy(parts->path, ptrs.path_start, len);
        parts->path[len] = '\0';
    }
}
