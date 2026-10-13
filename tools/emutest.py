#!/usr/bin/env python3
"""
emutest -- compile a C file with a pcc ccom for another architecture, link it
with zig's lld and run `int test()` in the Unicorn CPU emulator.

usage: emutest.py ARCH CCOM [ccom flags ...] file.c
   ARCH is aarch64, arm, mips, mips64, ppc or m68k.  Prints test()'s result.

needs (in a venv, see EMU_PY in emutest-all.sh):  pip install unicorn ziglang pyelftools
"""
import os, subprocess, sys, tempfile

ARCH = {
    # name: (zig target, unicorn arch, unicorn mode, result register, sp register, start stub)
    'aarch64': ('aarch64-linux', 'UC_ARCH_ARM64', 'UC_MODE_ARM', 'UC_ARM64_REG_X0', 'UC_ARM64_REG_SP',
                '.text\n.globl _start\n_start:\n\tbl test\n.globl _done\n_done:\n\tb _done\n'),
    'arm': ('arm-linux-gnueabi', 'UC_ARCH_ARM', 'UC_MODE_ARM', 'UC_ARM_REG_R0', 'UC_ARM_REG_SP',
            '.text\n.globl _start\n_start:\n\tbl test\n.globl _done\n_done:\n\tb _done\n'),
    'mips': ('mips-linux-gnu', 'UC_ARCH_MIPS', 'UC_MODE_MIPS32|UC_MODE_BIG_ENDIAN', 'UC_MIPS_REG_V0', 'UC_MIPS_REG_SP',
             '.text\n.set noreorder\n.globl _start\n_start:\n\tjal test\n\tnop\n.globl _done\n_done:\n\tb _done\n\tnop\n'),
}

def main():
    arch = sys.argv[1]
    ccom = sys.argv[2]
    flags = sys.argv[3:-1]
    src = sys.argv[-1]
    ztarget, uarch, umode, rreg, spreg, stub = ARCH[arch]
    py = sys.executable
    w = tempfile.mkdtemp(prefix='emutest.')
    def run(cmd, **kw):
        r = subprocess.run(cmd, capture_output=True, text=True, **kw)
        if r.returncode:
            sys.stderr.write(' '.join(cmd) + '\n' + r.stdout + r.stderr)
            sys.exit(1)
    run(['gcc', '-E', '-P', '-undef', '-D__pcc__', src, '-o', w + '/t.i'])
    run([ccom] + flags + [w + '/t.i', w + '/t.s'])
    open(w + '/start.s', 'w').write(stub)
    run([py, '-m', 'ziglang', 'cc', '-target', ztarget, '-nostdlib', '-static', '-Wl,-e,_start',
         '-Wl,--no-gc-sections', '-o', w + '/t.elf', w + '/start.s', w + '/t.s'])
    from elftools.elf.elffile import ELFFile
    import unicorn
    from unicorn import Uc
    elf = ELFFile(open(w + '/t.elf', 'rb'))
    mode = 0
    for m in umode.split('|'):
        mode |= getattr(unicorn, m)
    uc = Uc(getattr(unicorn, uarch), mode)
    for seg in elf.iter_segments():
        if seg['p_type'] != 'PT_LOAD':
            continue
        lo = seg['p_vaddr'] & ~0xfff
        hi = (seg['p_vaddr'] + seg['p_memsz'] + 0xfff) & ~0xfff
        try:
            uc.mem_map(lo, hi - lo)
        except unicorn.UcError:
            pass
        uc.mem_write(seg['p_vaddr'], seg.data())
    stack = 0x7f000000
    uc.mem_map(stack, 0x100000)
    uc.reg_write(getattr(unicorn.arm64_const if arch == 'aarch64' else unicorn.arm_const if arch == 'arm'
                         else unicorn.mips_const, spreg), stack + 0xffff0)
    symtab = elf.get_section_by_name('.symtab')
    done = symtab.get_symbol_by_name('_done')[0]['st_value']
    start = elf.header['e_entry']
    try:
        uc.emu_start(start, done, count=50000000)
    except unicorn.UcError as e:
        sys.stderr.write('emulation error: %s\n' % e)
        sys.exit(1)
    cst = unicorn.arm64_const if arch == 'aarch64' else unicorn.arm_const if arch == 'arm' else unicorn.mips_const
    v = uc.reg_read(getattr(cst, rreg))
    if arch == 'aarch64':
        v &= 0xffffffff
    if v & 0x80000000:
        v -= 1 << 32
    print(v)

main()
