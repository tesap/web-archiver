
#ifndef HTML_PARSER_H
#define HTML_PARSER_H

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

void search_resource_urls(
    const char* data,
    int size,
    void(*callback)(const char* link_start, int link_size, HrefType ht, void* ctx),
    void* ctx
);

#endif
