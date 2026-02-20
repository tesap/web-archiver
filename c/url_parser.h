
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

struct UrlPtrs get_url_pointers(const char* url);
void parse_url(const char* url, struct UrlParts* parts);

bool is_url_relative(const char* url);
bool is_url_http(char* url);
