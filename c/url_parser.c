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

bool detect_is_file(const char* url, HrefType type_hint) {
    bool is_file = false;
    switch (type_hint) {
        case HREF_TYPE_HTML:
        case HREF_TYPE_UNKNOWN: 
            is_file = false;
            break;
        case HREF_TYPE_IMG:
        case HREF_TYPE_STYLE:
        case HREF_TYPE_SCRIPT:
            is_file = true;
            break;
    }

    // Find last '/' occurence
    int i = strlen(url);
    while (i > 0) {
        if (type_hint == HREF_TYPE_UNKNOWN && url[i] == '.' && is_alphabet(url[i + 1])) {
            is_file = true;
        }
        else if (url[i] == '/') {
            break;
        }
        i--;
    }

    return is_file;
}

void url_to_filepath(const char* url, bool is_file, char* out_path, int* out_dir_len) {
    struct UrlPtrs ptrs = get_url_pointers(url);
    const char* from = ptrs.host_start ? ptrs.host_start : url;
    const char* to = ptrs.path_end ? ptrs.path_end : (url + strlen(url));

    char url2[MAX_URL_LENGTH];
    int len = to - from;
    memcpy(url2, from, len);
    url2[len] = '\0';

    if (ends_with(url2, "/")) {
        is_file = false;
    }

    if (is_file) {
        // Find last '/' occurence
        int i = strlen(url2);
        while (i > 0) {
            if (url2[i] == '/') {
                i++;
                break;
            }
            i--;
        }

        strcpy(out_path, url2);
        *out_dir_len = i;
    } else {
        strip_end(url2, '/');
        *out_dir_len = strlen(url2) + 1;
        sprintf(out_path, "%s/index.html", url2);
    }
}
