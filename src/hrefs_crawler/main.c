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

struct cmd_args {
    const char* root_url;
    int depth_level;
    int request_period;
    int request_timeout;
    DomainFilterType filter_type;
    bool is_save;
    int cache_ttl;
};

struct cmd_args cmd_args = {
    .root_url = "",
    .depth_level = DEFAULT_DEPTH_LEVEL,
    .request_period = DEFAULT_REQUEST_PERIOD,
    .request_timeout = DEFAULT_REQUEST_TIMEOUT,
    .filter_type = DOMAIN_FILTER_NO,
    .is_save = false,
    .cache_ttl = 0,
};

void exit_args_error() {
    fprintf(stderr, "Usage: ./hrefs_crawler -a <addr> [-d <depth>] [-p <period>] [-t <timeout]\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "\t-a, --address <addr>\tURL the crawler starts from\n");
    fprintf(stderr, "\t-d, --depth <val>\tLevel of recursion a crawler will dive into hrefs (default: %d)\n", DEFAULT_DEPTH_LEVEL);
    fprintf(stderr, "\t-p, --period <val>\tPeriod between TCP requests (in miliseconds) (default: %d)\n", DEFAULT_REQUEST_PERIOD);
    fprintf(stderr, "\t-t, --timeout <val>\tTimeout for TCP requests (in miliseconds) (default: %d)\n", DEFAULT_REQUEST_TIMEOUT);
    fprintf(stderr, "\t--save\t\t\tSave downloaded pages to fs cache\n");
    fprintf(stderr, "\t--cache-ttl <ttl>\tUse saved pages from fs to cache repeated requests, according to given TTL (Time to Live) of saved entries. 0 means each saved page is considered invalidated. TTL is in minutes (default: 0)\n");
    fprintf(stderr, "\t--filter-same-domain\tOnly account for URLs with the same domain as the initial URL\n");
    fprintf(stderr, "\t--filter-subdomain\tOnly account for URLs to subdomains to the domain of the initial URL\n");
    exit(1);
}

void parse_cmd_args(int argc, char* argv[], struct cmd_args* args) {
    bool a_flag = false;
    bool d_flag = false;
    bool p_flag = false;
    bool t_flag = false;

    while (1) {
        static struct option long_options[] = {
            {"address",             required_argument, 0, 'a'},
            {"depth-level",         required_argument, 0, 'd'},
            {"request-period",      required_argument, 0, 'p'},
            {"request-timeout",     required_argument, 0, 't'},
            {"save",                no_argument, 0, 0},
            {"cache-ttl",           required_argument, 0, 0},
            {"filter-same-domain",  no_argument, 0, 0},
            {"filter-subdomain",    no_argument, 0, 0},
            {0, 0, 0, 0}
        };

        int longindex;
        int c = getopt_long(argc, argv, "a:d:p:t:", long_options, &longindex);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 0:
                if (strcmp(long_options[longindex].name, "filter-same-domain") == 0) {
                    args->filter_type = DOMAIN_FILTER_SAME;
                }
                else if (strcmp(long_options[longindex].name, "filter-subdomain") == 0) {
                    args->filter_type = DOMAIN_FILTER_SUBDOMAIN;
                }
                else if (strcmp(long_options[longindex].name, "save") == 0) {
                    args->is_save = true;
                }
                else if (strcmp(long_options[longindex].name, "cache-ttl") == 0) {
                    if (!is_number(optarg)) {
                        fprintf(stderr, ANSI_COLOR_RED "Error: argument is not a number: %s\n" ANSI_COLOR_RESET, optarg);
                        exit(1);
                    }
                    args->cache_ttl = atoi(optarg);
                }
                break;
            case 'a':
                if (a_flag) {
                    exit_args_error();
                }
                a_flag = true;
                args->root_url = optarg;
                break;
            case 'd':
                if (d_flag) {
                    exit_args_error();
                }
                if (!is_number(optarg)) {
                    fprintf(stderr, ANSI_COLOR_RED "Error: argument is not a number: %s\n" ANSI_COLOR_RESET, optarg);
                    exit(1);
                }
                d_flag = true;
                args->depth_level = atoi(optarg);
                break;
            case 'p':
                if (p_flag) {
                    exit_args_error();
                }
                if (!is_number(optarg)) {
                    fprintf(stderr, "Error: argument is not a number: %s\n", optarg);
                    exit_args_error();
                }
                p_flag = true;
                args->request_period = atoi(optarg);
                break;
            case 't':
                if (t_flag) {
                    exit_args_error();
                }
                if (!is_number(optarg)) {
                    fprintf(stderr, ANSI_COLOR_RED "Error: argument is not a number: %s\n" ANSI_COLOR_RESET, optarg);
                    exit_args_error();
                }
                t_flag = true;
                args->request_timeout = atoi(optarg);
                break;
            case '?':
                exit_args_error();
            default:
                printf("==== default\n");
        }
    }

    // Assert required arguments
    if (!a_flag) {
        fprintf(stderr, ANSI_COLOR_RED "Address argument is required\n" ANSI_COLOR_RESET);
        exit_args_error();
    }
}

void crawl_urls(struct vec url, int depth_level) {
    /*
     * The algorithm is:
     * - Retrieve requested URL
     * - Traverse HTML to found http hrefs
     * - Recursively crawl found URLs by a depth of @depth_level
     */
    if (depth_level == 0) {
        return;
    }

    char _mem1[256];
    struct HttpPage downloaded_page = HttpPage_init(_mem1);
    int res = cached_download_http(
        url,
        cmd_args.request_timeout,
        cmd_args.is_save,
        cmd_args.cache_ttl,
        LINK_TYPE_HTML,
        &downloaded_page
    );

    if (res >= 0) {
        struct RecursiveCrawlCtx ctx = { 
            downloaded_page.effective_url,
            depth_level,
            crawl_urls,
            cmd_args.filter_type
        };
        iter_html_tags(
            downloaded_page.content_vec,
            on_found_url_callback,
            &ctx
        );
    } else {
        exit(res);
    }

    // Page was downloaded from network
    if (res == 0) {
        // Wait period between network requests
        usleep(cmd_args.request_period * 1000); 
    }
cleanup:
    HttpPage_dealloc(downloaded_page);
}

int main(int argc, char* argv[]) {
    parse_cmd_args(argc, argv, &cmd_args);

    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    crawl_urls(vec_wrap(cmd_args.root_url), cmd_args.depth_level);

    return 0;
}
