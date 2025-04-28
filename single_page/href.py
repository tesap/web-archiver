
from dataclasses import dataclass
from enum import Enum, auto
from typing import Any


class HrefType(Enum):
    Iframe = auto()
    Html = auto()
    Image = auto()
    Style = auto()
    Script = auto()



@dataclass
class Href:
    href_type: HrefType
    soup_obj: Any
    key: str
    value: str


def is_page_href(h: Href) -> bool:
    return h.href_type in [HrefType.Iframe, HrefType.Html]

def is_resource(h: Href) -> bool:
    return not is_page_href(h)
