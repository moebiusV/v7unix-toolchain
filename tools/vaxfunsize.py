#!/usr/bin/env python3
"""
vaxfunsize -- compare two VAX assembly listings function by function with a
rough model of instruction lengths, to see where a compiler is bigger.

    vaxfunsize.py a.s b.s [top]

The model counts the opcode byte and each operand specifier (register 1,
literal 1, immediate 1+size, byte/word/long displacement 2/3/5, symbol 5, index
+1, deferred modes as their plain forms) and 2 bytes for a branch.  It is not
the assembler: the assembler's own numbers (tools/vaxdiff.py) are the ones that
count.  This only points at the functions that differ.
"""
import re, sys

SIZE3 = {'b': 1, 'w': 2, 'l': 4, 'f': 4, 'd': 8, 'q': 8}


def opsize(op, mnem):
    t = mnem[-1] if mnem[-1] in 'bwlfdq' else 'l'
    if mnem[-2:-1] in 'bwlfdq' and mnem[-1] in '23':
        t = mnem[-2]
    return SIZE3.get(t, 4)


def spec(o, isz):
    o = o.strip()
    n = 0
    m = re.match(r'(.*)\[(r\d+|ap|fp|sp)\]$', o)
    if m:
        n += 1
        o = m.group(1)
    o = o.lstrip('*')
    if re.match(r'^(r\d+|ap|fp|sp)$', o):
        return n + 1
    if o.startswith('$'):
        v = o[1:]
        try:
            iv = int(v, 0)
            if 0 <= iv <= 63:
                return n + 1
        except ValueError:
            pass
        return n + 1 + isz
    if re.match(r'^\((r\d+|ap|fp|sp)\)\+?$', o) or re.match(r'^-\((r\d+|ap|fp|sp)\)$', o):
        return n + 1
    m = re.match(r'^(-?\w*)\((r\d+|ap|fp|sp)\)$', o)
    if m:
        try:
            d = int(m.group(1), 0)
            return n + (2 if -128 <= d < 128 else 3 if -32768 <= d < 32768 else 5)
        except ValueError:
            return n + 5
    return n + 5


def insn(line):
    line = line.strip()
    if not line or line.startswith(('.', '~')):
        return 0
    m = re.match(r'^(?:L\d+:)?\s*([a-z0-9]+)\s*(.*)$', line)
    if not m:
        return 0
    mn, rest = m.groups()
    if mn.startswith('j') or mn in ('brb', 'brw'):
        return 2 if mn != 'jmp' else 1 + spec(rest.split(',')[-1], 4)
    ops = [x for x in re.split(r',(?![^\[]*\])', rest) if x.strip()]
    isz = opsize(rest, mn)
    n = 1
    for o in ops:
        n += spec(o, isz)
    return n


def funcs(path):
    cur = None
    out = {}
    for line in open(path, errors='replace'):
        m = re.match(r'^(_\w+):\s*\.word', line)
        if m:
            cur = m.group(1)[:8]
            out[cur] = 2
            continue
        m = re.match(r'^(_\w+):$', line.strip())
        if cur and line.strip().startswith(('.globl', '.data', '.comm')):
            pass
        if cur is not None:
            out[cur] += insn(line)
    return out


def main():
    a, b = funcs(sys.argv[1]), funcs(sys.argv[2])
    top = int(sys.argv[3]) if len(sys.argv) > 3 else 12
    rows = []
    for f in sorted(set(a) | set(b)):
        rows.append((b.get(f, 0) - a.get(f, 0), f, a.get(f, 0), b.get(f, 0)))
    tot_a, tot_b = sum(a.values()), sum(b.values())
    print('model total: %d vs %d (%+d)' % (tot_a, tot_b, tot_b - tot_a))
    for d, f, x, y in sorted(rows, reverse=True)[:top]:
        print('%+5d  %-16s %5d %5d' % (d, f, x, y))


if __name__ == '__main__':
    main()
