from __future__ import annotations
import re
import zlib

def to_ident(text: str, _pat = re.compile("[^a-zA-Z0-9_]")):
    ident = re.sub(_pat, "_", text)
    if ident != text:
        hash = zlib.adler32(text.encode("utf-8"))
        ident += f"_{hash:0{8}x}"
    return ident
