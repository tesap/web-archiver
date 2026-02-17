

struct HttpPage {
    char effective_url[256];
    // char* http_data;
    struct vec* data_vec;
    int content_offset; // Offset to start of HTTP content inside data_vec
};

int create_socket(const char* hostname, const char* service);
int download_http(const char* url, struct HttpPage* res_page);

