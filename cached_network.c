
#include <time.h>

#include "./cached_network.h"
#include "./util.h"
#include "./url_parser.h"
#include "./html_parser.h"

int cached_download_http(struct vec url, int request_timeout, int is_save, int cache_ttl, LinkType type_hint, struct HttpPage* out) {
    bool is_file = detect_is_file(url, type_hint);

    char url_path[MAX_URL_LENGTH];
    // BAD! TODO think how to restrict it
    // struct vec v = vec_wrap(url_path);
    struct vec v = {url_path, 0};

    url_to_filepath(url, is_file, &v);
    vec_terminate(&v);
    int dir_substr_len = dirname_len(url_path);

    time_t cur_time = time(NULL);
    time_t mtime = get_file_last_modified(url_path);

    if (file_exists(url_path) && cache_ttl * 60 >= (cur_time - mtime)) {
        fprintf(stderr, ANSI_COLOR_GREEN "==== Cache hit: %.*s (file: %s)\n" ANSI_COLOR_RESET, url.size, url.ptr, url_path);
        char* buff;
        int size = read_file(url_path, &buff);

        out->headers_vec = NULL;

        out->content_vec = (struct vec*)malloc(sizeof(struct vec));
        out->content_vec->ptr = buff;
        out->content_vec->size = size;

        memcpy(out->effective_url, url.ptr, url.size);
        out->effective_url[url.size] = '\0';
        return 1;
    }

    fprintf(stderr, ANSI_COLOR_YELLOW "==== Cache miss: %.*s\n\t(file: %s)\n" ANSI_COLOR_RESET, url.size, url.ptr, url_path);

    int res = download_http(url.ptr, request_timeout / 1000, out);
    if (res == 0) {
        if (is_save) {
            // Save page to FS
            char effective_url_path[MAX_URL_LENGTH];
            struct vec _v1 = {effective_url_path, 0};
            url_to_filepath(vec_wrap(out->effective_url), is_file, &_v1);
            vec_terminate(&_v1);
            int dir_substr_len = dirname_len(effective_url_path);

            // Create needed directory
            char dir_path[MAX_URL_LENGTH];
            memcpy(dir_path, effective_url_path, dir_substr_len);
            dir_path[dir_substr_len] = '\0';

            int ok = mkdir_p(dir_path);

            if (ok) {
                // Save file to FS
                ok = write_file(
                    effective_url_path,
                    out->content_vec->ptr,
                    out->content_vec->size
                );
            }

            if (!ok) {
                fprintf(stderr, ANSI_COLOR_RED "==== Could not save file: %s\n" ANSI_COLOR_RESET, effective_url_path);
            }
        }
    }
    return res;
}

