
OUT=out
DEPS=-lssl -lcrypto
FLAGS=-g -Wreturn-type -fno-exceptions -fno-rtti ${DEPS} -DDEFAULT_DEPTH_LEVEL=1 -DDEFAULT_REQUEST_PERIOD=50 -DDEFAULT_REQUEST_TIMEOUT=3000 -DMAX_HTML_TAG_LENGTH=1024 -DMAX_URL_LENGTH=1024

# GCC=gcc
GCC=g++

.PHONY: ${OUT} clean

all: ${OUT}/cached_curl ${OUT}/hrefs_crawler ${OUT}/tests-url_parser ${OUT}/tests-http_parser ${OUT}/tests-util ${OUT}/tests-html_parser

clean:
	rm -rf ${OUT}

${OUT}/hrefs_crawler: hrefs_crawler.c url_parser.c html_parser.c network.c util.c http_parser.c cached_network.c | out
	${GCC} ${FLAGS} $^ -o $@

${OUT}/cached_curl: http_parser.c network.c url_parser.c cached_network.c util.c cached_curl.c | out
	${GCC} ${FLAGS} $^ -o $@
    
${OUT}/tests-url_parser: tests/tests-url_parser.c url_parser.c util.c | out
	${GCC} ${FLAGS} $^ -o $@

${OUT}/tests-http_parser: tests/tests-http_parser.c util.c http_parser.c url_parser.c | out
	${GCC} ${FLAGS} $^ -o $@

${OUT}/tests-html_parser: tests/tests-html_parser.c url_parser.c html_parser.c util.c | out
	${GCC} ${FLAGS} $^ -o $@

${OUT}/tests-util: tests/tests-util.c util.c | ${OUT} ${OUT}/files
	${GCC} ${FLAGS} $^ -o $@

${OUT}/files: tests/files
	ln -s ../tests/files out/

${OUT}:
	mkdir -p $@
