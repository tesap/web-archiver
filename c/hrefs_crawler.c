#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <curl/curl.h>

#define REQUEST_PERIOD 50000

struct vec {
    size_t size;
    char* ptr;
};

struct vec* vec_init(size_t newsize) {
    struct vec* v = malloc(sizeof(struct vec));
    v->ptr = (char*)malloc(sizeof(char) * newsize);
    v->size = newsize;
    return v;
}

struct vec* vec_deinit(struct vec* v) {
    if (v && v->ptr) {
        free(v->ptr);
    }
    if (v) {
        free(v);
    }
}

void vec_append(struct vec* v, char* buffer, size_t newsize) {
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
    v->ptr = ptr;

    memcpy((v->ptr + v->size), buffer, newsize);
    v->size += newsize;
}

typedef enum {
    STATE_DEFAULT,
    STATE_PROGRESS,
    // STATE_CAPTURE,
} HrefParserState;

void parse_html(const char* url, int depth_level, struct vec* data, void(*callback)(const char*, int, char*)) {
    /*
     * The function effectively searches for links in HTML document.
     * It uses simple per-byte parser which searches for the following match
     * and captures content inside quotes:
     *      '<a href="<CAPTURE>"'
     * 
     * It calls a @callback function on each captured URL
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
                            // state = STATE_CAPTURE;
                        } else if (el == ' ' || el == '\n') {
                        } else {
                            state = STATE_DEFAULT;
                            progress_step = 0;
                        }
                        break;
                    case 5: // STATE_CAPTURE
                        if (el == '\"') {
                            vec_append(capture_vec, "\0", 1);
                            (*callback)(url, depth_level, capture_vec->ptr);
                            free(capture_vec);
                            // vec_deinit(capture_vec);
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
                
            default:
                // Skip, go on
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

void handle_found_url_cb(const char* url, int depth_level, char* found_url) {
    if (is_url_http(found_url)) {
        // Print to STDOUT
        printf("%s\n", found_url);
        crawl_urls(strdup(found_url), depth_level - 1);
    } else if (is_url_relative(found_url)) {
        char* stripped_found_url = found_url + 1;
        int full_url_len = strlen(url) + strlen(stripped_found_url) + 1;
        char* full_url = malloc(full_url_len * sizeof(char));
        
        sprintf(full_url, "%s%s", url, stripped_found_url);
        // Print to STDOUT
        printf("%s\n", full_url);
        crawl_urls(full_url, depth_level - 1);
        // free(full_url);
    }

    free(found_url);
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
    vec_append(v, buffer, realsize);

    return realsize;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "=== Example: ./hrefs_crawler archlinux.org 3\n");
        return 1;
    }
    char* url = strdup(argv[1]);
    const char* errstr;
    int depth_level = atoi(argv[2]);

    debug_libcurl_version();
    curl_global_init(CURL_GLOBAL_NOTHING);

    crawl_urls(url, depth_level);

    curl_global_cleanup();
    return 0;
}

void crawl_urls(char* url, int depth_level) {
    usleep(REQUEST_PERIOD); 

    if (depth_level == 0) {
        return;
    }

    int url_len = strlen(url);
    if (url[url_len-1] != '/') {
        url = realloc(url, url_len + 2);
        url[url_len] = '/';
        url[url_len+1] = '\0';
    }

    CURL *handle = curl_easy_init();
    // === SETUP ===
    // curl_easy_setopt(handle, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(handle, CURLOPT_TIMEOUT, 5L);
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
        return;
    }

    // long res_status;
    // curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_status);

    // Replace url with an effective retrieved by curl.
    char* tmp_url;
    curl_easy_getinfo(handle, CURLINFO_EFFECTIVE_URL, &tmp_url);
    free(url);
    url = strdup(tmp_url);

    parse_html(url, depth_level, html_data, handle_found_url_cb);

    curl_easy_cleanup(handle);
    vec_deinit(html_data);
    free(url);
}
