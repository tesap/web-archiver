#!/bin/bash


# BASE_URL="https://doc.rust-lang.org/nomicon/vec/vec-drain.html"
DEPTH=3
OUT_DIR="out"
HREFS_LIST_FILE="${OUT_DIR}/out.urls"

mkdir -p $OUT_DIR

function crawl_url() {
    echo "======= $1"
    python hrefs_crawler/main.py \
        $1 \
        -o $HREFS_LIST_FILE \
        -d $DEPTH \
        --filter-host-preserve
        # --filter-path-regex "^/nomicon.*$" \


    cat $HREFS_LIST_FILE | while read page_href; do
      python single_page/main.py \
          $page_href \
          -o $OUT_DIR \
          # -f
    done
}

while read line
do
  if [[ -n $line ]]; then
    crawl_url $line
  fi
done < "${1:-/dev/stdin}"
