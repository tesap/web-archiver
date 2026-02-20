

OUT=out
DEPS=-lssl -lcrypto
FLAGS=-Werror -Wreturn-type -fno-exceptions -fno-rtti

.PHONY: ${OUT} clean

all: ${OUT}/hrefs_crawler ${OUT}/tests-url_parser ${OUT}/tests-network ${OUT}/tests-util

clean:
	rm -rf ${OUT}

${OUT}/hrefs_crawler: hrefs_crawler.c url_parser.c html_parser.c network.c util.c | out
	gcc $^ ${DEPS} -o $@
    
${OUT}/tests-url_parser: tests/tests-url_parser.c url_parser.c | out
	gcc $^ ${DEPS} -o $@

${OUT}/tests-network: tests/tests-network.c network.c util.c url_parser.c | out
	gcc $^ ${DEPS} -o $@

${OUT}/tests-util: tests/tests-util.c util.c | ${OUT} ${OUT}/tests/files
	gcc $^ ${DEPS} -o $@

${OUT}/tests/files: tests/files
	cp -r $^ out/

${OUT}:
	mkdir -p $@
