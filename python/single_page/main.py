#!python

import requests
import re
import os
import sys
import hashlib
import argparse

from bs4 import BeautifulSoup

from typing import Iterable
from urllib.parse import urljoin, urlparse, ParseResult

from cache import cache_by_parsed_url
from fs_cache import fs_cache_by_url_parsed, fs_cache_by_url
from href import HrefType, Href, is_resource 
from common import \
    file_write, \
    mkdir, \
    create_filepath_dirs, \
    get_url_local_path, \
    get_href_relpath, \
    DEBUG_OUT


def soup_iter_hrefs(soup) -> Iterable[Href]:
    for i in soup.find_all('iframe'):
        v = i.get('src')
        if v:
            yield Href(HrefType.Iframe, i, 'src', v)
    
    for i in soup.find_all('a'):
        v = i.get('href')
        if v:
            yield Href(HrefType.Html, i, 'href', v)

    # Extract resources from img tags
    print("=== IMAGES", file=DEBUG_OUT)
    for i in soup.find_all('img'):
        v = i.get('src')
        if v:
            yield Href(HrefType.Image, i, 'src', v)

    # Extract resources from script tags
    print("=== SCRIPTS", file=DEBUG_OUT)
    for i in soup.find_all('script'):
        v = i.get('src')
        if v:
            yield Href(HrefType.Script, i, 'src', v)

    # Extract resources from link tags
    print("=== CSS", file=DEBUG_OUT)
    for i in soup.find_all('link'):
        v = i.get('href')
        if not v:
            continue

        is_stylesheet = i.get('rel') in ['stylesheet', 'text/css']
        is_css = i.get('type') == 'text/css' or i['href'].endswith('.css')
        is_fav = i.get('rel') == ['icon']

        if is_stylesheet or is_css or is_fav:
            yield Href(HrefType.Style, i, 'href', v)

    # TODO how to replace inside CSS?
    # Extract resources links from CSS rules
    # print("=== STYLE")
    # for style in soup.find_all('style'):
    #     def extract_css_urls(css_content: str) -> List[str]:
    #         return re.compile(r'url\([\'"]?([^\'"()]+)[\'"]?\)').findall(css_content)
    #
    #     if style.string:
    #         yield from extract_css_urls(style.string)



def write_url_locally(parsed_url: ParseResult, out_dir: str, data: bytes):
    url = parsed_url.geturl()
    path = get_url_local_path(parsed_url, out_dir)
    
    create_filepath_dirs(path)

    print(url, "\t->", path, file=DEBUG_OUT)
    file_write(
        path,
        data
    )

@cache_by_parsed_url
def download_url(parsed_url: ParseResult, out_dir: str):
    url = parsed_url.geturl()
    try:
        resp = requests.get(url, timeout=3)
    except requests.exceptions.ConnectionError:
        print("Error connection")
        return

    if not resp.ok:
        print("Error retrieving: ", url)
        return

    write_url_locally(parsed_url, out_dir, resp.content)


def download_and_inject_hrefs(soup, url: str, out_dir: str):
    parsed_url = urlparse(url)

    for href in soup_iter_hrefs(soup):
        parsed_href = urlparse(href.value)

        is_local_href = not parsed_href.netloc

        if is_local_href:
            full_href_parsed = urlparse(urljoin(url, href.value))
        else:
            full_href_parsed = parsed_href

        if is_resource(href):
            try:
                download_url(full_href_parsed, out_dir)
            except Exception as e:
                print(f"--- Exc: {e}")

        href.soup_obj[href.key] = get_href_relpath(parsed_url, full_href_parsed)
        
def main(url: str, out_dir: str):
    mkdir(out_dir)

    resp = requests.get(url)
    soup = BeautifulSoup(resp.text, 'html.parser')

    download_and_inject_hrefs(soup, url, out_dir)

    write_url_locally(urlparse(url), out_dir, str(soup).encode())

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Download HTTP page by given URL with resources, allowing it to be viewed offline')
    parser.add_argument('url')
    parser.add_argument('-o', '--output', metavar="PATH", required=True, help='Output directory')
    parser.add_argument('-f', '--force-download', action="store_true", help='Ignore already present files and force their re-download')
    parser.add_argument('-v', '--verbose', action="store_true", help='Print debug output')
    
    args = parser.parse_args()

    VERBOSE = args.verbose

    if not VERBOSE:
        DEBUG_OUT = open(os.devnull, 'w')

    URL = args.url
    OUTPUT_DIR = args.output
    FORCE_DOWNLOAD = args.force_download

    if not FORCE_DOWNLOAD:
        download_url = fs_cache_by_url_parsed(download_url)
        main = fs_cache_by_url(main)

    main(URL, OUTPUT_DIR)



