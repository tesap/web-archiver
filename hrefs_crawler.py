from typing import List, Tuple, Set, Dict, Any, Optional, Iterable
import requests
import urllib
import re
from urllib.parse import urlparse, urljoin, ParseResult
from bs4 import BeautifulSoup

from dataclasses import dataclass

# === 

@dataclass(frozen=True)
class CrawlBaseHost:
    pass

@dataclass(frozen=True)
class CrawlRegexHost:
    host_regex: str = ".*"


CrawlHostFilter = CrawlBaseHost | CrawlRegexHost | None

@dataclass(frozen=True)
class CrawlRegexPath:
    path_regex: str = ".*"

CrawlPathFilter = CrawlRegexPath | None

@dataclass(frozen=True)
class CrawlFilter:
    host_filter: CrawlHostFilter
    path_filter: CrawlPathFilter

# === 

def is_regex_match(s, regex) -> bool:
    regex_pattern = re.compile(regex)
    return regex_pattern.search(s)


def should_crawl_url(url: ParseResult, base_url: ParseResult, crawl_filter: CrawlFilter) -> bool:
    """
    Determine if a URL should be crawled based on host and path filters.
    
    Args:
        url: Parsed URL
        base_url: Parsed base URL, for which the current crawling is going on
        crawl_filter: Combined host and path filtering rules
    
    Returns:
        bool: True if URL matches all filters, False otherwise
    """
    # Check host filter
    host_allowed = True
    match crawl_filter.host_filter:
        case CrawlBaseHost():
            host_allowed = (url.netloc == base_url.netloc)
        case CrawlRegexHost(host_regex):
            host_allowed = is_regex_match(url.netloc, host_regex)
        case None:
            host_allowed = True
        case _:
            raise ValueError(f"Unexpected host_filter: {crawl_filter.host_filter}")
    
    # Check path filter
    path_allowed = True
    match crawl_filter.path_filter:
        case CrawlRegexPath(path_regex):
            path_allowed = is_regex_match(url.path or "/", path_regex)
        case None:
            path_allowed = True
        case _:
            raise ValueError(f"Unexpected path_filter: {crawl_filter.path_filter}")
    
    return host_allowed and path_allowed


global_cache: Dict[Tuple[str, str], Any] = dict()

def cache_decorator(cache_ident_func, cache_data: Dict):
    def decorator(func):
        def wrapper(*args, **kwargs):
            cache_ident = cache_ident_func(*args, **kwargs)
            if cache_ident in cache_data:
                # Return found from cache
                print("CACHE HIT")
                return cache_data[cache_ident]
            else:
                # Save to cache
                res = func(*args, **kwargs)
                cache_data[cache_ident] = res
                return res

        return wrapper

    return decorator

def url_ident(url, *args, **kwargs) -> Tuple[str, str]:
    url_parsed = urllib.parse.urlparse(url)
    return url_parsed.netloc, url_parsed.path

def cache_by_url(func):
    return cache_decorator(url_ident, global_cache)(func)

def extract_page_hrefs(html_text: str) -> Iterable[str]:
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
    href_parsed = urllib.parse.urlparse(href)

    # Referencing page on the same host relatively
    if not href_parsed.netloc:
        href_abs = urljoin(base_url, href)
        href_parsed = urllib.parse.urlparse(href_abs)

    href_parsed = href_parsed._replace(params="", query="", fragment="")

    return href_parsed

@cache_by_url
def obtain_page_hrefs(url: str, crawl_filter: CrawlFilter) -> List[ParseResult]:
    url_parsed = urllib.parse.urlparse(url)
    resp = requests.get(url)

    if not resp.ok:
        print(f"ERROR Reteieving {url}: {resp.status_code}")
        return list()

    hrefs = extract_page_hrefs(resp.text)
    parsed = (parse_page_href(href, url) for href in hrefs)

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
    URL = "https://doc.rust-lang.org/nomicon/vec/vec-drain.html"
    DEPTH = 99
    FILTER = CrawlFilter(CrawlBaseHost(), CrawlRegexPath("^/nomicon.*$"))
    print(FILTER)

    l = recursive_obtain_page_hrefs(URL, DEPTH, FILTER)
    print("LEN: ", len(l))
    l = set(l)
    l = sorted(list(l))
    print("LEN: ", len(l))

    write_to_file("out.txt", l)

