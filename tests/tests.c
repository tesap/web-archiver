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
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>

#include "tests_common.h"
#include "util.h"
#include "hrefs_crawler.h"

/*
 * TODO tests for functions to add:
 * - vec_eq, vec_eq2, vec_to_cstr 
 * - struct vec on heap
 * - is_number, is_alphabet, starts_with, ends_with, strlen_with_delims
 * - file_size, file_mtime, file_exists
 * - rstrip
 * - url_dirname_len
 *
 * - download_http, cached_download_http
 * - parse_html_tag
 */

#define TEST_vec_append(name, s1, s2, size, res_exp) \
    TEST(vec_append__##name) { \
        char _mem[size]; \
        memcpy(_mem, s1, strlen(s1)); \
        struct vec v = {_mem, strlen(s1)}; \
        vec_append(&v, false, vec_wrap(s2)); \
        ASSERT_EQUAL_VEC(v, vec_wrap(res_exp)); \
    } \
    TEST(vec_append_cstring__##name) { \
        char _mem[size]; \
        memcpy(_mem, s1, strlen(s1)); \
        struct vec v = {_mem, strlen(s1)}; \
        vec_append_cstring(&v, false, s2); \
        ASSERT_EQUAL_VEC(v, vec_wrap(res_exp)); \
    } \

TEST_vec_append(1, "1234", "abcd", 8, "1234abcd");

#define TEST_vec_terminate(name, s, size_) \
    TEST(vec_terminate__##name) { \
        char _mem[size_]; \
        memset(_mem, 1, size_); \
        memcpy(_mem, s, strlen(s)); \
        struct vec v = {_mem, strlen(s)}; \
        \
        ASSERT_EQUAL_INT(v.ptr[v.size] != '\0', true); \
        vec_terminate(&v); \
        ASSERT_EQUAL_VEC(v, vec_wrap(s)); \
        ASSERT_EQUAL_INT(strlen(v.ptr), strlen(s)); \
        ASSERT_EQUAL_INT(v.ptr[v.size] == '\0', true); \
    } \

TEST_vec_terminate(1, "1234abcd", 9);

#define TEST_lstrip(name, s, suff, res_expected) \
    TEST(lstrip__##name) { \
        struct vec v = vec_wrap(s); \
        lstrip(&v, vec_wrap(suff)); \
        ASSERT_EQUAL_VEC(v, vec_wrap(res_expected)); \
    } \

TEST_lstrip(1, "./archlinux.org/bla", "./", "archlinux.org/bla");


void link_type_reverse(LinkType lt, struct HtmlTag* out) {
    switch (lt) {
        case LINK_TYPE_HTML:
            strcpy(out->tag_name, "a");
            return;
        case LINK_TYPE_IMG:
            strcpy(out->tag_name, "img");
            return;
        case LINK_TYPE_SCRIPT:
            strcpy(out->tag_name, "script");
            return;
        case LINK_TYPE_STYLE:
            out->type = vec_wrap("text/css");
            return;
        default:
            break;
    }
}

#define TEST_is_url_represents_file(name, url, type_hint, res_expected) \
    TEST(is_url_represents_file__##name) { \
        bool res = is_url_represents_file(vec_wrap(url), type_hint); \
        ASSERT_EQUAL_INT(res, res_expected); \
    } \

TEST_is_url_represents_file(1, "https://wiki.archlinux.org/index.html", LINK_TYPE_HTML, true);
TEST_is_url_represents_file(2, "https://doc.rust-lang.org/stable/rustc/index.html", LINK_TYPE_HTML, true);
TEST_is_url_represents_file(3, "https://doc.rust-lang.org/stable/rustc/", LINK_TYPE_HTML, false);
TEST_is_url_represents_file(4, "https://doc.rust-lang.org/stable/rustc/", LINK_TYPE_HTML, false);
TEST_is_url_represents_file(5, "https://archlinux.org/static/archlinux_common_style/navbar.css", LINK_TYPE_STYLE, true);
TEST_is_url_represents_file(6, "https://archlinux.org/static/rss.svg", LINK_TYPE_IMG, true);

#define TEST_normalize(name, path, res_expected) \
    TEST(normalize__##name) { \
        char _v[1024]; \
        struct vec v = {_v, 0}; \
        normalize(vec_wrap(path), &v); \
        ASSERT_EQUAL_VEC(v, vec_wrap(res_expected)); \
    } \

TEST_normalize(1, "doc.rust-lang.org/stable/rustc/../cargo/index.html", "doc.rust-lang.org/stable/cargo/index.html");
TEST_normalize(2, "archlinux.org/", "archlinux.org/");
TEST_normalize(3, "/", "/");

#define TEST_link_to_full_url(name, link, page_url, res_expected) \
    TEST(link_to_full_url__##name) { \
        char _v[1024]; \
        struct vec v = {_v, 0}; \
        link_to_full_url(vec_wrap(link), vec_wrap(page_url), &v); \
        ASSERT_EQUAL_VEC(v, vec_wrap(res_expected)); \
    } \

TEST_link_to_full_url(1, "/static/archlinux_common_style/favicon.png", "./archlinux.org/index.html", "archlinux.org/static/archlinux_common_style/favicon.png");
TEST_link_to_full_url(2, "https://devblog.archlinux.page", "./archlinux.org/index.html", "https://devblog.archlinux.page");
TEST_link_to_full_url(3, "/packages/extra/any/aarch64-linux-gnu-glibc/", "./archlinux.org/packages/", "archlinux.org/packages/extra/any/aarch64-linux-gnu-glibc/");
TEST_link_to_full_url(4, "https://archlinux.org/packages/?sort=-last_update", "./archlinux.org/index.html", "https://archlinux.org/packages/?sort=-last_update");
TEST_link_to_full_url(5, "https://wiki.archlinux.org/title/Arch_Linux_press_coverage", "./archlinux.org/index.html", "https://wiki.archlinux.org/title/Arch_Linux_press_coverage");
TEST_link_to_full_url(6, "../cargo/index.html", "https://doc.rust-lang.org/stable/rustc/index.html", "https://doc.rust-lang.org/stable/cargo/index.html");
TEST_link_to_full_url(7, "/", "./archlinux.org/index.html", "archlinux.org/");

#define TEST_url_save_path(name, link_type, url, res_expected) \
    TEST(url_save_path__##name) { \
        char _v[1024]; \
        struct vec v = {_v, 0}; \
        url_save_path(vec_wrap(url), link_type, &v); \
        ASSERT_EQUAL_VEC(v, vec_wrap(res_expected)); \
    } \


TEST_url_save_path(1, LINK_TYPE_HTML, "https://wiki.archlinux.org/title/Arch_Linux_press_coverage", "wiki.archlinux.org/title/Arch_Linux_press_coverage/index.html");
TEST_url_save_path(2, LINK_TYPE_HTML, "https://wiki.archlinux.org/title/Arch_Linux_press_coverage/", "wiki.archlinux.org/title/Arch_Linux_press_coverage/index.html");
TEST_url_save_path(3, LINK_TYPE_UNKNOWN, "https://archlinux.org/opensearch/packages/", "archlinux.org/opensearch/packages/index.html");
TEST_url_save_path(4, LINK_TYPE_UNKNOWN, "https://domain.org/opensearch/index.html", "domain.org/opensearch/index.html");


#define TEST_tag_link_type(name, tag_name, link, type, res_expected) \
    TEST(tag_link_type__##name) { \
        struct HtmlTag t = {tag_name, NULL, vec_wrap(link), vec_wrap(type)}; \
        ASSERT_EQUAL_INT(tag_link_type(&t), res_expected); \
    } \

TEST_tag_link_type(0, "link", "/static/archlinux_common_style/navbar.css", "text/css", LINK_TYPE_STYLE);
TEST_tag_link_type(1, "link", "/static/archweb.css", "text/css", LINK_TYPE_STYLE);
TEST_tag_link_type(2, "link", "/static/archweb.css", "", LINK_TYPE_STYLE);
TEST_tag_link_type(3, "link", "/static/archlinux_common_style/favicon.png", "image/png", LINK_TYPE_IMG);
TEST_tag_link_type(4, "link", "/static/archlinux_common_style/apple-touch-icon-144x144.png", "", LINK_TYPE_IMG);
TEST_tag_link_type(5, "link", "/static/pic.jpg", "", LINK_TYPE_IMG);
TEST_tag_link_type(6, "link", "/static/pic.jpg", "image/jpg", LINK_TYPE_IMG);
TEST_tag_link_type(7, "link", "/static/pic.svg", "", LINK_TYPE_IMG);
TEST_tag_link_type(8, "link", "/opensearch/packages/", "application/opensearchdescription+xml", LINK_TYPE_UNKNOWN);
TEST_tag_link_type(9, "link", "/feeds/news/", "application/rss+xml", LINK_TYPE_UNKNOWN);
TEST_tag_link_type(10, "link", "/feeds/packages/", "application/rss+xml", LINK_TYPE_UNKNOWN);
TEST_tag_link_type(11, "script", "/static/homepage.js", "text/javascript", LINK_TYPE_SCRIPT);
TEST_tag_link_type(12, "link", "/static/homepage.js", "text/javascript", LINK_TYPE_SCRIPT);
TEST_tag_link_type(13, "link", "/static/homepage.js", "", LINK_TYPE_SCRIPT);
TEST_tag_link_type(14, "a", "/", "", LINK_TYPE_HTML);
TEST_tag_link_type(15, "a", "/feeds/news/", "", LINK_TYPE_HTML);
TEST_tag_link_type(16, "img", "/static/rss.svg", "", LINK_TYPE_IMG);
TEST_tag_link_type(17, "a", "/news/kea-1303-6-update-requires-manual-intervention/", "", LINK_TYPE_HTML);
TEST_tag_link_type(18, "img", "/static/icons8_logo.png", "", LINK_TYPE_IMG);

#define TEST_iter_html_tags(name, buff, cmp_list, cmp_list_size) \
    TEST(iter_html_tags__##name) { \
        int cmp_index = 0; \
        struct TestCallbackCtx ctx = { \
            &cmp_index, \
            cmp_list, \
            cmp_list_size \
        }; \
        iter_html_tags(vec_wrap(buff), iter_html_tags_cb, &ctx); \
        ASSERT_EQUAL_INT(cmp_index, cmp_list_size); \
    } \

#define TEST_FILE_iter_html_tags(name, filepath, cmp_list, cmp_list_size) \
    TEST(iter_html_tags__##name) { \
        char* buff; \
        int size = read_file(filepath, &buff); \
        ASSERT_EQUAL_INT(size > 0, true); \
        int cmp_index = 0; \
        struct TestCallbackCtx ctx = { \
            &cmp_index, \
            cmp_list, \
            cmp_list_size \
        }; \
        iter_html_tags((struct vec){ buff, size }, iter_html_tags_cb, &ctx); \
    } \

struct CmpEntry {
    char* s1;
    char* s2;
    char* s3;
};

struct TestCallbackCtx {
    int* cmp_index;
    struct CmpEntry* cmp_list;
    int cmp_list_size;
};

void iter_html_tags_cb(struct HtmlTag* t, const void* ctx, FILE* fout) {
    struct TestCallbackCtx* ctx_struct = (struct TestCallbackCtx*)ctx;
    int* cmp_index = ctx_struct->cmp_index;
    struct CmpEntry* cmp_list = ctx_struct->cmp_list;
    int cmp_list_size = ctx_struct->cmp_list_size;

    // printf("=== (%d) URL: %.*s\n", *cmp_index, t->link.size, t->link.ptr);

    ASSERT_EQUAL_STR(t->tag_name, cmp_list[*cmp_index].s1);
    ASSERT_EQUAL_STRN(t->link.ptr, cmp_list[*cmp_index].s2, t->link.size);
    ASSERT_EQUAL_STRN(t->type.ptr, cmp_list[*cmp_index].s3, t->type.size);
    ASSERT_EQUAL_STRN(t->tag_end, ">", 1);
    (*cmp_index)++;
}

struct CmpEntry cmp_list5[] = {
    { "a", "HREF1", NULL },
    { "a", "HREF2", NULL },
};
int cmp_list_size5 = sizeof(cmp_list5) / sizeof(struct CmpEntry);

TEST_iter_html_tags(5, "<a href=\"HREF1\"/><a href=\"HREF2\"/>", cmp_list5, cmp_list_size5);

struct CmpEntry cmp_list6[] = {
    { "link", "/static/archlinux_common_style/navbar.css", "text/css" },
    { "link", "/static/archweb.css", "text/css" },
    { "link", "/static/archlinux_common_style/favicon.png", "image/png" },
    { "link", "/static/archlinux_common_style/favicon.png", "image/png" },
    { "link", "/static/archlinux_common_style/apple-touch-icon-57x57.png", NULL },
    { "link", "/static/archlinux_common_style/apple-touch-icon-72x72.png", NULL },
    { "link", "/static/archlinux_common_style/apple-touch-icon-114x114.png", NULL },
    { "link", "/static/archlinux_common_style/apple-touch-icon-144x144.png", NULL },
    { "link", "/opensearch/packages/", "application/opensearchdescription+xml" },
    { "link", "/feeds/news/", "application/rss+xml" },
    { "link", "/feeds/packages/", "application/rss+xml" },
    { "script", "/static/homepage.js", "text/javascript" },
    { "link", "https://fosstodon.org/@archlinux", NULL },
    { "a", "/", NULL },
    { "a", "/", NULL },
    { "a", "/packages/", NULL },
    { "a", "https://bbs.archlinux.org/", NULL },
    { "a", "https://wiki.archlinux.org/", NULL },
    { "a", "https://gitlab.archlinux.org/archlinux", NULL },
    { "a", "https://security.archlinux.org/", NULL },
    { "a", "https://aur.archlinux.org/", NULL },
    { "a", "/download/", NULL },
    { "a", "https://aur.archlinux.org/", NULL },
    { "a", "https://bbs.archlinux.org/", NULL },
    { "a", "https://lists.archlinux.org/", NULL },
    { "a", "https://wiki.archlinux.org/", NULL },
    { "a", "/about/", NULL },
    { "a", "/news/", NULL },
    { "a", "/feeds/news/", NULL },
    { "img", "/static/rss.svg", NULL },
    { "a", "/news/nvidia-590-driver-drops-pascal-support-main-packages-switch-to-open-kernel-modules/", NULL },
    { "a", "/news/net-packages-may-require-manual-intervention/", NULL },
    { "a", "/news/waydroid-154-3-update-may-require-manual-intervention/", NULL },
    { "a", "/news/dovecot-24-requires-manual-intervention/", NULL },
    { "a", "https://doc.dovecot.org/latest/installation/upgrade/2.3-to-2.4.html", NULL },
    { "a", "/news/recent-services-outages/", NULL },
    { "a", "https://lists.archlinux.org/archives/list/arch-general@lists.archlinux.org/thread/EU4NXRX6DDJAACOWIRZNU4S5KVXEUI72/", NULL },
    { "a", "/news/", NULL },
    { "a", "/news/zabbix-741-2-may-requires-manual-intervention/", NULL },
    { "a", "/news/linux-firmware-2025061312fe085f-5-upgrade-requires-manual-intervention/", NULL },
    { "a", "/news/plasma-640-will-need-manual-intervention-if-you-are-on-x11/", NULL },
    { "a", "/news/transition-to-the-new-wow64-wine-and-wine-staging/", NULL },
    { "a", "/news/valkey-to-replace-redis-in-the-extra-repository/", NULL },
    { "a", "/news/cleaning-up-old-repositories/", NULL },
    { "a", "/news/glibc-241-corrupting-discord-installation/", NULL },
    { "a", "/news/critical-rsync-security-release-340/", NULL },
    { "a", "/news/providing-a-license-for-package-sources/", NULL },
    { "a", "/news/manual-intervention-for-pacman-700-and-local-repositories-required/", NULL },
    { "a", "/packages/?sort=-last_update", NULL },
    { "a", "/feeds/packages/", NULL },
    { "img", "/static/rss.svg", NULL },
    { "a", "/packages/extra/x86_64/brltty/", NULL },
    { "a", "/packages/extra/x86_64/ultramaster-kr106/", NULL },
    { "a", "/packages/extra/x86_64/gotosocial/", NULL },
    { "a", "/packages/extra/any/ruby-maxmind-db/", NULL },
    { "a", "/packages/extra/any/python-schemdraw/", NULL },
    { "a", "/packages/extra/any/python-distributed/", NULL },
    { "a", "/packages/extra/any/python-dask/", NULL },
    { "a", "/packages/extra/x86_64/epiphany/", NULL },
    { "a", "/packages/extra/x86_64/kitty/", NULL },
    { "a", "/packages/core-testing/any/autoconf/", NULL },
    { "a", "/packages/extra/x86_64/abiword/", NULL },
    { "a", "/packages/extra/x86_64/linux-hardened/", NULL },
    { "a", "/packages/extra/x86_64/phosh-tour/", NULL },
    { "a", "/packages/extra/x86_64/plasma-camera/", NULL },
    { "a", "/packages/extra/x86_64/stopmotion/", NULL },
    { "a", "https://wiki.archlinux.org/", NULL },
    { "a", "https://man.archlinux.org/", NULL },
    { "a", "https://wiki.archlinux.org/title/Installation_guide", NULL },
    { "a", "https://lists.archlinux.org/", NULL },
    { "a", "https://wiki.archlinux.org/title/IRC_channels", NULL },
    { "a", "https://planet.archlinux.org/", NULL },
    { "a", "https://wiki.archlinux.org/title/International_communities", NULL },
    { "a", "/donate/", NULL },
    { "a", "https://www.freewear.org/?page=list_items&amp;org=Archlinux", NULL },
    { "a", "https://www.hellotux.com/arch", NULL },
    { "a", "/mirrorlist/", NULL },
    { "a", "/mirrors/", NULL },
    { "a", "/mirrors/status/", NULL },
    { "a", "https://wiki.archlinux.org/title/Getting_involved", NULL },
    { "a", "https://devblog.archlinux.page", NULL },
    { "a", "https://gitlab.archlinux.org/archlinux/", NULL },
    { "a", "https://wiki.archlinux.org/title/DeveloperWiki", NULL },
    { "a", "/groups/", NULL },
    { "a", "/todo/", NULL },
    { "a", "/releng/releases/", NULL },
    { "a", "/visualize/", NULL },
    { "a", "/packages/differences/", NULL },
    { "a", "/people/developers/", NULL },
    { "a", "/people/package-maintainers/", NULL },
    { "a", "/people/support-staff/", NULL },
    { "a", "/people/developer-fellows/", NULL },
    { "a", "/people/package-maintainer-fellows/", NULL },
    { "a", "/people/support-staff-fellows/", NULL },
    { "a", "/master-keys/", NULL },
    { "a", "https://wiki.archlinux.org/title/Arch_Linux_press_coverage", NULL },
    { "a", "/art/", NULL },
    { "a", "/news/", NULL },
    { "a", "/feeds/", NULL },
    { "a", "https://co.clickandpledge.com/Default.aspx?WID=47294", NULL },
    { "img", "/static/click_and_pledge.png", NULL },
    { "a", "https://www.hetzner.com", NULL },
    { "img", "/static/hetzner_logo.png", NULL },
    { "a", "https://icons8.com/", NULL },
    { "img", "/static/icons8_logo.png", NULL },
    { "a", "mailto:jvinet@zeroflux.org", NULL },
    { "a", "mailto:aaron@archlinux.org", NULL },
    { "a", "mailto:anthraxx@archlinux.org", NULL },
    { "a", "https://terms.archlinux.org/docs/trademark-policy/", NULL },
};
int cmp_list_size6 = sizeof(cmp_list6) / sizeof(struct CmpEntry);

TEST_FILE_iter_html_tags(6, TESTS_FILES_PATH "/archlinux.org.html", cmp_list6, cmp_list_size6);

#define TEST_parse_http_stream_chunk(name, path, headers_end_expected, n1, content_start_expected, n2) \
    TEST(parse_http_stream_chunk__##name) { \
        for (int BUFFER_SIZE = 4; BUFFER_SIZE <= 512; BUFFER_SIZE++) {\
            struct vec headers_vec = vec_alloc(0); \
            struct vec content_vec = vec_alloc(0); \
            bool content_started = false; \
            \
            int fd = open(path, O_RDONLY); \
            ASSERT_EQUAL_INT(fd != -1, true); \
            \
            char buff[BUFFER_SIZE]; \
            int bytes_read; \
            while ((bytes_read = read(fd, buff, BUFFER_SIZE)) != 0) { \
                parse_http_stream_chunk(buff, bytes_read, &headers_vec, &content_vec, &content_started); \
            } \
            ASSERT_EQUAL_STRN(headers_vec.ptr + headers_vec.size - n1, headers_end_expected, n1); \
            ASSERT_EQUAL_STRN(content_vec.ptr, content_start_expected, n2); \
            ASSERT_EQUAL_INT(content_started, true); \
            close(fd); \
            vec_dealloc(headers_vec); \
            vec_dealloc(content_vec); \
        } \
    } \

#define TEST_FILE_get_location_header(name, path, url, e1) \
    TEST(get_location_header__##name) { \
        char* buff; \
        read_file(path, &buff); \
        char _mem[256]; \
        struct vec result = {_mem, 0}; \
        int ok = get_location_header(vec_wrap(buff), vec_wrap(url), &result); \
        if (strlen(e1) > 0) { \
            ASSERT_EQUAL_INT(ok, 0); \
            ASSERT_EQUAL_VEC(result, vec_wrap(e1)); \
        } else {\
            ASSERT_EQUAL_INT(ok, -1); \
        } \
        free(buff); \
    } \

TEST_parse_http_stream_chunk(3, TESTS_FILES_PATH "/archlinux.org.http", ".org/", 5, "<html>", 6);
TEST_parse_http_stream_chunk(4, TESTS_FILES_PATH "/wiki.archlinux.org.http", "load", 4, "", 1);
TEST_parse_http_stream_chunk(5, TESTS_FILES_PATH "/rebuildworld.net.http", "text/html", 9, "<?xml ", 6);
TEST_parse_http_stream_chunk(6, TESTS_FILES_PATH "/archlinux.org-mirrors.http", "MISS", 4, "<!DOCT", 6);
TEST_FILE_get_location_header(7, TESTS_FILES_PATH "/wiki.archlinux.org.http", "wiki.archlinux.org", "https://wiki.archlinux.org/title/Main_page");
TEST_FILE_get_location_header(8, TESTS_FILES_PATH "/archlinux.org.http", "archlinux.org", "https://archlinux.org/");
TEST_FILE_get_location_header(9, TESTS_FILES_PATH "/lists.archlinux.org.http", "https://lists.archlinux.org", "https://lists.archlinux.org/mailman3/lists/");
TEST_FILE_get_location_header(10, TESTS_FILES_PATH "/lists.archlinux.org.http.lack", "https://lists.archlinux.org", "");

TEST(http_parser_10) {

    struct vec headers_vec = vec_alloc(0);
    struct vec content_vec = vec_alloc(0);
    bool content_started = false;

    char* buff = "abcdef";
    parse_http_stream_chunk(buff, strlen(buff), &headers_vec, &content_vec, &content_started);
    buff = "ghijkl";
    parse_http_stream_chunk(buff, strlen(buff), &headers_vec, &content_vec, &content_started);

    ASSERT_EQUAL_STRN(headers_vec.ptr, "abcdefghijkl", headers_vec.size);
    ASSERT_EQUAL_STRN(content_vec.ptr, "", content_vec.size);
    ASSERT_EQUAL_INT(content_started, false);

    buff = "\r\n\r\n12345678";
    parse_http_stream_chunk(buff, strlen(buff), &headers_vec, &content_vec, &content_started);

    ASSERT_EQUAL_STRN(headers_vec.ptr, "abcdefghijkl", headers_vec.size);
    ASSERT_EQUAL_STRN(content_vec.ptr, "12345678", content_vec.size);
    ASSERT_EQUAL_INT(content_started, true);
}

TEST(http_parser_11) {

    struct vec headers_vec = vec_alloc(0);
    struct vec content_vec = vec_alloc(0);
    bool content_started = false;

    char* buff = "abcde\r\n\r\n";
    parse_http_stream_chunk(buff, strlen(buff), &headers_vec, &content_vec, &content_started);
    buff = "123";
    parse_http_stream_chunk(buff, strlen(buff), &headers_vec, &content_vec, &content_started);

    ASSERT_EQUAL_STRN(headers_vec.ptr, "abcde", headers_vec.size);
    ASSERT_EQUAL_STRN(content_vec.ptr, "123", content_vec.size);
    ASSERT_EQUAL_INT(content_started, true);
}

TEST(http_parser_12) {

    struct vec headers_vec = vec_alloc(0);
    struct vec content_vec = vec_alloc(0);
    bool content_started = false;

    char* buff = "abc\r\n";
    parse_http_stream_chunk(buff, strlen(buff), &headers_vec, &content_vec, &content_started);

    ASSERT_EQUAL_STRN(headers_vec.ptr, "abc\r\n", headers_vec.size);
    ASSERT_EQUAL_STRN(content_vec.ptr, "", content_vec.size);
    ASSERT_EQUAL_INT(content_started, false);

    buff = "\r\n123";
    parse_http_stream_chunk(buff, strlen(buff), &headers_vec, &content_vec, &content_started);

    ASSERT_EQUAL_STRN(headers_vec.ptr, "abc", headers_vec.size);
    ASSERT_EQUAL_STRN(content_vec.ptr, "123", content_vec.size);
    ASSERT_EQUAL_INT(content_started, true);
}

TEST(http_parser_13) {

    struct vec headers_vec = vec_alloc(0);
    struct vec content_vec = vec_alloc(0);
    bool content_started = false;

    char* buff = "abcdef\r";
    parse_http_stream_chunk(buff, strlen(buff), &headers_vec, &content_vec, &content_started);

    ASSERT_EQUAL_STRN(headers_vec.ptr, "abcdef\r", headers_vec.size);
    ASSERT_EQUAL_STRN(content_vec.ptr, "", content_vec.size);
    ASSERT_EQUAL_INT(content_started, false);

    buff = "\n\r\n123";
    parse_http_stream_chunk(buff, strlen(buff), &headers_vec, &content_vec, &content_started);

    ASSERT_EQUAL_STRN(headers_vec.ptr, "abcdef", headers_vec.size);
    ASSERT_EQUAL_STRN(content_vec.ptr, "123", content_vec.size);
    ASSERT_EQUAL_INT(content_started, true);
}

TEST(http_parser_14) {
    struct vec headers_vec = vec_alloc(0);
    struct vec content_vec = vec_alloc(0);
    bool content_started = false;

    char* buff = "abcde\r\n\r";
    parse_http_stream_chunk(buff, strlen(buff), &headers_vec, &content_vec, &content_started);
    buff = "\n123";
    parse_http_stream_chunk(buff, strlen(buff), &headers_vec, &content_vec, &content_started);

    ASSERT_EQUAL_STRN(headers_vec.ptr, "abcde", headers_vec.size);
    ASSERT_EQUAL_STRN(content_vec.ptr, "123", content_vec.size);
    ASSERT_EQUAL_INT(content_started, true);
}

#define TEST_url_parts(name, url, protocol_expected, host_expected, path_expected) \
    TEST(url_parts__##name) { \
        struct UrlParts parts = UrlParts_init(); \
        url_parts(vec_wrap(url), &parts); \
        ASSERT_EQUAL_VEC(parts.protocol, vec_wrap(protocol_expected)); \
        ASSERT_EQUAL_VEC(parts.host, vec_wrap(host_expected)); \
        ASSERT_EQUAL_VEC(parts.path, vec_wrap(path_expected)); \
    } \

#define TEST_only_host_path(name, url, path_expected) \
    TEST(only_host_path__##name) { \
        char _mem[MAX_URL_LENGTH]; \
        struct vec res = {_mem, 0}; \
        \
        only_host_path(vec_wrap(url), &res); \
        ASSERT_EQUAL_VEC(res, vec_wrap(path_expected)); \
    } \

#define TEST_relpath(name, path_from, path_to, expected_result) \
    TEST(relpath__##name) { \
        char _result[1024]; \
        struct vec result = {_result, 0}; \
        relpath(vec_wrap(path_from), vec_wrap(path_to), &result); \
        vec_terminate(&result); \
        ASSERT_EQUAL_STR(result.ptr, expected_result); \
    } \

#define TEST_dirname_len(name, input, expected_result) \
    TEST(dirname_len__##name) { \
        int res = dirname_len(vec_wrap(input)); \
        ASSERT_EQUAL_INT(res, expected_result); \
    } \

#define TEST_dir_count(name, input, expected_result) \
    TEST(dir_count__##name) { \
        int res = dir_count(vec_wrap(input)); \
        ASSERT_EQUAL_INT(res, expected_result); \
    } \

TEST(url_parser__1) {
    char* url = "https://archlinux.org/asdf/we32";
    struct UrlPtrs ptrs = url_pointers(vec_wrap(url));
    ASSERT_EQUAL_INT(ptrs.protocol_start, url);
    ASSERT_EQUAL_INT(ptrs.protocol_end, url + 8);
    ASSERT_EQUAL_INT(ptrs.host_start, url + 8);
    ASSERT_EQUAL_INT(ptrs.host_end, url + 21);
    ASSERT_EQUAL_INT(ptrs.path_start, url + 21);
    ASSERT_EQUAL_INT(ptrs.path_end, url + 31);
}

TEST(url_parser__2) {
    char* url = "https://archlinux.org/asdf/we32\n\r";
    struct UrlPtrs ptrs = url_pointers(vec_wrap(url));
    ASSERT_EQUAL_INT(ptrs.protocol_start, url);
    ASSERT_EQUAL_INT(ptrs.protocol_end, url + 8);
    ASSERT_EQUAL_INT(ptrs.host_start, url + 8);
    ASSERT_EQUAL_INT(ptrs.host_end, url + 21);
    ASSERT_EQUAL_INT(ptrs.path_start, url + 21);
    ASSERT_EQUAL_INT(ptrs.path_end, url + 31);
}

TEST(url_parser__3) {
    char* url = "https://archlinux.org/asdf/we32#ASDF";
    struct UrlPtrs ptrs = url_pointers(vec_wrap(url));
    ASSERT_EQUAL_INT(ptrs.protocol_start, url);
    ASSERT_EQUAL_INT(ptrs.protocol_end, url + 8);
    ASSERT_EQUAL_INT(ptrs.host_start, url + 8);
    ASSERT_EQUAL_INT(ptrs.host_end, url + 21);
    ASSERT_EQUAL_INT(ptrs.path_start, url + 21);
    ASSERT_EQUAL_INT(ptrs.path_end, url + 31);
}

TEST(url_parser__4) {
    char* url = "archlinux.org";
    struct UrlPtrs ptrs = url_pointers(vec_wrap(url));
    ASSERT_EQUAL_INT(ptrs.protocol_start, NULL);
    ASSERT_EQUAL_INT(ptrs.protocol_end, NULL);
    ASSERT_EQUAL_INT(ptrs.host_start, url);
    ASSERT_EQUAL_INT(ptrs.host_end, url + 13);
    ASSERT_EQUAL_INT(ptrs.path_start, NULL);
    ASSERT_EQUAL_INT(ptrs.path_end, NULL);
}

TEST(url_parser_5) {
    char* url = "archlinux.org/path/123";
    struct UrlPtrs ptrs = url_pointers(vec_wrap(url));
    ASSERT_EQUAL_INT(ptrs.protocol_start, NULL);
    ASSERT_EQUAL_INT(ptrs.protocol_end, NULL);
    ASSERT_EQUAL_INT(ptrs.host_start, url);
    ASSERT_EQUAL_INT(ptrs.host_end, url + 13);
    ASSERT_EQUAL_INT(ptrs.path_start, url + 13);
    ASSERT_EQUAL_INT(ptrs.path_end, url + 22);
}

TEST(url_parser__6) {
    char* url = "https://archlinux.org";
    struct UrlPtrs ptrs = url_pointers(vec_wrap(url));
    ASSERT_EQUAL_INT(ptrs.protocol_start, url);
    ASSERT_EQUAL_INT(ptrs.protocol_end, url + 8);
    ASSERT_EQUAL_INT(ptrs.host_start, url + 8);
    ASSERT_EQUAL_INT(ptrs.host_end, url + 21);
    ASSERT_EQUAL_INT(ptrs.path_start, NULL);
    ASSERT_EQUAL_INT(ptrs.path_end, NULL);
}


TEST(url_parser__7) {
    char* url = "/asdf/we32/";
    struct UrlPtrs ptrs = url_pointers(vec_wrap(url));
    ASSERT_EQUAL_INT(ptrs.protocol_start, NULL);
    ASSERT_EQUAL_INT(ptrs.protocol_end, NULL);
    ASSERT_EQUAL_INT(ptrs.host_start, NULL);
    ASSERT_EQUAL_INT(ptrs.host_end, NULL);
    ASSERT_EQUAL_INT(ptrs.path_start, url);
    ASSERT_EQUAL_INT(ptrs.path_end, url + 11);
}

TEST_url_parts(8, "/sdfsd/123/sdfsd/", "", "", "/sdfsd/123/sdfsd/");
TEST_url_parts(9, "/", "", "", "/");
TEST_url_parts(10, "https://archlinux.org", "https://", "archlinux.org", "");
TEST_url_parts(10_2, "https://archlinux.org/", "https://", "archlinux.org", "/");
TEST_url_parts(11, "http://archlinux.org/some/path/long#SDF", "http://", "archlinux.org", "/some/path/long");
TEST_url_parts(12, "http://sub2.sub-sdf.archlinux.org", "http://", "sub2.sub-sdf.archlinux.org", "");
TEST_url_parts(13, "http://archlinux.org#SDF", "http://", "archlinux.org", "");
TEST_url_parts(14, "archlinux.org?SDF", "", "archlinux.org", "");
// TODO Fix logic for port if needed
// TEST_url_parts(15, "0.0.0.0:1234/path/234", "", "0.0.0.0:1234", "/path/234");
TEST_url_parts(16, "http://archlinux.org/\n", "http://", "archlinux.org", "/");
TEST_url_parts(17, "http://archlinux.org/\r", "http://", "archlinux.org", "/");

TEST_only_host_path(18, "https://archlinux.org/releases/downloads",     "archlinux.org/releases/downloads");
TEST_only_host_path(19, "https://archlinux.org/releases/downloads/",    "archlinux.org/releases/downloads");
// Yes, we want to trfalses as a directories. This is because TODO
TEST_only_host_path(20, "https://archlinux.org/releases/page.html",     "archlinux.org/releases/page.html");
TEST_only_host_path(21, "https://archlinux.org/",                       "archlinux.org");
TEST_only_host_path(22, "https://archlinux.org",                        "archlinux.org");
TEST_only_host_path(24, "archlinux.org",                                "archlinux.org");
TEST_only_host_path(25, "https://archlinux.org/packages/webkitgtk-6.0", "archlinux.org/packages/webkitgtk-6.0");
TEST_only_host_path(26, "archlinux.org/link.net",                       "archlinux.org/link.net");
TEST_only_host_path(27, "archlinux.org/link.net/",                      "archlinux.org/link.net");
TEST_only_host_path(28, "archlinux.org/link.net/",                       "archlinux.org/link.net");
TEST_only_host_path(29, "archlinux.org/static/archweb.css",              "archlinux.org/static/archweb.css");
TEST_only_host_path(30, "archlinux.org/static/script.js",                "archlinux.org/static/script.js");
TEST_only_host_path(31, "archlinux.org/static/archlinux_common_style/apple-touch-icon-144x144.png", "archlinux.org/static/archlinux_common_style/apple-touch-icon-144x144.png");
TEST_only_host_path(32, "https://www.archlinux.org/path/to/smth/",       "www.archlinux.org/path/to/smth");
TEST_only_host_path(33, "archlinux.org/mailto:jvinet@zeroflux.org",      "archlinux.org/mailto:jvinet@zeroflux.org");

TEST_relpath(34, "1/2/3", "4", "../../../4");
TEST_relpath(35, "dir1/dir2/one", "dir1/dir2/second", "../../../dir1/dir2/second");
TEST_relpath(36, "1/2", "3/4/5/6/", "../../3/4/5/6/");
TEST_relpath(37, "1/",  "1/2/3/4",  "../1/2/3/4");
TEST_relpath(38, "1/",  "1/2/3/4/", "../1/2/3/4/");
TEST_relpath(39, "1",   "1/2/3/4",  "../1/2/3/4");
TEST_relpath(40, "1",   "1/2/3/4/", "../1/2/3/4/");
TEST_relpath(41, "1/2/3/4", "1", "../../../../1");
TEST_relpath(42, "abc/abc", "abc/a12", "../../abc/a12");
TEST_relpath(43, "archlinux.org/pages/path/", "archlinux.org/other/index.html", "../../../archlinux.org/other/index.html");
TEST_relpath(44, "./archlinux.org/pages/path/", "archlinux.org/other/index.html", "../../../archlinux.org/other/index.html");
TEST_relpath(45, "./archlinux.org/feeds/news/", "archlinux.org/feeds/news/", "../../../archlinux.org/feeds/news/");

TEST_dirname_len(46, "1/2", 2);
TEST_dirname_len(47, "1/2/", 4);
TEST_dirname_len(48, "1", 0);
TEST_dirname_len(49, "1/", 2);

TEST_dir_count(50, "1/2/3/", 3);
TEST_dir_count(51, "1/2/3", 3);
TEST_dir_count(52, "1/2/3/././", 3);
TEST_dir_count(53, "1/2/3/../", 2);
TEST_dir_count(54, "./archlinux.org/pages/path/", 3);

// TODO fix relative paths
#define TEST_read_file(name, path, res_expected, out_expected) \
    TEST(read_file__##name) { \
        char* out; \
        int res = read_file(path, &out); \
        ASSERT_EQUAL_INT(res, res_expected); \
        if (res != -1) { \
            ASSERT_EQUAL_STR(out, out_expected); \
        } \
    } \

#define TEST_write_file(name, path, write_data, res_expected) \
    TEST(write_file__##name) { \
        int res = write_file(path, write_data, 6); \
        ASSERT_EQUAL_INT(res, res_expected); \
    } \

TEST_read_file(1, "non-existing-path", -1, "");
TEST_read_file(2, TESTS_FILES_PATH "/test_util_1.txt", 4, "123\n");
TEST_write_file(3, TESTS_FILES_PATH "/test_util_1_write.txt", "12345\n", 6);

#define TEST_FILE_replace_links(name, file_in, filepath_hint, file_cmp) \
    TEST(replace_links__##name) { \
        char* buff; \
        int size = read_file(file_in, &buff); \
        ASSERT_EQUAL_INT(size > 0, true); \
        \
        char* buff_cmp; \
        int size_cmp = read_file(file_cmp, &buff_cmp); \
        ASSERT_EQUAL_INT(size_cmp > 0, true); \
        \
        char* ptr_out; \
        size_t size_out; \
        FILE* vfile = open_memstream(&ptr_out, &size_out); \
        replace_links((struct vec){ buff, size }, vec_wrap(filepath_hint), vfile); \
        fclose(vfile); \
        \
        ASSERT_EQUAL_STRN_LARGE(ptr_out, buff_cmp, size); \
        free(ptr_out); \
    }

TEST_FILE_replace_links(
    1,
    TESTS_FILES_PATH"/archlinux.org.html",
    "./archlinux.org/index.html",
    TESTS_FILES_PATH"/archlinux.org.html.replaced"
);

#define TEST_should_crawl_url(name, url, page_url, domain_filter, res_expected) \
    TEST(should_crawl_url__##name) { \
        bool res = should_crawl_url(vec_wrap(url), vec_wrap(page_url), domain_filter); \
        ASSERT_EQUAL_INT(res, res_expected); \
    } \

TEST_should_crawl_url(1, ".", ".", DOMAIN_FILTER_NO, true);
TEST_should_crawl_url(2, "https://archlinux.org", "https://archlinux.org", DOMAIN_FILTER_NO, true);
TEST_should_crawl_url(3, "https://archlinux.org", "https://archlinux.org", DOMAIN_FILTER_SAME, true);
TEST_should_crawl_url(4, "https://archlinux.org", "https://archlinux.org", DOMAIN_FILTER_SUBDOMAIN, true);

TEST_should_crawl_url(5, "https://wiki.archlinux.org", "https://archlinux.org", DOMAIN_FILTER_NO, true);
TEST_should_crawl_url(6, "https://wiki.archlinux.org", "https://archlinux.org", DOMAIN_FILTER_SAME, false);
TEST_should_crawl_url(7, "https://wiki.archlinux.org", "https://archlinux.org", DOMAIN_FILTER_SUBDOMAIN, true);

TEST_should_crawl_url(8, "https://archlinux.org", "https://archlinux.org", DOMAIN_FILTER_NO, true);
TEST_should_crawl_url(9, "https://archlinux.org", "https://archlinux.org", DOMAIN_FILTER_SAME, true);
TEST_should_crawl_url(10, "https://archlinux.org", "https://archlinux.org", DOMAIN_FILTER_SUBDOMAIN, true);

TEST_should_crawl_url(11, "https://wiki.archlinux.org", "https://archlinux.org", DOMAIN_FILTER_NO, true);
TEST_should_crawl_url(12, "https://wiki.archlinux.org", "https://archlinux.org", DOMAIN_FILTER_SAME, false);
TEST_should_crawl_url(13, "https://wiki.archlinux.org", "https://archlinux.org", DOMAIN_FILTER_SUBDOMAIN, true);

TEST_should_crawl_url(14, "https://github.com/rust-lang/rust/tree/HEAD/src/doc/rustc", "https://doc.rust-lang.org/stable/rustc/index.html", DOMAIN_FILTER_SAME, false);

bool crawl_urls_called = false;
int crawl_urls_arg_depth_level = INT_MIN;
struct vec crawl_urls_arg_url = {NULL, 0};

void crawl_urls_mock(struct vec url, int depth_level) {
    fprintf(stderr, "==== CRAWL_URLS_MOCK: %.*s, %d\n", url.size, url.ptr, depth_level);
    crawl_urls_called = true;
    crawl_urls_arg_depth_level = depth_level;
    crawl_urls_arg_url = url;
}

#define TEST_crawl_url_cb(name, tag_name, link, type, page_url, depth_level, called_exp, url_exp) \
    TEST(crawl_url_cb__##name) { \
        struct RecursiveCrawlCtx ctx = { \
            vec_wrap(page_url), \
            depth_level, \
            crawl_urls_mock \
        }; \
        struct HtmlTag t = {tag_name, NULL, vec_wrap(link), vec_wrap(type)}; \
        on_found_url_callback(&t, &ctx, stderr); \
        ASSERT_EQUAL_INT(crawl_urls_called, called_exp); \
        if (called_exp) { \
            ASSERT_EQUAL_INT(crawl_urls_arg_depth_level, depth_level - 1); \
            ASSERT_EQUAL_VEC(crawl_urls_arg_url, vec_wrap(url_exp)); \
            crawl_urls_called = false; \
        } \
    } \

TEST_crawl_url_cb(0, "link", "/static/archlinux_common_style/navbar.css", "text/css", "https://archlinux.org/index.html", 1, false, "");
TEST_crawl_url_cb(1, "a", "/content.html", "", "https://archlinux.org/index.html", 1, true, "https://archlinux.org/content.html");
TEST_crawl_url_cb(2, "a", "/a/b/c/content.html", "", "https://archlinux.org/packages/index.html", 1, true, "https://archlinux.org/a/b/c/content.html");
TEST_crawl_url_cb(3, "a", "a/b/c/content.html", "", "https://archlinux.org/packages/index.html", 1, true, "https://archlinux.org/packages/a/b/c/content.html");


int main() {
    fclose(stderr);
    run_tests();
}

