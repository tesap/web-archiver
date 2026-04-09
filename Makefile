
OUT=out
DEPS=-lssl \
     -lcrypto
FLAGS=-g \
      -Wreturn-type ${DEPS} \
      -DDEFAULT_DEPTH_LEVEL=1 \
      -DDEFAULT_REQUEST_PERIOD=50 \
      -DDEFAULT_REQUEST_TIMEOUT=3000 \
      -DMAX_HTML_TAG_LENGTH=1024 \
      -DMAX_URL_LENGTH=1024

COVERAGE_FLAGS=-fprofile-arcs \
               -ftest-coverage \
               -O0 

GCC=gcc
# GCC=g++

.PHONY: ${OUT} clean

all: ${OUT}/cached_curl ${OUT}/hrefs_crawler ${OUT}/links_replacer ${OUT}/tests # ${OUT}/tests-url_parser ${OUT}/tests-http_parser ${OUT}/tests-util ${OUT}/tests-html_parser ${OUT}/links_replacer

clean:
	rm -rf ${OUT}

${OUT}/hrefs_crawler: hrefs_crawler.c util.c | ${OUT}
	${GCC} ${FLAGS} $^ -o $@

${OUT}/cached_curl: util.c cached_curl.c | ${OUT}
	${GCC} ${FLAGS} $^ -o $@
    
${OUT}/links_replacer: util.c links_replacer.c | ${OUT}
	${GCC} ${FLAGS} $^ -o $@
    
${OUT}/tests: tests.c util.c | ${OUT} ${OUT}/tests-files
	${GCC} ${FLAGS} ${COVERAGE_FLAGS} $^ -o $@

${OUT}/tests-files: tests-files
	ln -s ../tests-files ${OUT}/

${OUT}:
	mkdir -p $@
