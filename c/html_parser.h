
typedef enum {
    HREF_TYPE_UNKNOWN,
    HREF_TYPE_HTML,
    HREF_TYPE_IMG,
    HREF_TYPE_STYLE,
    HREF_TYPE_SCRIPT,
} HrefType;

void capture_hrefs_from_html(const char* data, int size, const char* page_url, int depth_level, void(*callback)(const char*, int, char*, HrefType));
