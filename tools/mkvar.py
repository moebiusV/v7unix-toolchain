#!/usr/bin/env python3
#
# mkvar — create a V7 (PDP-11) `ar` archive from a list of object files.
#
# V7's archive format (see /usr/include/ar.h and /usr/src/cmd/ar.c) is not the
# modern GNU/BSD format.  An archive is:
#
#   ARMAG (0177545, i.e. 0xff65 little-endian)             2 bytes
#   then, per member, in order:
#     struct ar_hdr {
#         char ar_name[14];   name, NUL-padded
#         long ar_date;       32-bit mtime, middle-endian (high word first,
#                             each word little-endian)     4 bytes
#         char ar_uid;        1 byte
#         char ar_gid;        1 byte
#         int  ar_mode;       16-bit st_mode, little-endian  2 bytes
#         long ar_size;       32-bit member length, middle-endian 4 bytes
#     }                                                    = 26 bytes
#     the member bytes, padded with one NUL to an even length
#
# A 32-bit value is stored "middle-endian": the high 16-bit word first (little-
# endian), then the low word (little-endian) — the PDP-11's long layout.
#
# Metadata (date/uid/gid/mode) is taken from each member file's stat, exactly
# as V7's ar.c does (movefil(): ar_date=st_mtime, ar_uid=st_uid, ...).  The
# *member bytes* are what a byte-identity check cares about; the timestamp will
# naturally differ from a 1979 image.

import os
import struct
import sys

ARMAG = 0xFF65


def _long_mid(v):
    """Encode a 32-bit value as [hi_word LE, lo_word LE]."""
    return struct.pack("<HH", (v >> 16) & 0xFFFF, v & 0xFFFF)


def _hdr(name, data, st):
    nm = name.encode()[:14]
    h = bytearray()
    h += nm + b"\0" * (14 - len(nm))
    h += _long_mid(int(st.st_mtime))
    h += bytes([st.st_uid & 0xFF, st.st_gid & 0xFF])
    h += struct.pack("<H", st.st_mode & 0xFFFF)
    h += _long_mid(len(data))
    return bytes(h)


def main(argv):
    if len(argv) < 3:
        sys.stderr.write("usage: mkvar.py OUT.a MEMBER.o ...\n")
        return 2
    out = argv[1]
    members = argv[2:]

    buf = bytearray(struct.pack("<H", ARMAG))
    for m in members:
        data = open(m, "rb").read()
        st = os.stat(m)
        buf += _hdr(os.path.basename(m), data, st)
        buf += data
        if len(data) & 1:
            buf += b"\0"
    with open(out, "wb") as f:
        f.write(buf)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
