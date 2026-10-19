#!/usr/bin/env python3
"""
arm64run -- assemble the output of the aarch64 pcc and run it on an emulated AArch64.

    arm64run.py file.s [entry]      (entry defaults to "test")

Calls `entry` (an int function of no arguments), prints the value it returns as a
signed decimal.  There is no libc and no kernel: the program is the one file.

The assembly is read here, not by an assembler program: directives are laid out by
this script (text, data and bss placed one after another, `.comm` in bss), every
instruction goes to keystone one at a time with the addresses of all symbols
available to it, and unicorn runs the result.  Needs the unicorn and keystone
modules (a venv: python3 -m venv V; V/bin/pip install unicorn keystone-engine).
"""
import re, struct, sys
import keystone
import unicorn
from unicorn import arm64_const as A

TEXT = 0x400000
DATA = 0x800000
STACK_TOP = 0x10000000
STACK_SZ = 0x100000
STOP = 0x1000
LENIENT = False     # an undefined symbol is 0x500000 (assembling only, to look for bad instructions)


class Asm:
    def __init__(self):
        self.sym = {}          # name -> (section, offset) then absolute
        self.items = {'text': [], 'data': [], 'bss': []}
        self.size = {'text': 0, 'data': 0, 'bss': 0}
        self.globl = set()

    def align(self, sec, n):
        pad = (-self.size[sec]) % n
        if pad:
            self.items[sec].append(('zero', pad))
            self.size[sec] += pad

    def label(self, sec, name):
        self.sym[name] = (sec, self.size[sec])

    def add(self, sec, kind, arg, size):
        self.items[sec].append((kind, arg))
        self.size[sec] += size


def num(t):
    """int(t, 0) that also takes C-style octal (0377), which Python rejects."""
    t = t.strip()
    m = re.match(r'^(-?)0([0-7]+)$', t)
    if m:
        return int(m.group(1) + m.group(2), 8)
    return int(t, 0)


def strval(s):
    out = bytearray()
    i = 0
    while i < len(s):
        c = s[i]
        if c == '\\':
            i += 1
            c = s[i]
            if c in '01234567':
                j = i
                while j < len(s) and j < i + 3 and s[j] in '01234567':
                    j += 1
                out.append(int(s[i:j], 8))
                i = j
                continue
            out.append({'n': 10, 't': 9, 'r': 13, '\\': 92, '"': 34, 'a': 7, 'b': 8, 'f': 12, 'v': 11, '0': 0}.get(c, ord(c)))
        else:
            out.append(ord(c))
        i += 1
    return bytes(out)


def parse(src):
    a = Asm()
    sec = 'text'
    for raw in src.split('\n'):
        line = raw.split('//')[0].strip()
        while True:
            m = re.match(r'^([A-Za-z_.$][\w.$]*):\s*(.*)$', line)
            if not m:
                break
            a.label(sec, m.group(1))
            line = m.group(2).strip()
        if not line:
            continue
        if line.startswith('.'):
            parts = line.split(None, 1)
            d = parts[0]
            rest = parts[1].strip() if len(parts) > 1 else ''
            if d in ('.text',):
                sec = 'text'
            elif d in ('.data',):
                sec = 'data'
            elif d in ('.bss',):
                sec = 'bss'
            elif d == '.section':
                nm = rest.split(',')[0]
                sec = 'text' if nm.startswith('.text') else 'bss' if nm.startswith('.bss') else 'data'
            elif d in ('.align', '.p2align'):
                a.align(sec, 1 << num(rest.split(',')[0]))
            elif d == '.balign':
                a.align(sec, num(rest.split(',')[0]))
            elif d in ('.global', '.globl', '.local', '.type', '.size', '.ident', '.file', '.weak', '.hidden', '.cfi_startproc', '.cfi_endproc'):
                pass
            elif d == '.comm':
                f = [x.strip() for x in rest.split(',')]
                al = num(f[2]) if len(f) > 2 else 8
                a.align('bss', al)
                a.label('bss', f[0])
                a.add('bss', 'zero', num(f[1]), num(f[1]))
            elif d in ('.lcomm',):
                f = [x.strip() for x in rest.split(',')]
                a.align('bss', 8)
                a.label('bss', f[0])
                a.add('bss', 'zero', num(f[1]), num(f[1]))
            elif d in ('.byte', '.short', '.hword', '.2byte', '.word', '.long', '.4byte', '.quad', '.xword', '.8byte', '.dword', '.int'):
                w = {'.byte': 1, '.short': 2, '.hword': 2, '.2byte': 2, '.word': 4, '.long': 4, '.4byte': 4, '.int': 4,
                     '.quad': 8, '.xword': 8, '.8byte': 8, '.dword': 8}[d]
                for v in rest.split(','):
                    a.add(sec, 'val', (v.strip(), w), w)
            elif d in ('.zero', '.space', '.skip'):
                n = num(rest.split(',')[0])
                a.add(sec, 'zero', n, n)
            elif d in ('.ascii', '.string', '.asciz'):
                m2 = re.match(r'^"(.*)"$', rest)
                b = strval(m2.group(1)) + (b'\0' if d != '.ascii' else b'')
                a.add(sec, 'bytes', b, len(b))
            else:
                raise SystemExit('arm64run: unknown directive: ' + line)
        else:
            if sec != 'text':
                raise SystemExit('arm64run: instruction outside text: ' + line)
            a.add('text', 'ins', line, 4)
    return a


def evalexpr(e, syms):
    e = e.strip()
    m = re.match(r'^(.*?)\s*([+-])\s*(\d+|0x[0-9a-fA-F]+)$', e)
    if m and not re.match(r'^-?\d', e):
        return evalexpr(m.group(1), syms) + (1 if m.group(2) == '+' else -1) * int(m.group(3), 0)
    if re.match(r'^-?(0x[0-9a-fA-F]+|\d+)$', e):
        return int(e, 0)
    if e in syms:
        return syms[e]
    if LENIENT:
        return 0x500000
    raise SystemExit('arm64run: undefined symbol: ' + e)


def build(a):
    text_sz = a.size['text']
    data_base = DATA
    bss_base = DATA + ((a.size['data'] + 15) & ~15) + 16
    base = {'text': TEXT, 'data': data_base, 'bss': bss_base}
    syms = {n: base[s] + o for n, (s, o) in a.sym.items()}
    # data and bss images
    img = {}
    for s in ('data',):
        b = bytearray()
        for kind, arg in a.items[s]:
            if kind == 'zero':
                b += bytes(arg)
            elif kind == 'bytes':
                b += arg
            elif kind == 'val':
                v = evalexpr(arg[0], syms)
                b += (v & ((1 << (8 * arg[1])) - 1)).to_bytes(arg[1], 'little')
        img[s] = bytes(b)
    # text: one instruction at a time
    ks = keystone.Ks(keystone.KS_ARCH_ARM64, keystone.KS_MODE_LITTLE_ENDIAN)

    def resolver(name, value):
        n = name.decode() if isinstance(name, bytes) else name
        if n in syms:
            value[0] = syms[n]
            return True
        if LENIENT:
            value[0] = 0x500000
            return True
        return False
    ks.sym_resolver = resolver
    code = bytearray()
    addr = TEXT
    for kind, arg in a.items['text']:
        if kind == 'zero':
            code += bytes(arg)
            addr += arg
        elif kind == 'ins':
            line = arg
            line = re.sub(r':lo12:([\w.$]+(?:[+-]\d+)?)', lambda m: '#%d' % (evalexpr(m.group(1), syms) & 0xfff), line)
            m = re.match(r'^adrp\s+(\w+),\s*([\w.$]+(?:[+-]\d+)?)$', line)
            if m:
                line = 'adrp %s, #%d' % (m.group(1), evalexpr(m.group(2), syms) & ~0xfff)
            # keystone reads a bare decimal as hex once a symbol resolver is set
            line = re.sub(r'#(-?)(\d+)\b(?![xX])', lambda m: '#%s%#x' % (m.group(1), int(m.group(2))) if m.group(2) != '0' else '#0', line)
            try:
                enc, _ = ks.asm(line, addr)
            except keystone.KsError as e:
                raise SystemExit('arm64run: cannot assemble "%s": %s' % (arg, e))
            if enc is None or len(enc) != 4:
                raise SystemExit('arm64run: cannot assemble "%s"' % arg)
            code += bytes(enc)
            addr += 4
        elif kind == 'bytes':		# data in the text section (a constant pool)
            code += arg
            addr += len(arg)
        elif kind == 'val':
            v = evalexpr(arg[0], syms)
            code += (v & ((1 << (8 * arg[1])) - 1)).to_bytes(arg[1], 'little')
            addr += arg[1]
    return code, img, base, syms


def srcline(a, pc):
    """The assembly line at address pc, with the two before it."""
    addr = TEXT
    lines = []
    for kind, arg in a.items['text']:
        if kind == 'zero':
            addr += arg
        elif kind == 'ins':
            lines.append((addr, arg))
            addr += 4
        elif kind == 'bytes':
            addr += len(arg)
        elif kind == 'val':
            addr += arg[1]
    for i, (ad, l) in enumerate(lines):
        if ad == pc:
            return ' | '.join(x[1] for x in lines[max(0, i - 2):i + 1])
    return '?'


def run(src, entry='test'):
    a = parse(src)
    code, img, base, syms = build(a)
    if entry not in syms:
        raise SystemExit('arm64run: no ' + entry)
    mu = unicorn.Uc(unicorn.UC_ARCH_ARM64, unicorn.UC_MODE_ARM)
    mu.mem_map(TEXT, (len(code) + 0xfff) & ~0xfff or 0x1000)
    mu.mem_write(TEXT, bytes(code))
    dsz = base['bss'] + a.size['bss'] - DATA + 0x1000
    mu.mem_map(DATA, (dsz + 0xfff) & ~0xfff)
    if img['data']:
        mu.mem_write(DATA, img['data'])
    mu.mem_map(STACK_TOP - STACK_SZ, STACK_SZ)
    mu.mem_map(STOP, 0x1000)
    mu.reg_write(A.UC_ARM64_REG_SP, STACK_TOP - 0x100)
    mu.reg_write(A.UC_ARM64_REG_X30, STOP)

    def hook(uc, address, size, ud):
        if address == STOP:
            uc.emu_stop()
    mu.hook_add(unicorn.UC_HOOK_CODE, hook, begin=STOP, end=STOP + 4)
    # enable FP/SIMD
    cpacr = mu.reg_read(A.UC_ARM64_REG_CPACR_EL1)
    mu.reg_write(A.UC_ARM64_REG_CPACR_EL1, cpacr | (3 << 20))
    try:
        mu.emu_start(syms[entry], STOP + 8, count=200000000)
    except unicorn.UcError as e:
        pc = mu.reg_read(A.UC_ARM64_REG_PC)
        raise SystemExit('arm64run: emulation fault %s at pc=%#x (%s)' % (e, pc, srcline(a, pc)))
    x0 = mu.reg_read(A.UC_ARM64_REG_X0) & 0xffffffff
    return x0 - (1 << 32) if x0 & 0x80000000 else x0


if __name__ == '__main__':
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    print(run(open(sys.argv[1]).read(), sys.argv[2] if len(sys.argv) > 2 else 'test'))
