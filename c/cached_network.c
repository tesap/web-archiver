
#include <time.h>

#include "./cached_network.h"
#include "./util.h"
#include "./url_parser.h"
#include "./html_parser.h"

int cached_download_http(const char* url, int request_timeout, int is_save, int cache_ttl, LinkType type_hint, struct HttpPage* out) {
    bool is_file = detect_is_file(url, type_hint);

    char file_path[MAX_URL_LENGTH];
    int dir_substr_len;
    url_to_filepath(url, is_file, file_path, &dir_substr_len);

    time_t cur_time = time(NULL);
    time_t mtime = get_file_last_modified(file_path);

    if (file_exists(file_path) && cache_ttl * 60 >= (cur_time - mtime)) {
        fprintf(stderr, ANSI_COLOR_GREEN "==== Cache hit: %s (file: %s)\n" ANSI_COLOR_RESET, url, file_path);
        char* buff;
        int size = read_file(file_path, &buff);

        out->headers_vec = NULL;

        out->content_vec = (struct vec*)malloc(sizeof(struct vec));
        out->content_vec->ptr = buff;
        out->content_vec->size = size;

        memcpy(out->effective_url, url, strlen(url));
        out->effective_url[strlen(url)] = '\0';
        return 1;
    }

    fprintf(stderr, ANSI_COLOR_YELLOW "==== Cache miss: %s\n\t(file: %s)\n" ANSI_COLOR_RESET, url, file_path);

    int res = download_http(url, request_timeout / 1000, out);
    if (res == 0) {
        if (is_save) {
            // Save page to FS
            const char* url = out->effective_url;
            char file_path[MAX_URL_LENGTH];
            int dir_substr_len;
            url_to_filepath(url, is_file, file_path, &dir_substr_len);



            // Create needed directory
            char dir_path[MAX_URL_LENGTH];
            memcpy(dir_path, file_path, dir_substr_len);
            dir_path[dir_substr_len] = '\0';

            int ok = mkdir_p(dir_path);

            if (ok) {
                // Save file to FS
                ok = write_file(
                    file_path,
                    out->content_vec->ptr,
                    out->content_vec->size
                );
            }

            if (!ok) {
                fprintf(stderr, ANSI_COLOR_RED "==== Could not save file: %s\n" ANSI_COLOR_RESET, file_path);
            }
        }
    }
    return res;
}

