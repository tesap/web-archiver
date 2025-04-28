import re

from dataclasses import dataclass
from urllib.parse import ParseResult

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
