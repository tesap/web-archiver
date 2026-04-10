
#include "util.h"

struct RecursiveCrawlCtx {
    struct vec page_url;
    int depth_level;
    void (*crawl_urls)(struct vec url, int depth_level);
    DomainFilterType filter_type;
};

void on_found_url_callback(struct HtmlTag* t, const void* ctx, FILE* fout);

