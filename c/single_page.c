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

int main(int argc, char* argv[]) {
    return 0;
}
