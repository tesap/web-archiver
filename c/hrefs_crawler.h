#include "./network.h"

typedef enum {
    DOMAIN_FILTER_NO,
    DOMAIN_FILTER_SAME,
    DOMAIN_FILTER_SUBDOMAIN,
} DomainFilterType;

struct CrawlerParams {
    const char* root_url;
    int depth_level;
    int request_period;
    int request_timeout;
    DomainFilterType filter_type;
};


void handle_found_url_cb(const char* page_url, int depth_level, char* found_url);
void crawl_urls(struct CrawlerParams* params);
