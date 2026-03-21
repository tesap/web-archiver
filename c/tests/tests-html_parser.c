#include "./tests-common.h"
#include "../html_parser.h"
#include "../util.h"

#include <assert.h>

#define TEST_SEARCH_RESOURCE_URL(name, buff, cmp_list, cmp_list_size) \
    TEST(html_parser_##name) { \
        int cmp_index = 0; \
        struct TestCallbackCtx ctx = { \
            &cmp_index, \
            cmp_list, \
            cmp_list_size \
        }; \
        search_resource_urls(buff, strlen(buff), test_callback_search_resource_url, &ctx); \
        ASSERT_EQUAL_INT(cmp_index, cmp_list_size); \
    } \

#define TEST_SEARCH_RESOURCE_URL_FILE(name, filepath, cmp_list, cmp_list_size) \
    TEST(html_parser_##name) { \
        char* buff; \
        int size = read_file(filepath, &buff); \
        ASSERT_EQUAL_INT(size > 0, true); \
        int cmp_index = 0; \
        struct TestCallbackCtx ctx = { \
            &cmp_index, \
            cmp_list, \
            cmp_list_size \
        }; \
        search_resource_urls(buff, size, test_callback_search_resource_url, &ctx); \
    } \

struct HtmlLink {
    char* value;
    HrefType type;
};

struct TestCallbackCtx {
    int* cmp_index;
    struct HtmlLink* cmp_list;
    int cmp_list_size;
};

void test_callback_search_resource_url(const char* found_url_start, int found_url_size, HrefType ht, void* ctx) {
    struct TestCallbackCtx* ctx_struct = (struct TestCallbackCtx*)ctx;
    int* cmp_index = ctx_struct->cmp_index;
    struct HtmlLink* cmp_list = ctx_struct->cmp_list;
    int cmp_list_size = ctx_struct->cmp_list_size;

    if (*cmp_index >= cmp_list_size) {
        return;
    }

    // printf("\n=== (%d) URL: %.*s", *cmp_index, found_url_size, found_url_start);

    ASSERT_EQUAL_STRN(found_url_start, cmp_list[*cmp_index].value, found_url_size);
    ASSERT_EQUAL_INT(ht, cmp_list[*cmp_index].type);
    (*cmp_index)++;
}

TEST(html_parser_1) { ASSERT_EQUAL_INT(href_type("https://aur.archlinux.org/", NULL, "a"), HREF_TYPE_HTML); }
TEST(html_parser_2) { ASSERT_EQUAL_INT(href_type("/feed/packages/", NULL, "a"), HREF_TYPE_HTML); }
TEST(html_parser_3) { ASSERT_EQUAL_INT(href_type("#title-anything", NULL, "a"), HREF_TYPE_HTML); }
TEST(html_parser_4) { ASSERT_EQUAL_INT(href_type("https://stackoverflow.com/Content/Sites/stackoverflow/Img/apple-touch-icon.png?v=9168b8ec82a5", NULL, "link"), HREF_TYPE_IMG); }
// TEST(html_parser_5) { ASSERT_EQUAL_INT(href_type("magnet:?xt=urn:btih:a4373c326657898d0c588c3ff892a0fac97ffa20&amp;dn=archlinux-2026.03.01-x86_64.iso", NULL, "a"), HREF_TYPE_UNKNOWN); }
// TEST(html_parser_6) { ASSERT_EQUAL_INT(href_type("mailto:jvinet@zeroflux.org", NULL, "a"), HREF_TYPE_UNKNOWN); }

struct HtmlLink cmp_list5[] = {
    {"HREF1", HREF_TYPE_HTML},
    {"HREF2", HREF_TYPE_HTML},
};
int cmp_list_size5 = sizeof(cmp_list5) / sizeof(struct HtmlLink);

TEST_SEARCH_RESOURCE_URL(5, "<a href=\"HREF1\"/><a href=\"HREF2\"/>", cmp_list5, cmp_list_size5);

struct HtmlLink cmp_list6[] = {
    {"/static/archlinux_common_style/navbar.css", HREF_TYPE_STYLE},
    {"/static/archweb.css", HREF_TYPE_STYLE},
    {"/static/archlinux_common_style/favicon.png", HREF_TYPE_IMG},
    {"/static/archlinux_common_style/favicon.png", HREF_TYPE_IMG},
    {"/static/archlinux_common_style/apple-touch-icon-57x57.png", HREF_TYPE_IMG},
    {"/static/archlinux_common_style/apple-touch-icon-72x72.png", HREF_TYPE_IMG},
    {"/static/archlinux_common_style/apple-touch-icon-114x114.png", HREF_TYPE_IMG},
    {"/static/archlinux_common_style/apple-touch-icon-144x144.png", HREF_TYPE_IMG},
    {"/opensearch/packages/", HREF_TYPE_UNKNOWN},
    {"/feeds/news/", HREF_TYPE_UNKNOWN},
    {"/feeds/packages/", HREF_TYPE_UNKNOWN},
    {"/static/homepage.js", HREF_TYPE_SCRIPT},
    {"https://fosstodon.org/@archlinux", HREF_TYPE_UNKNOWN},
    {"/", HREF_TYPE_HTML},
    {"/", HREF_TYPE_HTML},
    {"/packages/", HREF_TYPE_HTML},
    {"https://bbs.archlinux.org/", HREF_TYPE_HTML},
    {"https://wiki.archlinux.org/", HREF_TYPE_HTML},
    {"https://gitlab.archlinux.org/archlinux", HREF_TYPE_HTML},
    {"https://security.archlinux.org/", HREF_TYPE_HTML},
    {"https://aur.archlinux.org/", HREF_TYPE_HTML},
    {"/download/", HREF_TYPE_HTML},
    {"https://aur.archlinux.org/", HREF_TYPE_HTML},
    // Other hrefs are omitted
};
int cmp_list_size6 = sizeof(cmp_list6) / sizeof(struct HtmlLink);

TEST_SEARCH_RESOURCE_URL_FILE(6, "./out/files/archlinux.org.html", cmp_list6, cmp_list_size6);

int main() {
    RUN_TEST(html_parser_1);
    RUN_TEST(html_parser_2);
    RUN_TEST(html_parser_3);
    RUN_TEST(html_parser_4);
    RUN_TEST(html_parser_5);
    RUN_TEST(html_parser_6);
}
