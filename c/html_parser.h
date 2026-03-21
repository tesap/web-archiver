
#ifndef HTML_PARSER_H
#define HTML_PARSER_H

typedef enum {
    LINK_TYPE_UNKNOWN,
    LINK_TYPE_HTML,
    LINK_TYPE_IMG,
    LINK_TYPE_STYLE,
    LINK_TYPE_SCRIPT,
} LinkType;

LinkType tag_link_type(struct HtmlTag* t);
void print_link_type(LinkType lt);

struct HtmlTag {
    char tag_name[16];
    const char* tag_end;
    const char* link_start;
    int link_size;
    const char* type_start;
    int type_size;
};

struct HtmlTag parse_html_tag(const char* buff);
void iter_html_tags(
    const char* buff,
    int size,
    void(*callback)(struct HtmlTag* t, void* ctx),
    void* ctx
);

#endif
