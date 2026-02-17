#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <ctype.h>


#include "./hrefs_crawler.h"
#include "./util.h"

#define DEFAULT_DEPTH_LEVEL 1
#define DEFAULT_REQUEST_PERIOD 50
#define DEFAULT_REQUEST_TIMEOUT 30000

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

void parse_params(int argc, char* argv[], struct CrawlerParams* out) {
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
                    out->filter_type = DOMAIN_FILTER_SAME;
                }
                if (strcmp(long_options[longindex].name, "filter-subdomain") == 0) {
                    out->filter_type = DOMAIN_FILTER_SUBDOMAIN;
                }
                break;
            case 'a':
                if (a_flag) {
                    exit_args_error();
                }
                a_flag = true;
                out->root_url = strdup(optarg);
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
                out->depth_level = atoi(optarg);
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
                out->request_period = atoi(optarg);
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
                out->request_timeout = atoi(optarg);
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
    static struct CrawlerParams params = {
        .root_url = "",
        .depth_level = DEFAULT_DEPTH_LEVEL,
        .request_period = DEFAULT_REQUEST_PERIOD,
        .request_timeout = DEFAULT_REQUEST_TIMEOUT,
        .filter_type = DOMAIN_FILTER_NO,
    };

    parse_params(argc, argv, &params);

    crawl_urls(&params);

    return 0;
}
