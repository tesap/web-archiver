#include "./tests-common.h"
#include "../html_parser.h"
#include "../util.h"

#include <assert.h>

#define TEST_ITER_HTML_TAGS(name, buff, cmp_list, cmp_list_size) \
    TEST(html_parser_##name) { \
        int cmp_index = 0; \
        struct TestCallbackCtx ctx = { \
            &cmp_index, \
            cmp_list, \
            cmp_list_size \
        }; \
        iter_html_tags(buff, strlen(buff), iter_html_tags_cb, &ctx); \
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
        iter_html_tags(buff, size, iter_html_tags_cb, &ctx); \
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

void iter_html_tags_cb(struct HtmlTag* t, void* ctx) {
    struct TestCallbackCtx* ctx_struct = (struct TestCallbackCtx*)ctx;
    int* cmp_index = ctx_struct->cmp_index;
    struct CmpEntry* cmp_list = ctx_struct->cmp_list;
    int cmp_list_size = ctx_struct->cmp_list_size;

    printf("=== (%d) URL: %.*s\n", *cmp_index, t->link_size, t->link_start);

    ASSERT_EQUAL_STR(t->tag_name, cmp_list[*cmp_index].s1);
    ASSERT_EQUAL_STRN(t->link_start, cmp_list[*cmp_index].s2, t->link_size);
    ASSERT_EQUAL_STRN(t->type_start, cmp_list[*cmp_index].s3, t->type_size);
    ASSERT_EQUAL_STRN(t->tag_end, ">", 1);
    (*cmp_index)++;
}

// TEST(html_parser_1) { ASSERT_EQUAL_INT(tag_link_type("https://aur.archlinux.org/", NULL, "a"), LINK_TYPE_HTML); }
// TEST(html_parser_2) { ASSERT_EQUAL_INT(tag_link_type("/feed/packages/", NULL, "a"), LINK_TYPE_HTML); }
// TEST(html_parser_3) { ASSERT_EQUAL_INT(tag_link_type("#title-anything", NULL, "a"), LINK_TYPE_HTML); }
// TEST(html_parser_4) { ASSERT_EQUAL_INT(tag_link_type("https://stackoverflow.com/Content/Sites/stackoverflow/Img/apple-touch-icon.png?v=9168b8ec82a5", NULL, "link"), LINK_TYPE_IMG); }

// TEST(html_parser_5) { ASSERT_EQUAL_INT(tag_link_type("magnet:?xt=urn:btih:a4373c326657898d0c588c3ff892a0fac97ffa20&amp;dn=archlinux-2026.03.01-x86_64.iso", NULL, "a"), LINK_TYPE_UNKNOWN); }
// TEST(html_parser_6) { ASSERT_EQUAL_INT(tag_link_type("mailto:jvinet@zeroflux.org", NULL, "a"), LINK_TYPE_UNKNOWN); }

struct CmpEntry cmp_list5[] = {
    { "a", "HREF1", NULL },
    { "a", "HREF2", NULL },
};
int cmp_list_size5 = sizeof(cmp_list5) / sizeof(struct CmpEntry);

TEST_ITER_HTML_TAGS(5, "<a href=\"HREF1\"/><a href=\"HREF2\"/>", cmp_list5, cmp_list_size5);

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

TEST_SEARCH_RESOURCE_URL_FILE(6, "./out/files/archlinux.org.html", cmp_list6, cmp_list_size6);

int main() {
    // RUN_TEST(html_parser_1);
    // RUN_TEST(html_parser_2);
    // RUN_TEST(html_parser_3);
    // RUN_TEST(html_parser_4);
    RUN_TEST(html_parser_5);
    RUN_TEST(html_parser_6);
}
