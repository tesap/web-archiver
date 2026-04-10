
## Description

This is a collection of Unix-style programs that being combined allow to **recursively crawl given URL**. This means downloading HTML pages + corresponding resources (images, CSS, JS scripts).

Code: Written in modern C, compiled with g++ (C++ compiler)

## Usage

For example usage see `./all.sh` file that combines all three programs into a complete pipeline.
You can learn more by looking up for `--help` outputs of compiled executables.

## TODO

- Common
    - [x] Fix long connect time
    - [ ] `download_http`: Maybe we can cache socket connections in redirection cases to avoid double work
    - [ ] Test `cached_download_http` and `download_http` with mocked network
    - [ ] Improve URL relpath resolving, f.e. `doc.rust-lang.org/stable/rustc/../cargo/index.html`
- `hrefs_crawler`:
    - [x] Replace dependency on libcurl with manual HTTP/SSL file download with libssl + sockets
    - [x] Parse content type header to filter out non-html files
    - [x] Fs-caching of downloaded pages
        - [~] Improve it with caching by HashMap struct.
    - [x] Add (URL -> path) parsing + tests 
    - [x] Test hrefs_crawler callback logic
- `cached_curl`: Downloads a given URL to a given path
    - [x] Implement writing file to FS.
    - [ ] Rely on HTTP headers to better determine dir/file category (content-type header). Make it a file only in case non text/html
- `links_replacer`
    - [x] Implement links replacement to relative paths in HTML page
    - [x] Test `replace_links`
- `master`: A master process that combines all programs together
    - [ ] Implement simple logic of complete chain
    - [ ] Support stateful
    - [ ] Support service + public API
