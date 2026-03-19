
## Description

This is a collection of Unix-style programs that being combined allow to **recursively crawl given URL**. This means downloading HTML pages + corresponding resources (images, CSS, JS scripts).

Code: Written in modern C, compiled with g++ (C++ compiler)

## TODO

- Common
    - [x] Fix long connect time
    - [ ] network.c: Maybe we can cache socket connections in redirection cases to avoid double work
    - [ ] Add logging functions
    - [ ] Check for unfreed mallocs
- `hrefs_crawler`:
    - [x] Replace dependency on libcurl with manual HTTP/SSL file download with libssl + sockets
    - [x] Parse content type header to filter out non-html files
    - [ ] Caching for downloaded URIs. Track HashMap of crawled URLs to avoid redundant TCP requests
    - [x] Fs-caching of downloaded pages
    - [x] Add (URL -> path) parsing + tests 
- `cached_curl`: Downloads a given URL to a given path
    - [x] Implement writing file to FS.
    - [ ] Rely on HTTP headers to better determine dir/file category (content-type header). Make it a file only in case non text/html
    - [-] Try replacing it with cURL
- `html_paths_corrector`
    - [ ] Implement links replacement to relative paths in HTML page
- `master`: A master process that combines all programs together
    - [ ] Implement simple logic of complete chain
    - [ ] Support stateful
    - [ ] Support service + public API
