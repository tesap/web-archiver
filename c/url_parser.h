
#include "./html_parser.h"

struct UrlPtrs {
    const char* protocol_start;
    const char* protocol_end;
    const char* host_start;
    const char* host_end;
    const char* path_start;
    const char* path_end;
};

struct UrlParts {
    char protocol[16];
    char host[256];
    char path[1024];
};

bool is_url_relative(const char* url);
bool is_url_http(const char* url);

struct UrlPtrs get_url_pointers(struct vec url);
void parse_url_parts(const char* url, struct UrlParts* parts);
bool detect_is_file(struct vec url, LinkType type_hint);
void url_to_filepath(struct vec url, bool is_file, struct vec* out);
void get_relpath(
    struct vec path_from,
    struct vec path_to,
    struct vec* result
);
int dirname_len(const char* path);
int first_dir_len(const char* path);
int get_dir_count(struct vec path);
void join_paths(struct vec p1, struct vec p2, struct vec* out);
