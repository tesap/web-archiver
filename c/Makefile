
OUT=out
DEPS=-lssl -lcrypto
FLAGS=-Wreturn-type -fno-exceptions -fno-rtti ${DEPS}
GCC=g++

.PHONY: ${OUT} clean

all: ${OUT}/hrefs_crawler ${OUT}/tests-url_parser ${OUT}/tests-http_parser ${OUT}/tests-util

clean:
	rm -rf ${OUT}

${OUT}/hrefs_crawler: hrefs_crawler.c url_parser.c html_parser.c network.c util.c http_parser.c | out
	${GCC} ${FLAGS} $^ -o $@
    
${OUT}/tests-url_parser: tests/tests-url_parser.c url_parser.c util.c | out
	${GCC} ${FLAGS} $^ -o $@

${OUT}/tests-http_parser: tests/tests-http_parser.c util.c http_parser.c url_parser.c | out
	${GCC} ${FLAGS} $^ -o $@

${OUT}/tests-util: tests/tests-util.c util.c | ${OUT} ${OUT}/tests/files
	${GCC} ${FLAGS} $^ -o $@

${OUT}/tests/files: tests/files
	ln -s ../tests/files out/

${OUT}:
	mkdir -p $@
