
typedef enum {
    HREF_TYPE_UNKNOWN,
    HREF_TYPE_HTML,
    HREF_TYPE_IMG,
    HREF_TYPE_STYLE,
    HREF_TYPE_SCRIPT,
} HrefType;

HrefType href_type(
    const char* href_attr,
    const char* type_attr,
    const char* elem_name
);
void print_href_type(HrefType ht);

void parse_html_elem(
    char** ptr_start,
    const char* href_attr_name,
    const char* elem_name,
    void(*callback)(const char* href, HrefType ht, void* ctx),
    void* ctx
);

void search_html_hrefs(
    const char* data,
    int size,
    void(*callback)(const char* href, HrefType ht, void* ctx),
    void* ctx
);
