
#include <time.h>

#include "./cached_network.h"
#include "./util.h"
#include "./url_parser.h"

void save_downloaded_page(const struct HttpPage* hp) {
    const char* url = hp->effective_url;
    struct UrlPaths paths;
    parse_url_paths(url, &paths);

    // Create needed directory
    if (!mkdir_p(paths.dir_path)) {
        return;
    }
    // Save file to FS
    write_file(
        paths.file_path, 
        hp->content_vec->ptr,
        hp->content_vec->size
    );
}

int cached_download_http(const char* url, int request_timeout, int is_save, int cache_ttl, struct HttpPage* out) {
    struct UrlPaths paths;
    parse_url_paths(url, &paths);
    time_t cur_time = time(NULL);
    time_t mtime = get_file_last_modified(paths.file_path);

    if (file_exists(paths.file_path) && cache_ttl * 60 >= (cur_time - mtime)) {
        fprintf(stderr, ANSI_COLOR_GREEN "==== Cache hit: %s (file: %s)\n" ANSI_COLOR_RESET, url, paths.file_path);
        char* buff;
        int size = read_file(paths.file_path, &buff);

        out->headers_vec = NULL;

        out->content_vec = (struct vec*)malloc(sizeof(struct vec));
        out->content_vec->ptr = buff;
        out->content_vec->size = size;

        memcpy(out->effective_url, url, strlen(url));
        out->effective_url[strlen(url)] = '\0';
        return 1;
    }

    fprintf(stderr, ANSI_COLOR_YELLOW "==== Cache miss: %s\n\t(file: %s)\n" ANSI_COLOR_RESET, url, paths.file_path);

    int res = download_http(url, request_timeout / 1000, out);
    if (res == 0) {
        if (is_save) {
            // Save page to FS
            save_downloaded_page(out);
        }
    }
    return res;
}

