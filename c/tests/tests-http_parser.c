#include "./tests-common.h"
#include "../http_parser.h"
#include "../util.h"

#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h> 
#include <unistd.h>


#define TEST_PARSE_HTTP_CHUNKS(name, path, headers_end_expected, n1, content_start_expected, n2) \
    TEST(http_parser_##name) { \
        for (int BUFFER_SIZE = 4; BUFFER_SIZE <= 512; BUFFER_SIZE++) {\
            struct vec* headers_vec = vec_init(0); \
            struct vec* content_vec = vec_init(0); \
            bool content_started = false; \
            \
            int fd = open(path, O_RDONLY); \
            ASSERT_EQUAL_INT(fd != -1, true); \
            \
            char buff[BUFFER_SIZE]; \
            int bytes_read; \
            while ((bytes_read = read(fd, buff, BUFFER_SIZE)) != 0) { \
                parse_http_stream_chunk(buff, bytes_read, headers_vec, content_vec, &content_started); \
            } \
            ASSERT_EQUAL_STRN(headers_vec->ptr + headers_vec->size - n1, headers_end_expected, n1); \
            ASSERT_EQUAL_STRN(content_vec->ptr, content_start_expected, n2); \
            ASSERT_EQUAL_INT(content_started, true); \
            close(fd); \
            vec_deinit(headers_vec); \
            vec_deinit(content_vec); \
        } \
    } \

#define TEST_HTTP_PARSER_GET_LOCATION_HEADER_FILE(name, path, url, e1) \
    TEST(http_parser_##name) { \
        char* buff; \
        read_file(path, &buff); \
        char result[256]; \
        int ok = get_location_header(buff, url, result); \
        ASSERT_EQUAL_INT(ok, 0); \
        ASSERT_EQUAL_STR(result, e1); \
        free(buff); \
    } \

TEST_PARSE_HTTP_CHUNKS(3, "./out/files/archlinux.org.http", ".org/", 5, "<html>", 6);
TEST_PARSE_HTTP_CHUNKS(4, "./out/files/wiki.archlinux.org.http", "load", 4, "", 1);
TEST_PARSE_HTTP_CHUNKS(5, "./out/files/rebuildworld.net.http", "text/html", 9, "<?xml ", 6);
TEST_PARSE_HTTP_CHUNKS(6, "./out/files/archlinux.org-mirrors.http", "MISS", 4, "<!DOCT", 6);
TEST_HTTP_PARSER_GET_LOCATION_HEADER_FILE(7, "./out/files/wiki.archlinux.org.http", "wiki.archlinux.org", "https://wiki.archlinux.org/title/Main_page");
TEST_HTTP_PARSER_GET_LOCATION_HEADER_FILE(8, "./out/files/archlinux.org.http", "archlinux.org", "https://archlinux.org/");
TEST_HTTP_PARSER_GET_LOCATION_HEADER_FILE(9, "./out/files/lists.archlinux.org.http", "https://lists.archlinux.org", "https://lists.archlinux.org/mailman3/lists/");

TEST(http_parser_10) {

    struct vec* headers_vec = vec_init(0);
    struct vec* content_vec = vec_init(0);
    bool content_started = false;

    char* buff = "abcdef";
    parse_http_stream_chunk(buff, strlen(buff), headers_vec, content_vec, &content_started);
    buff = "ghijkl";
    parse_http_stream_chunk(buff, strlen(buff), headers_vec, content_vec, &content_started);

    ASSERT_EQUAL_STRN(headers_vec->ptr, "abcdefghijkl", headers_vec->size);
    ASSERT_EQUAL_STRN(content_vec->ptr, "", content_vec->size);
    ASSERT_EQUAL_INT(content_started, false);

    buff = "\r\n\r\n12345678";
    parse_http_stream_chunk(buff, strlen(buff), headers_vec, content_vec, &content_started);

    ASSERT_EQUAL_STRN(headers_vec->ptr, "abcdefghijkl", headers_vec->size);
    ASSERT_EQUAL_STRN(content_vec->ptr, "12345678", content_vec->size);
    ASSERT_EQUAL_INT(content_started, true);
}

TEST(http_parser_11) {

    struct vec* headers_vec = vec_init(0);
    struct vec* content_vec = vec_init(0);
    bool content_started = false;

    char* buff = "abcde\r\n\r\n";
    parse_http_stream_chunk(buff, strlen(buff), headers_vec, content_vec, &content_started);
    buff = "123";
    parse_http_stream_chunk(buff, strlen(buff), headers_vec, content_vec, &content_started);

    ASSERT_EQUAL_STRN(headers_vec->ptr, "abcde", headers_vec->size);
    ASSERT_EQUAL_STRN(content_vec->ptr, "123", content_vec->size);
    ASSERT_EQUAL_INT(content_started, true);
}

TEST(http_parser_12) {

    struct vec* headers_vec = vec_init(0);
    struct vec* content_vec = vec_init(0);
    bool content_started = false;

    char* buff = "abc\r\n";
    parse_http_stream_chunk(buff, strlen(buff), headers_vec, content_vec, &content_started);

    ASSERT_EQUAL_STRN(headers_vec->ptr, "abc\r\n", headers_vec->size);
    ASSERT_EQUAL_STRN(content_vec->ptr, "", content_vec->size);
    ASSERT_EQUAL_INT(content_started, false);

    buff = "\r\n123";
    parse_http_stream_chunk(buff, strlen(buff), headers_vec, content_vec, &content_started);

    ASSERT_EQUAL_STRN(headers_vec->ptr, "abc", headers_vec->size);
    ASSERT_EQUAL_STRN(content_vec->ptr, "123", content_vec->size);
    ASSERT_EQUAL_INT(content_started, true);
}

TEST(http_parser_13) {

    struct vec* headers_vec = vec_init(0);
    struct vec* content_vec = vec_init(0);
    bool content_started = false;

    char* buff = "abcdef\r";
    parse_http_stream_chunk(buff, strlen(buff), headers_vec, content_vec, &content_started);

    ASSERT_EQUAL_STRN(headers_vec->ptr, "abcdef\r", headers_vec->size);
    ASSERT_EQUAL_STRN(content_vec->ptr, "", content_vec->size);
    ASSERT_EQUAL_INT(content_started, false);

    buff = "\n\r\n123";
    parse_http_stream_chunk(buff, strlen(buff), headers_vec, content_vec, &content_started);

    ASSERT_EQUAL_STRN(headers_vec->ptr, "abcdef", headers_vec->size);
    ASSERT_EQUAL_STRN(content_vec->ptr, "123", content_vec->size);
    ASSERT_EQUAL_INT(content_started, true);
}

TEST(http_parser_14) {
    struct vec* headers_vec = vec_init(0);
    struct vec* content_vec = vec_init(0);
    bool content_started = false;

    char* buff = "abcde\r\n\r";
    parse_http_stream_chunk(buff, strlen(buff), headers_vec, content_vec, &content_started);
    buff = "\n123";
    parse_http_stream_chunk(buff, strlen(buff), headers_vec, content_vec, &content_started);

    ASSERT_EQUAL_STRN(headers_vec->ptr, "abcde", headers_vec->size);
    ASSERT_EQUAL_STRN(content_vec->ptr, "123", content_vec->size);
    ASSERT_EQUAL_INT(content_started, true);
}

int main() {
    RUN_TEST(http_parser_3);
    RUN_TEST(http_parser_4);
    RUN_TEST(http_parser_5);
    RUN_TEST(http_parser_6);
    RUN_TEST(http_parser_7);
    RUN_TEST(http_parser_8);
    RUN_TEST(http_parser_9);
    RUN_TEST(http_parser_10);
    RUN_TEST(http_parser_11);
    RUN_TEST(http_parser_12);
    RUN_TEST(http_parser_13);
    RUN_TEST(http_parser_14);
}
