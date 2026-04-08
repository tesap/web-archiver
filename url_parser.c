#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <limits.h>

#include "./url_parser.h"
#include "./util.h"

#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

bool is_url_relative(const char* url_start) {
    /*
     * Tells whether a URL refers relative path.
     * which means it is either refers relative path or an http/https URL.
     */
    return url_start[0] == '/';
}

bool is_url_http(const char* url_start) {
    /*
     * Tells whether URL is http/https.
     */
    return strncmp(url_start, "http", 4) == 0;
}

struct UrlPtrs get_url_pointers(struct vec url) {
    struct UrlPtrs res = { NULL, NULL, NULL, NULL, NULL, NULL };

    assert(url.ptr != NULL);
    assert(url.size > 0);

    bool host_found = false;

    // TODO Better parsing and handling of corner cases
    const char* i = url.ptr;
    for (i = url.ptr; i < url.ptr + url.size && (strchr("$?#\n\r ", *i) == NULL); i++) {
        if (!host_found && strncmp(i, "://", 3) == 0) {
            res.protocol_start = url.ptr;
            i += 3;
            res.protocol_end = i;
            res.host_start = i;
            host_found = true;
        } else if (*i == ':') {
            res.host_end = i;
        // Have not encountered '/' before
        } else if (*i == '/' && !res.path_start) {
            if (!res.host_start && i > url.ptr) {
                res.host_start = url.ptr;
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
        res.host_start = url.ptr;
        res.host_end = i;
    }

    return res;
}


void parse_url_parts(const char* url, struct UrlParts* parts) {
    struct vec v = { (char*)url, strlen(url) };
    struct UrlPtrs ptrs = get_url_pointers(v);

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

bool detect_is_file(struct vec url, LinkType type_hint) {
    bool is_file = false;
    switch (type_hint) {
        case LINK_TYPE_HTML:
        case LINK_TYPE_UNKNOWN: 
            is_file = false;
            break;
        case LINK_TYPE_IMG:
        case LINK_TYPE_STYLE:
        case LINK_TYPE_SCRIPT:
            is_file = true;
            break;
    }

    // Find last '/' occurence
    int i = url.size;
    while (i > 0) {
        if (type_hint == LINK_TYPE_UNKNOWN && url.ptr[i] == '.' && is_alphabet(url.ptr[i + 1])) {
            is_file = true;
        }
        else if (url.ptr[i] == '/') {
            break;
        }
        i--;
    }

    return is_file;
}

void url_to_filepath(struct vec url, bool is_file, struct vec* out) {
    /*
     * Given URL as a general string (@url.ptr, @url.size),
     * parse it and return relevant path (@out_path) to which a page at given URL could be saved.
     * @is_file flag is used to control whether treat 
     * 
     * For example:
     * INPUT:
     *     - url = "https://www.archlinux.org/path/to/smth/"
     *     - is_file = false
     * OUTPUT:
     *     - out_path = "www.archlinux.org/path/to/smth/index.html"
     *
     * INPUT:
     *     - url = "https://www.archlinux.org/path/to/smth/"
     *     - is_file = true
     * OUTPUT:
     *     - out_path = "www.archlinux.org/path/to/smth"
     */
    struct UrlPtrs ptrs = get_url_pointers(url);
    const char* from = ptrs.host_start ? ptrs.host_start : url.ptr;
    const char* to = ptrs.path_end ? ptrs.path_end : (url.ptr + url.size);

    char url2[MAX_URL_LENGTH];
    int len = to - from;
    memcpy(url2, from, len);
    url2[len] = '\0';

    /* Test implementation that "just works" because it removes all nesting and returns unique filename */
    // for (int i = 0; i < len; i++) {
    //     if (url2[i] == '/') {
    //         url2[i] = '\\';
    //     }
    // }
    //
    // strcpy(out_path, url2);

    strip_end(url2, '/');

    // TODO make vec_append without *
    vec_append(out, false, vec_wrap(url2));
    
    if (!is_file) {
        vec_append(out, false, vec_wrap("/index.html"));
    }
}

int get_dir_count(struct vec path) {
        int i = 1;
        int cnt = 0;
        assert(path.ptr[0] != '/');
        // while (path.ptr[i] != '\0') {
        while (i < path.size) {
            if (strncmp(path.ptr + i - 2, "/./", 3) == 0) {
            }
            else if (i == 1 && strncmp(path.ptr, "./", 2) == 0) {
            }
            else if (strncmp(path.ptr + i - 2, "../", 3) == 0) {
                cnt--;
            }
            else if (path.ptr[i] == '/') {
                cnt++;
            }
            i++;
        }
        if (path.ptr[path.size - 1] != '/') {
            cnt++;
        }
        return cnt;
}

void get_relpath(
    struct vec path_from,
    struct vec path_to,
    struct vec* result
) {
    /*
     * Calculate relative path from @path_from to @path_to.
     * Both @path_from and @path_to are treated as directories.
     *
     * For example:
     * IN:
     *     path_from = dir1/dir2/one
     *     path_to   = dir1/dir2/second
     * OUT:
     *     res = ../second
     */

    int i = 0;
    int last_saved_slash = 0;

    // while (true) {
    //     if (path_from.ptr[i] != path_to.ptr[i]) {
    //         if (path_from.ptr[i] == '/' || path_to.ptr[i] == '/') {
    //             last_saved_slash = i + 1;
    //         }
    //         break;
    //     }
    //     if (path_from.ptr[i] == '/' && path_to.ptr[i] == '/') {
    //         last_saved_slash = i + 1;
    //     }
    //     if (path_from.ptr[i] == '\0' || path_to.ptr[i] == '\0') {
    //         last_saved_slash = i - 1;
    //         break;
    //     }
    //     i++;
    // }
    // i = last_saved_slash;

    char from_path[path_from.size];
    char to_path[path_to.size];

    // int len = MAX((int)path_from.size - i, 0);
    // memcpy(from_path, path_from.ptr + i, len);
    // memcpy(from_path, path_from.ptr, path_from.size);
    // from_path[len] = '\0';
    //
    // len = MAX((int)path_to.size - i, 0);
    // memcpy(to_path, path_to.ptr + i, len);
    // to_path[len] = '\0';

    // strip_end(from_path, '/');
    // strip_end(to_path, '/');

    // n1 = Count number of directories in @from
    int dir_count;
    if (path_from.size == 0) {
        dir_count = 0;
    } else {
        dir_count = get_dir_count(path_from);
    }
    
    // res = '../' * n1 + @to
    for(int i = 0; i < dir_count; i++) {
        vec_append(result, false, vec_wrap("../"));
        // strcpy(result + 3 * i, "../");
    }
    // strcpy(result + 3 * dir_count, to_path);
    vec_append(result, false, path_to);
}


int dirname_len(const char* path) {
    /*
     * Given @path (c-string) calculates directory name prefix length
     */
    if (path[strlen(path) - 1] == '/') {
        return strlen(path);
    } else {
        // Find last '/' occurence
        int i = strlen(path);
        while (i > 0) {
            if (path[i] == '/') {
                return i + 1;
            }
            i--;
        }

    }
    return 0;
}

int first_dir_len(const char* path) {
    const char* init = path;
    assert(strncmp(path, "../", 3) != 0);

    while (strncmp(path, "./", 2) == 0) {
        path += 2;
    }
    const char* slash_occ = strchr(path, '/');
    assert(slash_occ != NULL);
    return slash_occ - init;
}

void join_paths(struct vec p1, struct vec p2, struct vec* out) {
    vec_append(out, false, p1);
    if (p1.ptr[p1.size - 1] != '/' && p2.ptr[0] != '/') {
        vec_append(out, false, vec_wrap("/"));
    }
    vec_append(out, false, p2);
}
