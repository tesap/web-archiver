#!/bin/bash

OUT=res
EXEC_OUT=../out
START_URL=archlinux.org
# START_URL=https://doc.rust-lang.org/stable/rustc/index.html

CRAWL_DEPTH=1
REQUEST_PERIOD=1000
REQUEST_TIMEOUT=3000

CACHE_TTL=1000

mkdir $OUT
cd $OUT

if [[ ! -f urls.txt ]]; then
    # Crawls URLs list
    echo -e "$EXEC_OUT/hrefs_crawler
        -a $START_URL
        -d $CRAWL_DEPTH
        -p $REQUEST_PERIOD
        --cache-ttl $CACHE_TTL
        --filter-same-domain
        --save > urls.txt"
    $EXEC_OUT/hrefs_crawler \
        -a $START_URL \
        -d $CRAWL_DEPTH \
        -p $REQUEST_PERIOD \
        --cache-ttl $CACHE_TTL \
        --filter-same-domain \
        --save > urls.txt
fi

# Download each URL to filesystem
cat urls.txt | while read -r line; do
    url=$(echo $line | awk '{ print $2 }')
    type=$(echo $line | awk '{ print $1 }')
    if [[ -z $url ]]; then
        echo "EMPTY URL: $line"
        echo "URL: $url"
    fi
    echo -e "$EXEC_OUT/cached_curl
        -a "$url"
        --type-hint $type
        --cache-ttl $CACHE_TTL
        --timeout $REQUEST_TIMEOUT
        --save"

    $EXEC_OUT/cached_curl \
        -a "$url" \
        --type-hint $type \
        --cache-ttl $CACHE_TTL \
        --timeout $REQUEST_TIMEOUT \
        --save

    res=$?
    if [[ $res == 0 ]]; then
        sleep $(echo "scale=2; $REQUEST_PERIOD/1000" | bc)
    fi
    if [[ $res == -1 ]]; then
        sleep $(echo "scale=2; $REQUEST_PERIOD/1000" | bc)
    fi
done

# Replace links for proper relative paths
find . -type f | while read -r line; do
    if [[ $line != "./urls.txt" ]]; then
        echo "$EXEC_OUT/links_replacer -f $line | sponge $line"
        $EXEC_OUT/links_replacer -f $line | sponge $line
    fi
done


