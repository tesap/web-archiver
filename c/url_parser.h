
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

struct UrlPtrs get_url_pointers(const char* url, int size);
void parse_url_parts(const char* url, struct UrlParts* parts);
bool detect_is_file(const char* url, LinkType type_hint);
void url_to_filepath(const char* url, bool is_file, char* out_path, int* out_dir_len);
