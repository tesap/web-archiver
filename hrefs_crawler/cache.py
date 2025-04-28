import urllib

from typing import Tuple, Dict, Any

url_cache: Dict[Tuple[str, str], Any] = dict()

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
    return cache_decorator(url_ident, url_cache)(func)

