import os

from pathlib import Path
from urllib.parse import ParseResult

# ==== FS ====
def file_write(path: str, data: bytes) -> bool:
    try:
        with open(path, 'wb') as fout:
            fout.write(data)
    except IOError as e:
        print(f"Failed to write: {path}")

def mkdir(d: str):
    if not os.path.exists(d):
        Path(d).mkdir(parents=True, exist_ok=True)

def get_filepath_dirs(p: str):
    p = p.rstrip("/")
    return p[:p.rindex("/")]

def create_filepath_dirs(path: str):
    mkdir(get_filepath_dirs(path))

# ==== URLs ====

def is_url_path_dir(url_parsed: ParseResult) -> bool:
    p = url_parsed.path
    rind = p.rindex("/") if "/" in p else 0

    return p[rind:].count(".") == 0

def get_url_local_path(url_parsed: ParseResult, out_dir: str = ""):
    local_path = os.path.join(out_dir, url_parsed.netloc, url_parsed.path.lstrip("/"))

    if is_url_path_dir(url_parsed):
        local_path = os.path.join(local_path, "index.html")
    return local_path

def get_href_relpath(cur_parsed: ParseResult, ref_parsed: ParseResult) -> str:
    cur_path = get_url_local_path(cur_parsed)
    cur_path = get_filepath_dirs(cur_path)

    ref_path = get_url_local_path(ref_parsed)

    # return os.path.relpath(cur_path, ref_path)
    return os.path.relpath(ref_path, cur_path)

