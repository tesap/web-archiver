

## TODO

- `hrefs_crawler`:
    - [x] Replace dependency on libcurl with manual HTTP/SSL file download with libssl + sockets
    - Parse content type header to filter out non-html files
    - Track HashMap of crawled URLs to avoid redundant TCP requests
- `single_page`:
    - Implement parsing of all resources hrefs. It is more complicated than plain hrefs capture
        - Add tests for parsing
    - Track HashMap of retreived files 
    - Implement writing files to FS.
