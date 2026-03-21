#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "./util.h"
#include "./html_parser.h"
#include "./url_parser.h"

void print_href_type(HrefType ht) {
    switch (ht) {
        case HREF_TYPE_UNKNOWN: printf("HREF_TYPE_UNKNOWN"); break;
        case HREF_TYPE_IMG: printf("HREF_TYPE_IMG"); break;
        case HREF_TYPE_STYLE: printf("HREF_TYPE_STYLE"); break;
        case HREF_TYPE_SCRIPT: printf("HREF_TYPE_SCRIPT"); break;
        case HREF_TYPE_HTML: printf("HREF_TYPE_HTML"); break;
    }
}

HrefType href_type(
    const char* link_attr,
    const char* type_attr,
    const char* elem_name
) {
    if (strncmp(elem_name, "a", 1) == 0) {
        return HREF_TYPE_HTML;
    }
    if (strncmp(elem_name, "img", 3) == 0) {
        return HREF_TYPE_IMG;
    }
    if (strncmp(elem_name, "script", 6) == 0) {
        return HREF_TYPE_SCRIPT;
    }

    if (type_attr && strlen(type_attr) > 0) {
        if (strncmp(type_attr, "text/css", 8) == 0) {
            return HREF_TYPE_STYLE;
        }
        if (strncmp(type_attr, "image/", 6) == 0) {
            return HREF_TYPE_IMG;
        }
        if (strncmp(type_attr, "text/javascript", 15) == 0) {
            return HREF_TYPE_SCRIPT;
        }
    } else {
        assert(link_attr != NULL);
        int len = strlen(link_attr);
        assert(len > 0);

        struct UrlPtrs ptrs = get_url_pointers(link_attr);
        if (strncmp(ptrs.path_end - 4, ".png", 4) == 0) {
            return HREF_TYPE_IMG;
        }
        if (strncmp(ptrs.path_end - 4, ".jpg", 4) == 0) {
            return HREF_TYPE_IMG;
        }
        if (strncmp(ptrs.path_end - 4, ".css", 4) == 0) {
            return HREF_TYPE_STYLE;
        }
        if (strncmp(ptrs.path_end - 3, ".js", 3) == 0) {
            return HREF_TYPE_SCRIPT;
        }
    }

    return HREF_TYPE_UNKNOWN;
}

struct CapturedLink {
    char* link_start;
    int link_size;
    char* type_start;
    int type_size;
};

enum CaptureAttr {
    CAPTURE_LINK,
    CAPTURE_TYPE,
    CAPTURE_NO,
};

struct CapturedLink parse_html_tag(
    char** ptr_start,
    const char* elem_name, // f.e. "a" or "script" or "link"
    const char* link_attr_name // f.e. "href" or "src"
) {
    /*
     * The function effectively searches for <a> elements in HTML document.
     * It uses simple per-byte parser which searches for the following match
     * and captures content inside quotes:
     *      '<a .* type="<CAPTURE_1>" .* href="<CAPTURE_2>">.*</a>"'
     *      '<img  .* src="<CAPTURE>"'
     *      '<link .* href="<CAPTURE>"'
     * 
     * A caller does not pass ownership of @page_url, he should handle it on its own.
     */

    int progress_step = 0;
    CaptureAttr capture_attr = CAPTURE_NO;
    char* capture_start = NULL;

    char* link_start = NULL;
    char* type_start = NULL;
    int link_size = 0;
    int type_size = 0;

    char link_attr_prefix[strlen(link_attr_name) + 3];
    sprintf(link_attr_prefix, "%s=\"", link_attr_name);

    char* el_ptr = *ptr_start;
    while ((el_ptr - *ptr_start) < MAX_HTML_TAG_LENGTH) {
        switch (progress_step) {
            case 0:
                // printf("---> \t2: (%.*s)\n", 10, el_ptr);
                if (*el_ptr == '>') {
                    *ptr_start = el_ptr;
                    goto end;
                } else if (strncmp(el_ptr, link_attr_prefix, strlen(link_attr_prefix)) == 0) {
                    progress_step++;
                    el_ptr += strlen(link_attr_prefix) - 1;
                    capture_start = el_ptr + 1;
                    capture_attr = CAPTURE_LINK;
                } else if (strncmp(el_ptr, "type=\"", 6) == 0) {
                    progress_step++;
                    el_ptr += 5;
                    capture_start = el_ptr + 1;
                    capture_attr = CAPTURE_TYPE;
                }
                break;
            case 1: // STATE_CAPTURE
                if (*el_ptr == '\"') { // End of arg capture
                    int captured_size = el_ptr - capture_start;
                    if (captured_size > 0) {
                        if (capture_attr == CAPTURE_LINK) {
                            link_start = capture_start;
                            link_size = captured_size;
                        } else if (capture_attr == CAPTURE_TYPE) {
                            type_start = capture_start;
                            type_size = captured_size;
                        }
                    }
                    progress_step = 0;
                    capture_attr = CAPTURE_NO;
                    capture_start = NULL;
                }
                break;
            default:
                fprintf(stderr, "=== Unknown progress_step: %d\n", progress_step);

        }
        el_ptr++;
    }

end:
    *ptr_start = el_ptr;
    return {
        .link_start = link_start,
        .link_size = link_size,
        .type_start = type_start,
        .type_size = type_size
    };
}


void search_resource_urls(
    const char* data,
    int size,
    void(*callback)(const char* link_start, int link_size, HrefType ht, void* ctx),
    void* ctx
) {

    char* el_ptr = (char*)data;
    while ((el_ptr - data) < size) {
        struct CapturedLink l = { NULL, 0, NULL, 0 };
        char* elem_name;
        if (strncmp(el_ptr, "<a", 2) == 0) {
            l = parse_html_tag(&el_ptr, "a", "href");
            elem_name = "a";
        }
        else if (strncmp(el_ptr, "<link", 5) == 0) {
            l = parse_html_tag(&el_ptr, "link", "href");
            elem_name = "link";
        }
        else if (strncmp(el_ptr, "<img", 4) == 0) {
            l = parse_html_tag(&el_ptr, "img", "src");
            elem_name = "img";
        }
        else if (strncmp(el_ptr, "<script", 7) == 0) {
            l = parse_html_tag(&el_ptr, "script", "src");
            elem_name = "script";
        }

        if (l.link_start) {
            char link_attr[MAX_URL_LENGTH];
            char type_attr[64] = "";

            memcpy(link_attr, l.link_start, l.link_size);
            link_attr[l.link_size] = '\0';

            HrefType ht = HREF_TYPE_UNKNOWN;
            if (l.type_start) {
                memcpy(type_attr, l.type_start, l.type_size);
                type_attr[l.type_size] = '\0';
            }

            ht = href_type(link_attr, type_attr, elem_name);
            callback(l.link_start, l.link_size, ht, ctx);
        }
        el_ptr++;
    }
}
