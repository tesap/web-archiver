import urllib

from typing import Tuple, Dict, Any
from urllib.parse import ParseResult

url_cache: Dict[Tuple[str, str], Any] = dict()
url_parsed_cache: Dict[str, Any] = dict()

def cache_decorator(args_hash_func, cache_data: Dict):
    def decorator(func):
        def wrapper(*args, **kwargs):
            cache_key = args_hash_func(*args, **kwargs)
            if cache_key in cache_data:
                # Return found from cache
                print("CACHE HIT: Process memory")
                return cache_data[cache_key]
            else:
                # Save to cache
                res = func(*args, **kwargs)
                cache_data[cache_key] = res
                return res

        return wrapper

    return decorator

def url_hash(url, *args, **kwargs) -> Tuple[str, str]:
    url_parsed = urllib.parse.urlparse(url)
    return url_parsed.netloc, url_parsed.path

def url_parsed_hash(url_parsed: ParseResult, *args, **kwargs) -> str:
    return hash(url_parsed)

def cache_by_url(func):
    return cache_decorator(url_hash, url_cache)(func)

def cache_by_parsed_url(func):
    return cache_decorator(url_parsed_hash, url_parsed_cache)(func)

