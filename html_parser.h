
#ifndef HTML_PARSER_H
#define HTML_PARSER_H

#include "./util.h"

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
    struct vec link;
    struct vec type;
};

struct HtmlTag parse_html_tag(const char* buff);
void iter_html_tags(
    struct vec data,
    // const char* buff,
    // int size,
    void(*callback)(struct HtmlTag* t, void* ctx),
    void* ctx
);

#endif
