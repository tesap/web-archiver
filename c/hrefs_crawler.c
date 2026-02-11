#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <ctype.h>

#include <curl/curl.h>

#define DEFAULT_DEPTH_LEVEL 1
#define DEFAULT_REQUEST_PERIOD 50
#define DEFAULT_REQUEST_TIMEOUT 30000

typedef enum {
    DOMAIN_FILTER_NO,
    DOMAIN_FILTER_SAME,
    DOMAIN_FILTER_SUBDOMAIN,
} DomainFilterType;

struct cmd_args {
    const char* root_url;
    int depth_level;
    int request_period;
    int request_timeout;
    DomainFilterType filter_type;
};

struct cmd_args cmd_args = {
    .root_url = "",
    .depth_level = DEFAULT_DEPTH_LEVEL,
    .request_period = DEFAULT_REQUEST_PERIOD,
    .request_timeout = DEFAULT_REQUEST_TIMEOUT,
    .filter_type = DOMAIN_FILTER_NO,
};


struct vec {
    size_t size;
    char* ptr;
    bool allocated;
};

struct vec* vec_init(size_t newsize) {
    struct vec* v = (struct vec*)malloc(sizeof(struct vec));
    v->ptr = (char*)malloc(sizeof(char) * newsize);
    v->size = newsize;
    return v;
}

void vec_deinit(struct vec* v) {
    free(v->ptr);
    free(v);
}

void vec_append(struct vec* v, const char* buffer, size_t newsize) {
    if (newsize <= 0) {
        fprintf(stderr, "Error vec_append: newsize = %d\n", newsize);
        exit(1);
    }
    if (!v || !v->ptr) {
        fprintf(stderr, "v->ptr is NULL\n");
        exit(1);
    }

    void* ptr = realloc(v->ptr, v->size + newsize);
    if(!ptr) {
        /* out of memory */
        fprintf(stderr, "Not enough memory (realloc returned NULL): %s\n", strerror(errno));
        exit(1);
        return;
    }
    v->ptr = (char *)ptr;

    memcpy((v->ptr + v->size), buffer, newsize);
    v->size += newsize;
}

typedef enum {
    STATE_DEFAULT,
    STATE_PROGRESS,
    // STATE_CAPTURE,
} HrefParserState;

char* capture_domain_from_url(const char* url) {
    /*
     * '.*://<CAPTURE>/.*'
     */

    int state = 0;
    struct vec* capture_vec = vec_init(0);

    for (int i = 0; i < strlen(url); i++) {
        char el = url[i];
        const char* el_ptr = url + i;
        switch (state) {
            case 0:
                if (strncmp(el_ptr, "://", 3) == 0) {
                    state++;
                    i += 2;
                }
                break;
            case 1:
                if (el == '/') {
                    // Capture END
                    vec_append(capture_vec, "\0", 1);
                    goto end_loop;
                } else {
                    vec_append(capture_vec, el_ptr, 1);
                }
            // default:
            //     break;
        }
    }

    // Loop ended without goto
    // fprintf(stderr, "=== capture_domain_from_url: Capture has not reached END\n");

end_loop:; 
    char* result = capture_vec->ptr;
    free(capture_vec);
    return result;
}

bool is_number(char* s) {
    char* i = s;
    while (*i != '\0') {
        if (!isdigit((unsigned char)*i)) {
            return false;
        }
        i++;
    }
    return true;
}

bool ends_with(char* str, char* suffix) {
    // If the suffix is longer than the string, it cannot be a suffix.
    int len_suffix = strlen(suffix);
    int len_str = strlen(str);
    int delta = len_str - len_suffix;

    if (delta < 0) {
        return 0;
    }

    return (strcmp(str + delta, suffix) == 0);
}

void capture_hrefs_from_html(struct vec* data, const char* page_url, int depth_level, void(*callback)(const char*, int, char*)) {
    /*
     * The function effectively searches for links in HTML document.
     * It uses simple per-byte parser which searches for the following match
     * and captures content inside quotes:
     *      '<a href="<CAPTURE>"'
     * 
     * It calls a @callback function on each captured URL
     * A caller does not pass ownership of @page_url, he should handle it on its own.
     */

    int progress_step = 0;
    HrefParserState state = STATE_DEFAULT;
    struct vec* capture_vec = vec_init(0);
    
    for (int i = 0; i < data->size; i++) {
        char el = data->ptr[i];
        char* el_ptr = data-> ptr + i;
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

bool is_url_relative(char* url) {
    /*
     * Tells whether a URL refers relative path.
     * which means it is either refers relative path or an http/https URL.
     */
    return strncmp(url, "/", 1) == 0;
}

bool is_url_http(char* url) {
    /*
     * Tells whether a URL is http/https.
     */
    return strncmp(url, "http", 4) == 0;
}

void crawl_urls(char* url, int depth_level);

void handle_found_url_cb(const char* page_url, int depth_level, char* found_url) {
    /*
     * The function takes ownership of @found_url, i.e. it frees it at the end.
     */
    char* result_url;
    if (is_url_http(found_url)) {
        result_url = found_url;
    } else if (is_url_relative(found_url)) {
        char* stripped_found_url = found_url + 1;
        int full_url_len = strlen(page_url) + strlen(stripped_found_url) + 1;
        char* full_url = (char *)malloc(full_url_len * sizeof(char));
        
        sprintf(full_url, "%s%s", page_url, stripped_found_url);
        result_url = full_url;
        free(found_url);
    } else {
        // URL does not match needed criterias
        return;
    }

    if (cmd_args.filter_type != DOMAIN_FILTER_NO) {
        char* domain_result_url = capture_domain_from_url(result_url);
        char* domain_page_url = capture_domain_from_url(page_url);
        if (cmd_args.filter_type == DOMAIN_FILTER_SAME) {
            if (strcmp(domain_page_url, domain_result_url) != 0) {
                return;
            }
        }
        if (cmd_args.filter_type == DOMAIN_FILTER_SUBDOMAIN) {
            // Check whether result_url is a subdomain of page_url, f.e.:
            // domain_result_url =  terms.archlinux.org
            // domain_page_url =          archlinux.org
            // Then first is a subdomain of the second
            if (!ends_with(domain_result_url, domain_page_url)) {
                return;
            }
        }
    }

    // Finally print URL to stdout
    printf("%s\n", result_url);

    // Crawl URLs recursively
    crawl_urls(result_url, depth_level - 1);
}

void debug_libcurl_version() {
    curl_version_info_data *ver = curl_version_info(CURLVERSION_NOW);
    printf("libcurl version %u.%u.%u\n",
        (ver->version_num >> 16) & 0xff,
        (ver->version_num >> 8) & 0xff,
        ver->version_num & 0xff);
}

// size_t write_data(void *buffer, size_t size, size_t nmemb, void *ctx);
size_t write_data(void *buffer, size_t size, size_t nmemb, void *ctx) {
    size_t realsize = size * nmemb;

    struct vec* v = (struct vec*)ctx;
    vec_append(v, (char *)buffer, realsize);

    return realsize;
}

void exit_args_error() {
    fprintf(stderr, "Usage: ./hrefs_crawler -a <addr> [-d <depth>] [-p <period>] [-t <timeout]\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "\t-a, --address <addr>\tURL the crawler starts from\n");
    fprintf(stderr, "\t-d, --depth <val>\tLevel of recursion a crawler will dive into hrefs (default: %d)\n", DEFAULT_DEPTH_LEVEL);
    fprintf(stderr, "\t-p, --period <val>\tPeriod between TCP requests (in miliseconds) (default: %d)\n", DEFAULT_REQUEST_PERIOD);
    fprintf(stderr, "\t-t, --timeout <val>\tTimeout for TCP requests (in miliseconds) (default: %d)\n", DEFAULT_REQUEST_TIMEOUT);
    fprintf(stderr, "\t--filter-same-domain\tOnly account for URLs with the same domain as the initial URL\n");
    fprintf(stderr, "\t--filter-subdomain\tOnly account for URLs to subdomains to the domain of the initial URL\n");
    exit(1);
}

void parse_cmd_args(int argc, char* argv[], struct cmd_args* args) {
    bool a_flag = false;
    bool d_flag = false;
    bool p_flag = false;
    bool t_flag = false;

    while (1) {
        static struct option long_options[] = {
            {"address",             required_argument, 0, 'a'},
            {"depth-level",         required_argument, 0, 'd'},
            {"request-period",      required_argument, 0, 'p'},
            {"request-timeout",     required_argument, 0, 't'},
            {"filter-same-domain",  no_argument, 0, 0},
            {"filter-subdomain",    no_argument, 0, 0},
            {0, 0, 0, 0}
        };

        int longindex;
        int c = getopt_long(argc, argv, "a:d:p:t:", long_options, &longindex);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 0:
                if (strcmp(long_options[longindex].name, "filter-same-domain") == 0) {
                    args->filter_type = DOMAIN_FILTER_SAME;
                }
                if (strcmp(long_options[longindex].name, "filter-subdomain") == 0) {
                    args->filter_type = DOMAIN_FILTER_SUBDOMAIN;
                }
                break;
            case 'a':
                if (a_flag) {
                    exit_args_error();
                }
                a_flag = true;
                args->root_url = optarg;
                break;
            case 'd':
                if (d_flag) {
                    exit_args_error();
                }
                if (!is_number(optarg)) {
                    fprintf(stderr, "Error: argument is not a number: %s\n", optarg);
                    exit(1);
                }
                d_flag = true;
                args->depth_level = atoi(optarg);
                break;
            case 'p':
                if (p_flag) {
                    exit_args_error();
                }
                if (!is_number(optarg)) {
                    fprintf(stderr, "Error: argument is not a number: %s\n", optarg);
                    exit_args_error();
                }
                p_flag = true;
                args->request_period = atoi(optarg);
                break;
            case 't':
                if (t_flag) {
                    exit_args_error();
                }
                if (!is_number(optarg)) {
                    fprintf(stderr, "Error: argument is not a number: %s\n", optarg);
                    exit_args_error();
                }
                t_flag = true;
                args->request_timeout = atoi(optarg);
                break;
            case '?':
                exit_args_error();
            default:
                printf("==== default\n");
        }
    }

    // Assert required arguments
    if (!a_flag) {
        fprintf(stderr, "Address argument is required\n");
        exit_args_error();
    }
}

int main(int argc, char* argv[]) {
    parse_cmd_args(argc, argv, &cmd_args);

    // debug_libcurl_version();
    curl_global_init(CURL_GLOBAL_NOTHING);

    crawl_urls(strdup(cmd_args.root_url), cmd_args.depth_level);

    curl_global_cleanup();
    return 0;
}

void crawl_urls(char* url, int depth_level) {
    /*
     * The algorithm is:
     * - Retrieve requested URL
     * - Traverse HTML to found http hrefs
     * - Recursively crawl found URLs by a depth of @depth_level
     *
     * A caller should pass ownership of @url, i.e. @url is freed at the end of current function.
     */
    if (depth_level == 0) {
        free(url);
        return;
    }

    usleep(cmd_args.request_period * 1000); 

    // Add '/' at end if missing
    int url_len = strlen(url);
    if (url[url_len-1] != '/') {
        url = (char* )realloc(url, url_len + 2);
        url[url_len] = '/';
        url[url_len+1] = '\0';
    }

    CURL *handle = curl_easy_init();
    // === SETUP ===
    // curl_easy_setopt(handle, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, cmd_args.request_timeout);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);
    // No larger than 10MB
    curl_easy_setopt(handle, CURLOPT_MAXFILESIZE_LARGE, (curl_off_t)10 * 1024 * 1024);

    curl_easy_setopt(handle, CURLOPT_URL, url);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);
    struct vec* html_data = vec_init(0);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, html_data);

    CURLcode success = curl_easy_perform(handle);
    if (success != CURLE_OK) {
        fprintf(stderr, "=== Error requesting page\n");
        curl_easy_cleanup(handle);
        free(url);
        return;
    }

    // Replace url with an effective retrieved by curl.
    char* tmp_url;
    curl_easy_getinfo(handle, CURLINFO_EFFECTIVE_URL, &tmp_url);
    free(url);
    url = strdup(tmp_url);

    capture_hrefs_from_html(html_data, url, depth_level, handle_found_url_cb);

    curl_easy_cleanup(handle);
    vec_deinit(html_data);
    free(url);
}
