#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "./util.h"
#include "./html_parser.h"
#include "./url_parser.h"

void print_link_type(LinkType ht) {
    switch (ht) {
        case LINK_TYPE_UNKNOWN: printf("LINK_TYPE_UNKNOWN"); break;
        case LINK_TYPE_IMG: printf("LINK_TYPE_IMG"); break;
        case LINK_TYPE_STYLE: printf("LINK_TYPE_STYLE"); break;
        case LINK_TYPE_SCRIPT: printf("LINK_TYPE_SCRIPT"); break;
        case LINK_TYPE_HTML: printf("LINK_TYPE_HTML"); break;
    }
}

LinkType tag_link_type(struct HtmlTag* t) {
    if (strncmp(t->tag_name, "a", 1) == 0) {
        return LINK_TYPE_HTML;
    }
    if (strncmp(t->tag_name, "img", 3) == 0) {
        return LINK_TYPE_IMG;
    }
    if (strncmp(t->tag_name, "script", 6) == 0) {
        return LINK_TYPE_SCRIPT;
    }

    if (t->type_start) {
        if (strncmp(t->type_start, "text/css", 8) == 0) {
            return LINK_TYPE_STYLE;
        }
        if (strncmp(t->type_start, "image/", 6) == 0) {
            return LINK_TYPE_IMG;
        }
        if (strncmp(t->type_start, "text/javascript", 15) == 0) {
            return LINK_TYPE_SCRIPT;
        }
    } else {
        assert(t->link_start != NULL);

        struct UrlPtrs ptrs = get_url_pointers(t->link_start, t->link_size);
        if (strncmp(ptrs.path_end - 4, ".png", 4) == 0) {
            return LINK_TYPE_IMG;
        }
        if (strncmp(ptrs.path_end - 4, ".jpg", 4) == 0) {
            return LINK_TYPE_IMG;
        }
        if (strncmp(ptrs.path_end - 4, ".css", 4) == 0) {
            return LINK_TYPE_STYLE;
        }
        if (strncmp(ptrs.path_end - 3, ".js", 3) == 0) {
            return LINK_TYPE_SCRIPT;
        }
    }

    return LINK_TYPE_UNKNOWN;
}

enum CaptureAttr {
    CAPTURE_LINK,
    CAPTURE_TYPE,
    CAPTURE_NO,
};

struct HtmlTag parse_html_tag(const char* buff) {
    /*
     * The function effectively searches for a,link,img,script tags in @buff.
     * It uses simple per-byte parser which searches for the following match
     * and captures content inside quotes:
     *      '<a .* type="<CAPTURE_1>" .* href="<CAPTURE_2>">.*</a>"'
     * 
     * A caller does not pass ownership of @page_url, he should handle it on its own.
     *
     * @buff is not expected to have '>' occurence
     */

    struct HtmlTag out = {
        { NULL },
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    };

    const char* ptr = buff;
    ptr++; // Skip '<'

    char* link_attr_prefix;
    if (strncmp(ptr, "a", 1) == 0) {
        strcpy(out.tag_name, "a");
        link_attr_prefix = "href=\"";
    }
    else if (strncmp(ptr, "link", 4) == 0) {
        strcpy(out.tag_name, "link");
        link_attr_prefix = "href=\"";
    }
    else if (strncmp(ptr, "img", 3) == 0) {
        strcpy(out.tag_name, "img");
        link_attr_prefix = "src=\"";
    }
    else if (strncmp(ptr, "script", 6) == 0) {
        strcpy(out.tag_name, "script");
        link_attr_prefix = "src=\"";
    } else {
        return out;
    }

    int progress_step = 0;
    CaptureAttr capture_attr = CAPTURE_NO;
    const char* capture_start = NULL;

    while ((ptr - buff) < MAX_HTML_TAG_LENGTH) {
        switch (progress_step) {
            case 0:
                // printf("---> \t2: (%.*s)\n", 10, ptr);
                if (*ptr == '>') {
                    goto end;
                } else if (strncmp(ptr, link_attr_prefix, strlen(link_attr_prefix)) == 0) {
                    progress_step++;
                    ptr += strlen(link_attr_prefix) - 1;
                    capture_start = ptr + 1;
                    capture_attr = CAPTURE_LINK;
                } else if (strncmp(ptr, "type=\"", 6) == 0) {
                    progress_step++;
                    ptr += 5;
                    capture_start = ptr + 1;
                    capture_attr = CAPTURE_TYPE;
                }
                break;
            case 1: // STATE_CAPTURE
                if (*ptr == '\"') { // End of arg capture
                    int captured_size = ptr - capture_start;
                    if (captured_size > 0) {
                        if (capture_attr == CAPTURE_LINK) {
                            out.link_start = capture_start;
                            out.link_size = captured_size;
                        } else if (capture_attr == CAPTURE_TYPE) {
                            out.type_start = capture_start;
                            out.type_size = captured_size;
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
        ptr++;
    }

end:
    out.tag_end = ptr;
    return out;
}


void iter_html_tags(
    const char* buff,
    int size,
    void(*callback)(struct HtmlTag* t, void* ctx),
    void* ctx
) {
    /*
     * Takes: ptr to string
     * Calls @callback on each occurence of URL
     */
    const char* ptr = buff;
    int left = size;
    while (left > 0 && (ptr = (const char*)memchr(ptr, '<', left))) {
        left = size - (ptr - buff);
        
        struct HtmlTag t = parse_html_tag(ptr);
        if (t.link_start == NULL) {
            ptr++;
            continue;
        }

        callback(&t, ctx);
        ptr = t.tag_end + 1;
    }
}
//
//     // TODO can we make it const?
//     char* el_ptr = (char*)ptr;
//     while ((el_ptr - ptr) < size) {
//         struct HtmlTag l = { NULL, 0, NULL, 0 };
//         char* elem_name;
//         if (strncmp(el_ptr, "<a", 2) == 0) {
//             l = parse_html_tag(&el_ptr, "a", "href");
//             elem_name = "a";
//         }
//         else if (strncmp(el_ptr, "<link", 5) == 0) {
//             l = parse_html_tag(&el_ptr, "link", "href");
//             elem_name = "link";
//         }
//         else if (strncmp(el_ptr, "<img", 4) == 0) {
//             l = parse_html_tag(&el_ptr, "img", "src");
//             elem_name = "img";
//         }
//         else if (strncmp(el_ptr, "<script", 7) == 0) {
//             l = parse_html_tag(&el_ptr, "script", "src");
//             elem_name = "script";
//         }
//
//         if (l.link_start) {
//             char link_attr[MAX_URL_LENGTH];
//             char type_attr[64] = "";
//
//             memcpy(link_attr, l.link_start, l.link_size);
//             link_attr[l.link_size] = '\0';
//
//             LinkType ht = LINK_TYPE_UNKNOWN;
//             if (l.type_start) {
//                 memcpy(type_attr, l.type_start, l.type_size);
//                 type_attr[l.type_size] = '\0';
//             }
//
//             ht = tag_link_type(link_attr, type_attr, elem_name);
//             callback(l.link_start, l.link_size, ht, ctx);
//         }
//         el_ptr++;
//     }
// }
