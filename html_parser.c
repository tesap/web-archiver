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

    if (t->type.ptr) {
        if (strncmp(t->type.ptr, "text/css", 8) == 0) {
            return LINK_TYPE_STYLE;
        }
        if (strncmp(t->type.ptr, "image/", 6) == 0) {
            return LINK_TYPE_IMG;
        }
        if (strncmp(t->type.ptr, "text/javascript", 15) == 0) {
            return LINK_TYPE_SCRIPT;
        }
    } else {
        assert(t->link.ptr != NULL);

        struct UrlPtrs ptrs = get_url_pointers(t->link);
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
        NULL
        // NULL,
        // NULL
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
                            // out.link = { capture_start, captured_size };
                            out.link.ptr = (char*)capture_start;
                            out.link.size = captured_size;
                        } else if (capture_attr == CAPTURE_TYPE) {
                            // out.type = { capture_start, captured_size };
                            out.type.ptr = (char*)capture_start;
                            out.type.size = captured_size;
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
    struct vec data,
    void(*callback)(struct HtmlTag* t, void* ctx),
    void* ctx
) {
    /*
     * Takes: ptr to string
     * Calls @callback on each occurence of URL
     */
    const char* init = data.ptr;
    const char* ptr = data.ptr;
    int left = data.size;
    while (left > 0 && (ptr = (const char*)memchr(ptr, '<', left))) {
        left = data.size - (ptr - init);
        
        struct HtmlTag t = parse_html_tag(ptr);
        if (t.link.ptr == NULL) {
            ptr++;
            continue;
        }

        callback(&t, ctx);
        ptr = t.tag_end + 1;
    }
}
