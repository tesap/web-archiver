
OUT=out

all: hrefs_crawler

hrefs_crawler: hrefs_crawler.c
	g++ -Werror -Wreturn-type $^ -lcurl -o $@
