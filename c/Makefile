
.PHONY: out

OUT=out
FLAGS=-Werror -Wreturn-type -fno-exceptions -fno-rtti

all: ${OUT}/hrefs_crawler ${OUT}/tests-url_parser ${OUT}/tests-network ${OUT}/tests-util

${OUT}/hrefs_crawler: hrefs_crawler.c url_parser.c html_parser.c network.c util.c | out
	gcc $^ -o $@
    
${OUT}/tests-url_parser: tests/tests-url_parser.c url_parser.c | out
	gcc $^ -o $@

${OUT}/tests-network: tests/tests-network.c network.c util.c url_parser.c | out
	gcc $^ -o $@

${OUT}/tests-util: tests/tests-util.c util.c | ${OUT} ${OUT}/tests/files
	gcc $^ -o $@

${OUT}/tests/files: tests/files
	cp -r $^ out/

${OUT}:
	mkdir -p $@
