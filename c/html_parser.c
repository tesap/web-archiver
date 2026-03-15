#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "./util.h"
#include "./html_parser.h"

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
    const char* href_attr,
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

    if (type_attr) {
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
        assert(href_attr != NULL);
        int len = strlen(href_attr);
        if (strncmp(href_attr + len - 4, ".png", 4) == 0) {
            return HREF_TYPE_IMG;
        }
        if (strncmp(href_attr + len - 4, ".jpg", 4) == 0) {
            return HREF_TYPE_IMG;
        }
        if (strncmp(href_attr + len - 4, ".css", 4) == 0) {
            return HREF_TYPE_STYLE;
        }
        if (strncmp(href_attr + len - 3, ".js", 3) == 0) {
            return HREF_TYPE_SCRIPT;
        }
    }

    return HREF_TYPE_UNKNOWN;
}

void parse_html_elem(
    char** ptr_start,
    const char* elem_name, // f.e. "a" or "script" or "link"
    const char* href_attr_name, // f.e. "href" or "src"
    void(*callback)(const char* href, HrefType ht, void* ctx),
    void* ctx
) {
    /*
     * The function effectively searches for <a> elements in HTML document.
     * It uses simple per-byte parser which searches for the following match
     * and captures content inside quotes:
     *      '<a .* type="<CAPTURE_1>" .* href="<CAPTURE_2>">.*</a>"'
     *      '<img  .* src="<CAPTURE>"'
     *      '<link .* href="<CAPTURE>"'
     * 
     * It calls a @callback function on each captured URL
     * A caller does not pass ownership of @page_url, he should handle it on its own.
     */

    int progress_step = 0;
    char* capturing_arg = NULL;
    char* href_arg = NULL;
    char* type_arg = NULL;

    struct vec* capture_vec = vec_init(0);

    char href_attr_match[strlen(href_attr_name) + 3];
    sprintf(href_attr_match, "%s=\"", href_attr_name);

    #define MAX_ELEM_SIZE 1024
    char* el_ptr = *ptr_start;
    while ((el_ptr - *ptr_start) < MAX_ELEM_SIZE) {
        switch (progress_step) {
            case 0:
                // printf("---> \t2: (%.*s)\n", 10, el_ptr);
                if (*el_ptr == '>') {
                    if (href_arg) {
                        HrefType ht = href_type(href_arg, type_arg, elem_name);
                        (*callback)(href_arg, ht, ctx);
                    }

                    *ptr_start = el_ptr + 1;
                    goto cleanup;
                } else if (strncmp(el_ptr, href_attr_match, strlen(href_attr_match)) == 0) {
                    progress_step++;
                    el_ptr += strlen(href_attr_match) - 1;
                    capturing_arg = "href";
                } else if (strncmp(el_ptr, "type=\"", 6) == 0) {
                    progress_step++;
                    el_ptr += 5;
                    capturing_arg = "type";
                }
                break;
            case 1: // STATE_CAPTURE
                if (*el_ptr == '\"') { // End of arg capture
                    vec_append(capture_vec, "\0", 1);
                    if (strlen(capture_vec->ptr) > 0) {
                        if (strncmp(capturing_arg, "href", 4) == 0) {
                            href_arg = capture_vec->ptr;
                        } else if (strncmp(capturing_arg, "type", 4) == 0) {
                            type_arg = capture_vec->ptr;
                        }
                    }
                    progress_step = 0;
                    capturing_arg = NULL;
                    free(capture_vec);
                    capture_vec = vec_init(0);
                } else {
                    vec_append(capture_vec, el_ptr, 1);
                }
                break;
            default:
                fprintf(stderr, "=== Unknown progress_step: %d\n", progress_step);

        }
        el_ptr++;
    }
    *ptr_start = el_ptr;

cleanup:
    vec_deinit(capture_vec);
}


void search_html_hrefs(
    const char* data,
    int size,
    void(*callback)(const char* href, HrefType ht, void* ctx),
    void* ctx
) {

    char* el_ptr = (char*)data;
    while ((el_ptr - data) < size) {
        if (strncmp(el_ptr, "<a", 2) == 0) {
            parse_html_elem(&el_ptr, "a", "href", callback, ctx);
        }
        if (strncmp(el_ptr, "<link", 5) == 0) {
            parse_html_elem(&el_ptr, "link", "href", callback, ctx);
        }
        if (strncmp(el_ptr, "<img", 4) == 0) {
            parse_html_elem(&el_ptr, "img", "src", callback, ctx);
        }
        if (strncmp(el_ptr, "<script", 7) == 0) {
            parse_html_elem(&el_ptr, "script", "src", callback, ctx);
        }
        el_ptr++;
    }
}
