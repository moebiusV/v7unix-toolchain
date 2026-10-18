#!/usr/bin/env python3
"""
vaxasm -- assemble VAX .s files with 32V's own as and print text+data and the
instruction listing sizes.   vaxasm.py file.s ...   (environment as vaxdiff.py)
"""
import os, re, shutil, sys, tempfile
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import vax32v


def main():
    srcs = sys.argv[1:]
    if not srcs:
        sys.exit(__doc__)
    work = tempfile.mkdtemp(prefix='vaxasm.')
    files = {}
    for i, s in enumerate(srcs):
        files['a%d.s' % i] = open(s, 'rb').read()
    files['run.sh'] = ('cd /tmp\nfor t in %s\ndo\n\tas -o a$t.o a$t.s > /dev/null 2>&1 && '
                       'echo "SZ $t `size a$t.o`"\ndone\necho ALLDONE\n'
                       % ' '.join(str(i) for i in range(len(srcs)))).encode()
    disk = os.path.join(work, 'disk')
    shutil.copyfile(os.environ['VAX32V_DISK'], disk)
    vax32v.put_files(disk, files)
    port = int(os.environ.get('VAX32V_PORT', '10024'))
    ini = os.path.join(work, 'boot.ini')
    open(ini, 'w').write(vax32v.BOOT_INI.format(disk=disk, port=port))
    v = vax32v.Vax(ini, port)
    try:
        v.poll(2.0)
        for _ in range(60):
            if 'file' in v.text():
                break
            v.poll(0.5)
        v.send('unix')
        if not v.wait_for('#', 240):
            sys.exit('32V did not boot')
        v.send('/etc/mount /dev/rp0h /usr'); v.poll(3.0)
        v.send('cd /tmp'); v.poll(1.0)
        start = len(v.text())
        v.send('sh run.sh')
        v.wait_for('ALLDONE', 60 + 20 * len(srcs), start)
        out = v.text()[start:]
    finally:
        v.close()
    for m in re.finditer(r'SZ (\d+) (\d+)\+(\d+)\+(\d+)', out):
        i, t, d, b = m.groups()
        print('%-40s text %s data %s bss %s' % (srcs[int(i)], t, d, b))
    shutil.rmtree(work, ignore_errors=True)


if __name__ == '__main__':
    main()
