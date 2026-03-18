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

// #include "./url_parser.h"
// #include "./html_parser.h"
// #include "./network.h"
// #include "./util.h"

// Iterate each href in HTML document:
// - calculate domain + relative path 
// - Download by path_by(domain, relative_path)
// - If already exist, skip download

void exit_args_error() {
    fprintf(stderr, "Usage: ./single_page -a <addr> [-p <period>] [-t <timeout]\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "\t-a, --address <addr>\tURL to download\n");
    fprintf(stderr, "\t-p, --period <val>\tPeriod between TCP requests (in miliseconds) (default: %d)\n", DEFAULT_REQUEST_PERIOD);
    fprintf(stderr, "\t-t, --timeout <val>\tTimeout for TCP requests (in miliseconds) (default: %d)\n", DEFAULT_REQUEST_TIMEOUT);
    exit(1);
}


int main(int argc, char* argv[]) {
    exit_args_error();
    // #define LINE_SIZE 1024
    // char buff[LINE_SIZE];
    // while (fgets(buff, LINE_SIZE, stdin)) {
    //     printf("---> |%s|\n", buff);
    // }
}
