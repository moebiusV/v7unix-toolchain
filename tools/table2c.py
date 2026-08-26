#!/usr/bin/env python3
"""
table2c.py -- convert V7 c1's table.o (a PDP-11 a.out) into C struct initializers.

The PDP-11 object is self-contained: the text section holds null-terminated
bytecode strings plus `struct optab` arrays, the data section holds the four
dispatch tables (`_regtab`/`_efftab`/`_cctab`/`_sptab`).  Every pointer in it is
a 16-bit offset (RTEXT-relocated) into the text section, so we can walk the whole
thing from the symbol table + the struct layouts and re-emit it as host-layout C:

      struct optab { char tabdeg1, tabtyp1, tabdeg2, tabtyp2; char *tabstring; };
      struct table { int  tabop;                        struct optab *tabp;   };

Usage:  table2c.py table.o > table.c
"""

import struct
import sys

TEXT, DATA = 0o2, 0o3
EXTERN = 0o40


def read16(b, o):
    return struct.unpack_from('<H', b, o)[0]


def parse(path):
    data = open(path, 'rb').read()
    magic, tsize, dsize, bsize, ssize, entry, pad, flags = struct.unpack('<8H', data[:16])
    assert magic == 0o407, f"bad magic {magic:o}"
    text = data[16:16 + tsize]
    dat = data[16 + tsize:16 + tsize + dsize]
    sym_off = 16 + tsize + dsize + (tsize + dsize)   # skip the reloc words (1 per word)
    nsym = ssize // 12
    syms = []
    for i in range(nsym):
        o = sym_off + i * 12
        name = data[o:o + 8].split(b'\0')[0].decode('latin1')
        stype = data[o + 8]
        sval = read16(data, o + 10)
        syms.append((name, stype, sval))
    return text, dat, tsize, syms


def cident(name):
    """Turn a V7 asm symbol name into a legal C identifier."""
    return name  # V7 symbols are already C-legal (letters/digits/underscore)


def esc_bytes(b):
    """Emit a byte run as a C string literal (returns the source text)."""
    out = []
    for x in b:
        if x == 0x22:            # "
            out.append('\\"')
        elif x == 0x5C:          # backslash
            out.append('\\\\')
        elif x == 0x0A:
            out.append('\\n')
        elif x == 0x09:
            out.append('\\t')
        elif 0x20 <= x < 0x7F:
            out.append(chr(x))
        else:
            out.append('\\%03o' % x)   # octal escape: unambiguous in a string
    return '"' + ''.join(out) + '"'


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else '/tmp/table.o'
    text, dat, tsize, syms = parse(path)

    # offset -> name, for TEXT labels.  Keep the most meaningful name when
    # several symbols share an address (e.g. `fas1` and its `L40` alias).
    text_name = {}
    for name, stype, sval in syms:
        if (stype & 0o37) == TEXT:
            cur = text_name.get(sval)
            if cur is None:
                text_name[sval] = name
            else:
                # prefer a non-`L%d` name (subroutine) over a generated label
                if name[:1] != 'L' and cur[:1] == 'L':
                    text_name[sval] = name
                elif name[:1] != 'L' and name[:1] != '_' and cur[:1] == 'L':
                    text_name[sval] = name

    # data symbols -> table offsets
    tables = {}
    for name, stype, sval in syms:
        if (stype & 0o37) == DATA and name.startswith('_'):
            tables[name] = sval - tsize
    ordered = ['_regtab', '_efftab', '_cctab', '_sptab']
    data_end = len(dat)
    ranges = []
    for i, t in enumerate(ordered):
        start = tables[t]
        end = tables[ordered[i + 1]] if i + 1 < len(ordered) else data_end
        ranges.append((t, start, end))

    # walk the dispatch tables -> collect optab offsets (tabp targets)
    optab_offsets = set()
    table_entries = {}          # table name -> list of (tabop, optab_offset)
    for tname, start, end in ranges:
        entries = []
        o = start
        while o + 4 <= end:
            tabop = read16(dat, o)
            if tabop == 0:
                break
            tabp = read16(dat, o + 2)
            entries.append((tabop, tabp))
            optab_offsets.add(tabp)
            o += 4
        table_entries[tname] = entries

    # walk each optab array -> collect (entry) list + tabstring offsets
    optabs = {}                 # offset -> list of (d1,t1,d2,t2,str_off)
    string_offsets = set()
    for off in sorted(optab_offsets):
        entries = []
        o = off
        while o + 6 <= tsize:
            d1 = text[o]
            if d1 == 0:
                break
            t1, d2, t2 = text[o + 1], text[o + 2], text[o + 3]
            str_off = read16(text, o + 4)
            entries.append((d1, t1, d2, t2, str_off))
            string_offsets.add(str_off)
            o += 6
        optabs[off] = entries

    # extract each string (null-terminated) from the text section
    strings = {}                # offset -> bytes (excluding null)
    for off in sorted(string_offsets):
        if off == 0 and off not in text_name:
            # null pointer target (a real null tabstring) -- skip
            continue
        e = text.index(b'\0', off)
        strings[off] = text[off:e]

    # ---------------- emit ----------------
    print("/* generated by table2c.py -- do not edit */")
    print("#include \"c1.h\"")
    print()

    def resolve(off):
        """16-bit text offset -> C identifier (or `0` for null)."""
        name = text_name.get(off)
        if name is None:
            sys.stderr.write(f"WARNING: text offset {off} has no symbol\n")
            return '0'
        return cident(name)

    # strings first (referenced by optabs)
    for off in sorted(strings):
        name = resolve(off)
        if name == '0':
            continue
        print(f"static char {name}[] = {esc_bytes(strings[off])};")
    print()

    # optab arrays
    for off in sorted(optabs):
        name = text_name.get(off, 'optab_%d' % off)
        name = cident(name)
        entries = optabs[off]
        print(f"static struct optab {name}[] = {{")
        for d1, t1, d2, t2, s in entries:
            print(f"\t{{{d1}, {t1}, {d2}, {t2}, {resolve(s)}}},")
        print("\t{0, 0, 0, 0, 0},")   # deg1==0 terminator
        print("};")
        print()

    # dispatch tables (V7 asm names them `_regtab` etc.; C drops the underscore)
    for tname in ordered:
        cname = tname.lstrip('_')
        entries = table_entries[tname]
        print(f"struct table {cname}[] = {{")
        for tabop, tabp in entries:
            name = text_name.get(tabp, '0')
            print(f"\t{{{tabop}, {cident(name)}}},")
        print("\t{0, 0},")   # tabop==0 terminator
        print("};")
        print()


if __name__ == '__main__':
    main()
