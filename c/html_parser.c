#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./util.h"
#include "./html_parser.h"

void capture_hrefs_from_html(const char* data, int size, const char* page_url, int depth_level, void(*callback)(const char*, int, char*)) {
    /*
     * The function effectively searches for links in HTML document.
     * It uses simple per-byte parser which searches for the following match
     * and captures content inside quotes:
     *      '<a href="<CAPTURE>"'
     * 
     * It calls a @callback function on each captured URL
     * A caller does not pass ownership of @page_url, he should handle it on its own.
     */
    // TODO Could be rewritten with strstr()

    int progress_step = 0;
    HrefParserState state = STATE_DEFAULT;
    struct vec* capture_vec = vec_init(0);
    
    for (int i = 0; i < size; i++) {
        char el = data[i];
        const char* el_ptr = data + i;
        switch (state) {
            case STATE_DEFAULT:
                if (el == '<') {
                    state = STATE_PROGRESS;
                    progress_step = 1;
                }
                break;
            case STATE_PROGRESS: {
                switch (progress_step) {
                    case 1:
                        // printf("---> 1: (%c)\n", el);
                        if (el == 'a') {
                            progress_step++;
                        } else if (el == ' ' || el == '\n') {
                        } else {
                            state = STATE_DEFAULT;
                            progress_step = 0;
                        }
                        break;
                    case 2:
                        // TODO support spaces between, i.e. "< / a >"
                        // printf("---> \t2: (%.*s)\n", 10, el_ptr);
                        if (strncmp(el_ptr, "</a>", 4) == 0) {
                            state = STATE_DEFAULT;
                            progress_step = 0;
                        }
                        if (strncmp(el_ptr, "href", 4) == 0) {
                            progress_step++;
                            i += 3;
                        }
                        break;
                    case 3:
                        // printf("---> \t\t3: (%.*s)\n", 10, el_ptr);
                        if (el == '=') {
                            progress_step++;
                        } else if (el == ' ' || el == '\n') {
                        } else {
                            state = STATE_DEFAULT;
                            progress_step = 0;
                        }
                        break;
                    case 4:
                        if (el == '\"') {
                            progress_step++;
                        } else if (el == ' ' || el == '\n') {
                        } else {
                            state = STATE_DEFAULT;
                            progress_step = 0;
                        }
                        break;
                    case 5: // STATE_CAPTURE
                        if (el == '\"') {
                            vec_append(capture_vec, "\0", 1);
                            (*callback)(page_url, depth_level, capture_vec->ptr);
                            free(capture_vec);
                            capture_vec = vec_init(0);

                            state = STATE_DEFAULT;
                            progress_step = 0;
                        } else {
                            vec_append(capture_vec, el_ptr, 1);
                        }
                        break;
                    default:
                        fprintf(stderr, "=== Unknown progress_step: %d\n", progress_step);
                }
            }
                
            // default:
            //     Skip, go on
        }
    }
    vec_deinit(capture_vec);
}
