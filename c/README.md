
## Description

This is a collection of Unix-style programs that being combined allow to **recursively crawl given URL**. This means downloading HTML pages + corresponding resources (images, CSS, JS scripts).

Code: Written in modern C, compiled with g++ (C++ compiler)

## TODO

- Common
    - [ ] Fix long connect time
    - [ ] network.c: Maybe we can cache socket connections in redirection cases to avoid double work
- `hrefs_crawler`:
    - [x] Replace dependency on libcurl with manual HTTP/SSL file download with libssl + sockets
    - [ ] Parse content type header to filter out non-html files
    - [ ] Caching for downloaded URIs. Track HashMap of crawled URLs to avoid redundant TCP requests
- `single_page`:
    - [x] Implement parsing of all resources hrefs. It is more complicated than plain hrefs capture
    - [ ] Add tests for parsing
    - [ ] Implement writing files to FS.
    - [ ] Caching of downloaded URIs. Track HashMap of retreived files 
    - [ ] Caching of files
