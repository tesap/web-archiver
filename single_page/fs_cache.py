import os

from urllib.parse import urlparse

from common import *

def file_exists(path: str):
    return os.path.exists(path)

def fs_cache_decorator(args_to_filepath):
    def decorator(func):
        def wrapper(*args, **kwargs):
            path = args_to_filepath(*args, **kwargs)

            if file_exists(path):
                print("CACHE HIT: Filesystem \t->", path)
                return
            else:
                return func(*args, **kwargs)

        return wrapper
    return decorator


def url_parsed_arg_to_filepath(url_parsed: ParseResult, out_dir: str, *args, **kwargs):
    return get_url_local_path(url_parsed, out_dir)

def fs_cache_by_url_parsed(func):
    return fs_cache_decorator(url_parsed_arg_to_filepath)(func)

def url_arg_to_filepath(url: str, out_dir: str, *args, **kwargs):
    return url_parsed_arg_to_filepath(urlparse(url), out_dir, *args, **kwargs)

def fs_cache_by_url(func):
    return fs_cache_decorator(url_arg_to_filepath)(func)

