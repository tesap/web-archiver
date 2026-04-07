#!/bin/bash

OUT=res
EXEC_OUT=../out
START_URL=archlinux.org

CRAWL_DEPTH=2
REQUEST_PERIOD=1000
REQUEST_TIMEOUT=3000

CACHE_TTL=1000

mkdir $OUT
cd $OUT

# Crawls URLs list
$EXEC_OUT/hrefs_crawler \
    -a $START_URL \
    -d $CRAWL_DEPTH \
    -p $REQUEST_PERIOD \
    --cache-ttl $CACHE_TTL \
    --filter-same-domain \
    --save > urls.txt

# Download each URL to filesystem
cat urls.txt | while read -r line; do
    url=$(echo $line | awk -F"\t" '{ print $2 }')
    type=$(echo $line | awk -F"\t" '{ print $1 }')
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
done

# Replace links for proper relative paths
find . -type f | while read -r line; do
    $EXEC_OUT/links_replacer -f $line | sponge $line
done


