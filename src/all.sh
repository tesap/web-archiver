#!/bin/bash

OUT=res
EXEC_OUT=../out
# START_URL=archlinux.org
# START_URL=https://doc.rust-lang.org/stable/rustc/index.html
# START_URL=https://wikipedia.org/
START_URL=$1
URLS_FILE="urls-${START_URL//\//\\}.txt"
echo "START_URL: '$START_URL'"

if [[ -z $START_URL ]]; then
    exit -1
fi

CRAWL_DEPTH=1
REQUEST_PERIOD=1000
REQUEST_TIMEOUT=3000

SLEEP_SEC=$(echo "scale=2; $REQUEST_PERIOD/1000" | bc)

CACHE_TTL=1000

mkdir $OUT
cd $OUT

if [[ ! -f $URLS_FILE ]]; then
    echo "============= STARTING hrefs_crawler ============="
    # Crawls URLs list
    $EXEC_OUT/hrefs_crawler \
        -a $START_URL \
        -d $CRAWL_DEPTH \
        -p $REQUEST_PERIOD \
        --filter-same-domain \
        --save > $URLS_FILE
    res=$?
    if [[ $res != 0 ]]; then
        echo "hrefs_crawler: status $res"
        rm $URLS_FILE
        exit $res
    fi
else 
    echo "============= SKIP hrefs_crawler ============="
fi

# Download each URL to filesystem
cat $URLS_FILE | while read -r line; do
    url=$(echo $line | awk '{ print $2 }')
    type=$(echo $line | awk '{ print $1 }')
    if [[ -z $url ]]; then
        echo "EMPTY URL: $line"
        continue;
    fi
    $EXEC_OUT/cached_curl \
        -a "$url" \
        --type-hint $type \
        --cache-ttl $CACHE_TTL \
        --timeout $REQUEST_TIMEOUT \
        --save

    res=$?
    if [[ $res == 0 ]]; then
        sleep $SLEEP_SEC
    fi
    if [[ $res == -1 ]]; then
        sleep $SLEEP_SEC
        echo "cached_curl: status $res"
    fi
    if [[ $res == 2 ]]; then
        echo "cached_curl: TOO MANY REQUESTS"
        sleep $SLEEP_SEC
        sleep $SLEEP_SEC
        sleep $SLEEP_SEC
        echo "Retrying after long timeout"
        $EXEC_OUT/cached_curl \
            -a "$url" \
            --type-hint $type \
            --cache-ttl $CACHE_TTL \
            --timeout $REQUEST_TIMEOUT \
            --save
    fi
done

# Replace links for proper relative paths
find . -type f | while read -r line; do
    if [[ $line != "./$URLS_FILE" ]]; then
        echo "$EXEC_OUT/links_replacer -f $line | sponge $line"
        $EXEC_OUT/links_replacer -f $line | sponge $line
    fi
    res=$?
    if [[ $res == -1 ]]; then
        exit $res
    fi
done

echo "============= COMPLETED SUCCESSFULLY ============="
exit 0
