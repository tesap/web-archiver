
OUT=out
DEPS=-lssl \
     -lcrypto
FLAGS=-g \
      -Isrc \
      -Isrc/util \
      -Isrc/hrefs_crawler \
      -Wreturn-type ${DEPS} \
      -DDEFAULT_DEPTH_LEVEL=1 \
      -DDEFAULT_REQUEST_PERIOD=50 \
      -DDEFAULT_REQUEST_TIMEOUT=3000 \
      -DMAX_HTML_TAG_LENGTH=1024 \
      -DMAX_URL_LENGTH=1024 \
      -DTESTS_FILES_PATH=\"out/files\"

COVERAGE_FLAGS=-fprofile-arcs \
               -ftest-coverage \
               -O0 
# TESTS_FLAGS=-Wl,--wrap=/0

# GCC=../tinycc/tcc
# GCC=tcc
GCC=gcc
# GCC=g++

.PHONY: ${OUT} clean

all: ${OUT}/cached_curl ${OUT}/hrefs_crawler ${OUT}/links_replacer ${OUT}/tests

clean:
	rm -rf ${OUT}

${OUT}/hrefs_crawler: src/hrefs_crawler/hrefs_crawler.c src/hrefs_crawler/main.c src/util.c | ${OUT}
	${GCC} ${FLAGS} $^ -o $@

${OUT}/cached_curl: src/cached_curl.c src/util.c | ${OUT}
	${GCC} ${FLAGS} $^ -o $@
    
${OUT}/links_replacer: src/links_replacer.c src/util.c | ${OUT}
	${GCC} ${FLAGS} $^ -o $@
    
${OUT}/tests: tests/tests.c src/hrefs_crawler/hrefs_crawler.c src/util.c | ${OUT} ${OUT}/tests/files
	${GCC} ${FLAGS} ${COVERAGE_FLAGS} $^ -o $@

${OUT}/tests/files: tests/files
	ln -sf ../tests/files ${OUT}/

${OUT}:
	mkdir -p $@
