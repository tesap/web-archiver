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

bool is_url_http(const char* url) {
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
            i += 3;
            res.protocol_end = i;
            res.host_start = i;
        } else if (*i == ':') {
            res.host_end = i;
        // Have not been encountered '/' before
        } else if (*i == '/' && !res.path_start) {
            if (!res.host_start && i > url) {
                res.host_start = url;
            }
            if (!res.host_end && res.host_start) {
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


void parse_url_parts(const char* url, struct UrlParts* parts) {
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


void parse_url_paths(const char* url, struct UrlPaths* out) {
    struct UrlParts url_p;
    parse_url_parts(url, &url_p);

    // Strip '/' at end
    char* url_path = url_p.path;
    if (url_path[strlen(url_path) - 1] == '/') {
        url_path[strlen(url_path) - 1] = '\0';
    }

    const int url_len = strlen(url);
    const int url_path_len = strlen(url_path);

    char stripped_url[url_len];
    sprintf(stripped_url, "%s%s", url_p.host, url_path);

    bool is_file = false;
    // Find last '/' occurence
    int i = url_path_len;
    while (i > 0) {
        if (url_path[i] == '.') {
            is_file = true;
        }
        else if (url_path[i] == '/') {
            break;
        }
        i--;
    }

    if (is_file) {
        int filename_len = url_path_len - i;
        int dir_len = strlen(stripped_url) - filename_len;

        // Fill dir_path
        memcpy(out->dir_path, stripped_url, dir_len);
        out->dir_path[dir_len] = '\0';
        // Fill file_path
        strcpy(out->file_path, stripped_url);
    } else {
        // Fill dir_path
        strcpy(out->dir_path, stripped_url);
        // Fill file_path
        sprintf(out->file_path, "%s/index.html", stripped_url);
    }
}
