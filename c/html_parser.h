
typedef enum {
    STATE_DEFAULT,
    STATE_PROGRESS,
    // STATE_CAPTURE,
} HrefParserState;

void capture_hrefs_from_html(const char* data, int size, const char* page_url, int depth_level, void(*callback)(const char*, int, char*));
