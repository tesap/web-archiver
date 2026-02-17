
struct UrlParts {
    char protocol[16];
    char host[256];
    char path[1024];
    // TODO: int port;
};

void parse_url(const char* url, struct UrlParts* parts);
bool is_url_relative(char* url);
bool is_url_http(char* url);
