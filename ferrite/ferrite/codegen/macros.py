from __future__ import annotations
from typing import Any, Callable, List

from enum import Enum

from ferrite.utils.strings import quote

from ferrite.codegen.utils import hash_str, indent, to_ident


def io_read_type() -> str:
    return "io::ReadExact"


def io_write_type() -> str:
    return "io::WriteExact"


def io_result_type(tys: str = "std::monostate") -> str:
    return f"Result<{tys}, io::Error>"


def monostate() -> str:
    return "std::monostate{}"


def try_unwrap(expr: str, op: Callable[[str], str] | None = None) -> List[str]:
    res = f"res_{hash_str((op('') if op is not None else '') + expr)[:4]}"
    return [
        f"auto {res} = {expr};",
        f"if ({res}.is_err()) {{ return Err({res}.unwrap_err()); }}",
        *([op(f"{res}.unwrap()")] if op is not None else []),
    ]


def stream_read(stream: str, dst_ptr: str, size: int | str) -> str:
    return f"{stream}.read_exact(reinterpret_cast<uint8_t *>({dst_ptr}), {size})"


def stream_write(stream: str, src_ptr: str, size: int | str) -> str:
    return f"{stream}.write_exact(reinterpret_cast<const uint8_t *>({src_ptr}), {size})"


class ErrorKind(Enum):
    UNEXPECTED_EOF = 1,
    INVALID_DATA = 2,

    def cpp_name(self) -> str:
        pref = "io::ErrorKind::"
        if self == ErrorKind.UNEXPECTED_EOF:
            post = "UnexpectedEof"
        elif self == ErrorKind.INVALID_DATA:
            post = "InvalidData"
        return f"{pref}{post}"


def io_error(kind: ErrorKind, desc: str | None = None) -> str:
    if desc is not None:
        msg_arg = f", \"{quote(desc)}\""
    else:
        msg_arg = ""
    return f"io::Error{{{kind.cpp_name()}{msg_arg}}}"
