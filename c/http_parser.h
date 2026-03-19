
// const char* get_content_start(const char* response);
void parse_http_stream_chunk(const char* recv_buff, int size, struct vec* headers_vec, struct vec* content_vec, bool* content_started);
int get_location_header(const char* response, const char* request_url, char* result);
