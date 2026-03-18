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

#include "./url_parser.h"
#include "./html_parser.h"
#include "./network.h"
#include "./util.h"

typedef enum {
    DOMAIN_FILTER_NO,
    DOMAIN_FILTER_SAME,
    DOMAIN_FILTER_SUBDOMAIN,
} DomainFilterType;

struct cmd_args {
    const char* root_url;
    int depth_level;
    int request_period;
    int request_timeout;
    DomainFilterType filter_type;
};

struct FoundUrlCallbackCtx {
    const char* page_url;
    int depth_level;
};

struct cmd_args cmd_args = {
    .root_url = "",
    .depth_level = DEFAULT_DEPTH_LEVEL,
    .request_period = DEFAULT_REQUEST_PERIOD,
    .request_timeout = DEFAULT_REQUEST_TIMEOUT,
    .filter_type = DOMAIN_FILTER_NO,
};


void crawl_urls(const char* url, int depth_level);

void on_found_url_callback(const char* found_url, HrefType ht, void* ctx) {
    /*
     * The function takes ownership of @found_url, i.e. it frees it at the end.
     */

    struct FoundUrlCallbackCtx* capture_ctx = (struct FoundUrlCallbackCtx*)ctx;
    const char* page_url = capture_ctx->page_url;
    int depth_level = capture_ctx->depth_level;
    char* result_url = NULL;

    assert(page_url != NULL);
    assert(found_url != NULL);
    assert(strlen(page_url) != 0);
    assert(strlen(found_url) != 0);

    bool free_result_url = false;
    if (is_url_http(found_url)) {
        // Conversion: result_url is not changed further, needed for else branch
        result_url = (char*)found_url;
    } else if (is_url_relative(found_url)) {
        struct UrlParts p_page;
        parse_url(page_url, &p_page);
        char* protocol = p_page.protocol;
        char* host = p_page.host;
        
        int full_url_len = strlen(protocol) + strlen(host) + strlen(found_url) + 1;
        char* full_url = (char *)malloc(full_url_len * sizeof(char));

        sprintf(full_url, "%s%s%s", protocol, host, found_url);
        result_url = full_url;
        free_result_url = true;
    } else {
        // URL does not match needed criterias
        goto cleanup;
    }

    if (cmd_args.filter_type != DOMAIN_FILTER_NO) {
        struct UrlParts p_result, p_page;
        parse_url(result_url, &p_result);
        parse_url(page_url, &p_page);
        if (cmd_args.filter_type == DOMAIN_FILTER_SAME) {
            if (strcmp(p_result.host, p_page.host) != 0) {
                goto cleanup;
            }
        }
        if (cmd_args.filter_type == DOMAIN_FILTER_SUBDOMAIN) {
            // Check whether result_url is a subdomain of page_url, f.e.:
            // p_result.host =  terms.archlinux.org
            // p_page.host   =        archlinux.org
            // Then first is a subdomain of the second
            if (!ends_with(p_result.host, p_page.host)) {
                goto cleanup;
            }
        }
    }

    // Finally print URL to stdout
    switch (ht) {
        case HREF_TYPE_UNKNOWN: printf("UNKNOWN"); break;
        case HREF_TYPE_IMG: printf("IMAGE"); break;
        case HREF_TYPE_STYLE: printf("STYLE"); break;
        case HREF_TYPE_SCRIPT: printf("SCRIPT"); break;
        case HREF_TYPE_HTML: printf("HTML"); break;
    }
    printf("\t%s\n", result_url);

    if (ht == HREF_TYPE_HTML) {
        // Crawl URLs recursively
        crawl_urls(result_url, depth_level - 1);
    }

cleanup:
    if (free_result_url) {
        free(result_url);
    }
}

void crawl_urls(const char* url, int depth_level) {
    /*
     * The algorithm is:
     * - Retrieve requested URL
     * - Traverse HTML to found http hrefs
     * - Recursively crawl found URLs by a depth of @depth_level
     */
    if (depth_level == 0) {
        return;
    }

    struct HttpPage downloaded_page;

    // Wait period between network requests
    usleep(cmd_args.request_period * 1000); 
    int res = download_http(
        url,
        cmd_args.request_timeout / 1000,
        &downloaded_page
    );
    if (res == 0) {
        struct FoundUrlCallbackCtx ctx = { 
            downloaded_page.effective_url,
            depth_level,
        };
        search_resource_urls(
            downloaded_page.data_vec->ptr + downloaded_page.content_offset,
            downloaded_page.data_vec->size - downloaded_page.content_offset,
            on_found_url_callback,
            &ctx
        );

        // Cleanup
        vec_deinit(downloaded_page.data_vec);
    }
}

void exit_args_error() {
    fprintf(stderr, "Usage: ./hrefs_crawler -a <addr> [-d <depth>] [-p <period>] [-t <timeout]\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "\t-a, --address <addr>\tURL the crawler starts from\n");
    fprintf(stderr, "\t-d, --depth <val>\tLevel of recursion a crawler will dive into hrefs (default: %d)\n", DEFAULT_DEPTH_LEVEL);
    fprintf(stderr, "\t-p, --period <val>\tPeriod between TCP requests (in miliseconds) (default: %d)\n", DEFAULT_REQUEST_PERIOD);
    fprintf(stderr, "\t-t, --timeout <val>\tTimeout for TCP requests (in miliseconds) (default: %d)\n", DEFAULT_REQUEST_TIMEOUT);
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
                if (strcmp(long_options[longindex].name, "filter-subdomain") == 0) {
                    args->filter_type = DOMAIN_FILTER_SUBDOMAIN;
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
                    fprintf(stderr, "Error: argument is not a number: %s\n", optarg);
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
                    fprintf(stderr, "Error: argument is not a number: %s\n", optarg);
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
        fprintf(stderr, "Address argument is required\n");
        exit_args_error();
    }
}

int main(int argc, char* argv[]) {
    parse_cmd_args(argc, argv, &cmd_args);

    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    crawl_urls(cmd_args.root_url, cmd_args.depth_level);

    return 0;
}
