#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "./util.h"
#include "./html_parser.h"

void skip_spaces(char** ptr) {
    while (**ptr == ' ' || **ptr == '\n') {
        (*ptr)++;
    }
}

HrefType href_type(
    const char* href_argument,
    const char* type_argument,
    const char* element_type
) {
    if (strncmp(element_type, "a", 1) == 0) {
        return HREF_TYPE_HTML;
    }
    if (strncmp(element_type, "img", 3) == 0) {
        return HREF_TYPE_IMG;
    }
    if (strncmp(element_type, "script", 6) == 0) {
        return HREF_TYPE_SCRIPT;
    }

    if (type_argument) {
        if (strncmp(type_argument, "text/css", 8) == 0) {
            return HREF_TYPE_STYLE;
        }
        if (strncmp(type_argument, "image/", 6) == 0) {
            return HREF_TYPE_IMG;
        }
        if (strncmp(type_argument, "text/javascript", 15) == 0) {
            return HREF_TYPE_SCRIPT;
        }
    } else {
        assert(href_argument != NULL);
        int len = strlen(href_argument);
        if (strncmp(href_argument + len - 4, ".png", 4) == 0) {
            return HREF_TYPE_IMG;
        }
        if (strncmp(href_argument + len - 4, ".jpg", 4) == 0) {
            return HREF_TYPE_IMG;
        }
        if (strncmp(href_argument + len - 4, ".css", 4) == 0) {
            return HREF_TYPE_STYLE;
        }
        if (strncmp(href_argument + len - 3, ".js", 3) == 0) {
            return HREF_TYPE_SCRIPT;
        }
    }

    return HREF_TYPE_UNKNOWN;
}

void parse_html_elem(
    char** ptr_start,
    const char* element_type,
    const char* page_url,
    const int depth_level,
    void(*callback)(const char*, int, char*, HrefType)
) {
    /*
     * The function effectively searches for <a> elements in HTML document.
     * It uses simple per-byte parser which searches for the following match
     * and captures content inside quotes:
     *      '<a .* type="<CAPTURE_1>" .* href="<CAPTURE_2>">.*</a>"'
     // *      '<img  .* src="<CAPTURE>"'
     // *      '<link .* href="<CAPTURE>"'
     * 
     * It calls a @callback function on each captured URL
     * A caller does not pass ownership of @page_url, he should handle it on its own.
     */
    // TODO Could be rewritten with strstr()

    int progress_step = 0;
    char* capturing_arg = NULL;
    char* href_arg = NULL;
    char* type_arg = NULL;

    struct vec* capture_vec = vec_init(0);

    #define MAX_ELEM_SIZE 256
    char* el_ptr = *ptr_start;
    while ((el_ptr - *ptr_start) < MAX_ELEM_SIZE) {
        switch (progress_step) {
            case 0:
                // printf("---> \t2: (%.*s)\n", 10, el_ptr);
                if (*el_ptr == '>') {
                    if (type_arg) {
                        printf("=== type: %s\n", type_arg);
                    }
                    if (href_arg) {
                        HrefType ht = href_type(href_arg, type_arg, element_type);
                        (*callback)(page_url, depth_level, href_arg, ht);
                    }

                    *ptr_start = el_ptr + 4;
                    goto cleanup;
                } else if (strncmp(el_ptr, "href=\"", 6) == 0) {
                    progress_step++;
                    el_ptr += 5;
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

void capture_hrefs_from_html(
    const char* data,
    int size,
    const char* page_url,
    int depth_level,
    void(*callback)(const char*, int, char*, HrefType)
) {
    char* el_ptr = (char*)data;
    while ((el_ptr - data) < size) {
        if (strncmp(el_ptr, "<a", 2) == 0) {
            parse_html_elem(&el_ptr, "a", page_url, depth_level, callback);
        }
        if (strncmp(el_ptr, "<link", 5) == 0) {
            parse_html_elem(&el_ptr, "link", page_url, depth_level, callback);
        }
        if (strncmp(el_ptr, "<img", 4) == 0) {
            parse_html_elem(&el_ptr, "img", page_url, depth_level, callback);
        }
        if (strncmp(el_ptr, "<script", 7) == 0) {
            parse_html_elem(&el_ptr, "script", page_url, depth_level, callback);
        }
        el_ptr++;
    }
}
