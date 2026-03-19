
#include "./network.h"
#include "./util.h"
#include "./url_parser.h"

#include <openssl/ssl.h>
#include <openssl/err.h>

#define BUFFER_SIZE 64

void parse_http_stream_chunk(const char* recv_buff, int size, struct vec* headers_vec, struct vec* content_vec, bool* content_started) {
    char border_window[6];
    memcpy(border_window, headers_vec->ptr + headers_vec->size - 3, 3);
    memcpy(border_window + 3, recv_buff, 3);
    int i = 0;
    bool found = false;
    while (i < 6) {
        if (strncmp(border_window + i, "\r\n\r\n", 4) == 0) {
            found = true;
            break;
        }
        i++;
    }

    if (found) {
        headers_vec->size -= 3 - i;

        int content_offset_in_buff = i + 4 - 3;
        *content_started = true;
        vec_append(
            content_vec,
            recv_buff + content_offset_in_buff,
            size - content_offset_in_buff
        );
        return;
    }
    
    char* pattern_ptr = (char*)memmem(recv_buff, size, "\r\n\r\n", 4);
    if (pattern_ptr != NULL) {
        size_t header_size_in_buff = pattern_ptr - recv_buff;
        vec_append(
            headers_vec,
            recv_buff,
            header_size_in_buff
        );

        size_t content_offset_in_buff = header_size_in_buff + 4;
        *content_started = true;
        vec_append(
            content_vec,
            recv_buff + content_offset_in_buff,
            size - content_offset_in_buff
        );
        return;
    }

    if (*content_started) {
        vec_append(content_vec, recv_buff, size);
    } else {
        vec_append(headers_vec, recv_buff, size);
    }
}

int get_location_header(const char* headers_buff, const char* request_url, char* result) {
    const char* header_start = strstr(headers_buff, "Location: ");
    if (!header_start) {
        header_start = strstr(headers_buff, "location: ");
        if (!header_start) {
            fprintf(stderr, "=== Error searching for 'Location: ' pattern\n");
            return -1;
        }
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

