
#include "./network.h"
#include "./util.h"
#include "./url_parser.h"

#include <openssl/ssl.h>
#include <openssl/err.h>

#define BUFFER_SIZE 64

const char* get_content_start(const char* response) {
    const char* match = strstr(response, "\r\n\r\n");
    if (!match) {
        return NULL;
    }
    
    return match + 4;
}

int get_location_header(const char* response, const char* request_url, char* result) {
    const char* header_start = strstr(response, "Location: ");
    if (!header_start) {
        fprintf(stderr, "=== Error searching for 'Location: ' pattern\n");
        return -1;
    }

    const char* value_start = header_start + 10;
    size_t value_len = strlen_with_delims(value_start);

    if (is_url_relative(value_start)) {
        struct UrlPtrs ptrs = get_url_pointers(request_url);
        // Copy part of URL without path
        int len = ptrs.host_end - request_url;
        memcpy(result, request_url, len);
        // Append redirection relative path 
        memcpy(result + len, value_start, value_len);
        result[len + value_len] = '\0';
    } else {
        memcpy(result, value_start, value_len);
        result[value_len] = '\0';
    }
    return 0;
}

