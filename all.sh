#!/bin/bash


BASE_URL="https://doc.rust-lang.org/nomicon/vec/vec-drain.html"
DEPTH=2
OUT_DIR="out"
HREFS_LIST_FILE="${OUT_DIR}/out.urls"

mkdir -p $OUT_DIR

./hrefs_crawler/main.py \
    $BASE_URL \
    -o $HREFS_LIST_FILE \
    -d $DEPTH
    # --filter-host-preserve \
    # --filter-path-regex "^/nomicon.*$" \


cat $HREFS_LIST_FILE | while read url; do
  ./single_page/main.py \
      $url \
      -o $OUT_DIR
      # -f

done
