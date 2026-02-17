
OUT=out
FLAGS=-Werror -Wreturn-type -fno-exceptions -fno-rtti

all: out/hrefs_crawler out/tests-url_parser

${OUT}/hrefs_crawler: hrefs_crawler.c url_parser.c html_parser.c network.c util.c
	gcc $^ -o $@
    
${OUT}/tests-url_parser: tests/tests-url_parser.c url_parser.c
	gcc $^ -o $@
