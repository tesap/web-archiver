#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <ctype.h>
#include <assert.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "util.h"
#include "hrefs_crawler.h"

void on_found_url_callback(struct HtmlTag* t, const void* ctx_, FILE* fout) {
    struct RecursiveCrawlCtx* ctx = (struct RecursiveCrawlCtx*)ctx_;

    assert(ctx->page_url.ptr != NULL);
    assert(ctx->page_url.size > 0);
    assert(t->link.ptr != NULL);
    assert(t->link.size > 0);

    if (starts_with(t->link, vec_wrap("../"))) {
        return;
    }

    char _mem1[MAX_URL_LENGTH];
    struct vec res_url = {_mem1, 0};

    // For now we are only interested in printing the right URL, not saving it.
    link_to_full_url(t->link, ctx->page_url, &res_url);

    if (!should_crawl_url(res_url, ctx->page_url, ctx->filter_type)) {
        return;
    }

    // Finally print URL to stdout
    LinkType lt = tag_link_type(t);
    switch (lt) {
        case LINK_TYPE_UNKNOWN: fprintf(fout, "NO"); break;
        case LINK_TYPE_IMG: fprintf(fout, "IMAGE"); break;
        case LINK_TYPE_STYLE: fprintf(fout, "STYLE"); break;
        case LINK_TYPE_SCRIPT: fprintf(fout, "SCRIPT"); break;
        case LINK_TYPE_HTML: fprintf(fout, "HTML"); break;
    }
    fprintf(fout, "\t%.*s\n", res_url.size, res_url.ptr);

    if (lt == LINK_TYPE_HTML) {
        // Crawl URLs recursively
        ctx->crawl_urls(res_url, ctx->depth_level - 1);
    }
}
