#!/usr/bin/env python3
"""
consts.py -- binary128 constants: the words the compiler writes for long double literals,
against the host's __float128 (gcc, libquadmath).  Needs a ccom built with -DA64QUAD.

    consts.py CCOM
"""
import re, subprocess, sys, tempfile, os

LITS = '''1.0L -2.5L 0.1L 3.14159265358979323846264338327950288L 0.0L -0.0L 0x1.8p+0L 1e100L -1.5e-100L
1e4000L 1e-4000L 1.18973149535723176502126385303097021e+4932L 3.36210314311209350626267781732175260e-4932L
123456789012345678901234567890.5L 0.3333333333333333333333333333333333L 1e-4900L 1e-4940L'''.split()


def main():
    ccom = sys.argv[1]
    d = tempfile.mkdtemp()
    open(d + '/t.c', 'w').write(''.join('long double k%d = %s;\n' % (i, l) for i, l in enumerate(LITS)))
    h = '#include <stdio.h>\n#include <string.h>\n#include <stdint.h>\n'
    h += ''.join('__float128 k%d = %sQ;\n' % (i, l[:-1]) for i, l in enumerate(LITS))
    h += 'int main(void){uint32_t w[4];\n'
    h += ''.join('memcpy(w,&k%d,16);printf("%%u %%u %%u %%u\\n",w[0],w[1],w[2],w[3]);\n' % i for i in range(len(LITS)))
    open(d + '/h.c', 'w').write(h + 'return 0;}\n')
    subprocess.check_call(['gcc', '-o', d + '/h', d + '/h.c', '-lquadmath'])
    host = [tuple(map(int, l.split())) for l in subprocess.check_output([d + '/h'], text=True).splitlines()]
    subprocess.check_call('gcc -E -P %s/t.c > %s/t.i' % (d, d), shell=True)
    subprocess.check_call([ccom, d + '/t.i', d + '/t.s'])
    words = [int(x) for x in re.findall(r'\.long\s+(\d+)', open(d + '/t.s').read())]
    bad = 0
    for i, l in enumerate(LITS):
        w = tuple(words[4 * i:4 * i + 4])
        if w != host[i]:
            bad += 1
            print('BAD %s: pcc %s host %s' % (l, ['%08x' % x for x in w[::-1]], ['%08x' % x for x in host[i][::-1]]))
    print('consts: %d of %d match' % (len(LITS) - bad, len(LITS)))
    sys.exit(1 if bad else 0)


main()
