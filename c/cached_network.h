
#include "./network.h"

void save_downloaded_page(const struct HttpPage* hp);
int cached_download_http(const char* url, int request_timeout, int is_save, int cache_ttl, struct HttpPage* out);
