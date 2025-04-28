import requests
import urllib
import sys

from bs4 import BeautifulSoup
from urllib.parse import urlparse, urljoin, ParseResult
from typing import List, Tuple, Set, Iterable

from cache import cache_by_url
from crawl_filter import should_crawl_url, CrawlFilter, CrawlRegexPath, CrawlRegexHost, CrawlBaseHost

def extract_page_hrefs(html_text: str) -> Iterable[str]:
    """
    Parse HTML to get list of raw hrefs
    """
    soup = BeautifulSoup(html_text, 'html.parser')

    for f in soup.find_all('iframe'):
        href = f.get('src')
        if href:
            yield href
    
    for a in soup.find_all('a'):
        href = a.get('href')
        if href:
            yield href

def parse_page_href(href: str, base_url: str) -> ParseResult:
    """
    Process given href to get its full URL information as a dataclass
    """
    href_parsed = urllib.parse.urlparse(href)

    # If referencing page on the same host relatively
    if not href_parsed.netloc:
        href_abs = urljoin(base_url, href)
        href_parsed = urllib.parse.urlparse(href_abs)

    # Remove extra params that do not identify page location
    href_parsed = href_parsed._replace(params="", query="", fragment="")

    return href_parsed

@cache_by_url
def obtain_page_hrefs(url: str, crawl_filter: CrawlFilter) -> List[ParseResult]:
    url_parsed = urllib.parse.urlparse(url)
    # Making GET request
    resp = requests.get(url)

    if not resp.ok:
        print(f"ERROR Reteieving {url}: {resp.status_code}")
        return list()

    # Parsing response to get all references
    hrefs = extract_page_hrefs(resp.text)
    parsed = (parse_page_href(href, url) for href in hrefs)

    # Filter them according to crawl_filter
    return (p for p in parsed if should_crawl_url(p, url_parsed, crawl_filter))

def recursive_obtain_page_hrefs(url: str, depth, crawl_filter: CrawlFilter):
    result = []
    url_parsed = urllib.parse.urlparse(url)

    if depth <= 0:
        return list()

    hrefs = obtain_page_hrefs(url, crawl_filter)

    if depth == 1:
        result = list(hrefs)
        result.append(url_parsed)
        return result

    for i in hrefs:
        print(depth, i.geturl())
        children = recursive_obtain_page_hrefs(i.geturl(), depth - 1, crawl_filter)
        result.extend(children)

    result.append(url_parsed)
    return result

def write_to_file(filename: str, urls: Iterable[ParseResult]):
    with open(filename, "w") as fout:
        for i in urls:
            fout.write(i.geturl() + "\n")


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description='Recursively crawl HTTP hrefs by given initial URL')
    parser.add_argument('url')
    parser.add_argument('-d', '--depth', type=int, default=1, help='Maximum depth of hrefs to follow')
    parser.add_argument('-o', '--output', required=True, help='Output file path')
    parser.add_argument('-q', '--quiet', action="store_true", help='Suppress debug output')
    parser.add_argument('--filter-host-preserve', action="store_true", help='Crawl within base URL host')
    parser.add_argument('--filter-host-regex', metavar="STRING", help='Narrow crawling scope by URL host regex')
    parser.add_argument('--filter-path-regex', metavar="STRING", help='Narrow crawling scope by URL path regex')
    
    args = parser.parse_args()

    if args.quiet:
        import os
        devnull = open(os.devnull, 'w')
        sys.stdout = devnull
    
    URL = args.url
    DEPTH = args.depth
    if args.filter_host_preserve:
        FILTER_HOST = CrawlBaseHost()
    elif args.filter_host_regex:
        FILTER_HOST = CrawlRegexHost(args.filter_host_regex)
    else:
        FILTER_HOST = None

    if args.filter_path_regex:
        FILTER_PATH = CrawlRegexPath(args.filter_path_regex)
    else:
        FILTER_PATH = None

    FILTER = CrawlFilter(FILTER_HOST, FILTER_PATH)
    print(FILTER)

    l = recursive_obtain_page_hrefs(URL, DEPTH, FILTER)
    l = set(l)
    l = sorted(list(l))
    print("Crawled URLs: ", len(l))

    write_to_file(args.output, l)

