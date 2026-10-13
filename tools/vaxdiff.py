#!/usr/bin/env python3
"""
vaxdiff -- code size of the VAX pcc against 32V's own cc, measured under 32V.

    vaxdiff.py --ccom CCOM [-f "ccom flags"] [-I inc] file.c[.gz] ...

For every source: the guest compiles it with its own cc (and cc -O, which runs
the VAX c2), and assembles the assembly the VAX ccom wrote (alone, and after
the guest's c2).  `size` gives text+data for each.  Prints one line per file
and totals.

Environment as for vax32v.py (VAX780, VAX32V_DISK, VAX32V_PORT, FILSYS).
"""
import gzip, os, re, shutil, subprocess, sys, tempfile, time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import vax32v


def main():
    args = sys.argv[1:]
    ccom = os.environ.get('VAXCCOM')
    flags = ''
    inc = os.environ.get('VAX32V_INC')
    keep = None
    while args and args[0].startswith('-') and args[0] != '-':
        if args[0] == '--ccom':
            ccom = args[1]; args = args[2:]
        elif args[0] == '-f':
            flags = args[1]; args = args[2:]
        elif args[0] == '-I':
            inc = args[1]; args = args[2:]
        elif args[0] == '--keep':
            keep = args[1]; args = args[2:]
        else:
            sys.exit(__doc__)
    if not ccom or not args:
        sys.exit(__doc__)
    work = tempfile.mkdtemp(prefix='vaxdiff.')
    files = {}
    names = []
    ids = []
    skipped = []
    for n, src in enumerate(args):
        base = os.path.basename(src)
        name = re.sub(r'\.gz$', '', base)
        name = re.sub(r'\.c$', '', name)
        if src.endswith('.gz'):
            data = gzip.open(src).read()
        else:
            data = open(src, 'rb').read()
        local = os.path.join(work, name + '.c')
        open(local, 'wb').write(data)
        pp = [] if not inc else ['-nostdinc', '-I', inc]
        r = subprocess.run(['gcc', '-E', '-P', '-undef', '-Dvax', '-Dunix', '-D__vax__'] + pp +
                           ['-I', os.path.dirname(os.path.abspath(src)), local, '-o', local + '.i'],
                           capture_output=True, text=True)
        if r.returncode:
            skipped.append((name, 'cpp'))
            continue
        try:
            r = subprocess.run([ccom] + flags.split() + [local + '.i', local + '.s'],
                               capture_output=True, text=True, errors='replace', timeout=60)
        except subprocess.TimeoutExpired:
            skipped.append((name, 'ccom timed out'))
            continue
        if r.returncode or not os.path.exists(local + '.s'):
            skipped.append((name, 'ccom'))
            continue
        files['f%d.c' % n] = data
        files['p%d.s' % n] = open(local + '.s', 'rb').read()
        names.append(name)
        ids.append(n)
    sh = 'cd /tmp\nfor t in ' + ' '.join(str(i) for i in ids) + '\ndo\n' \
         '\tcc -c f$t.c > /dev/null 2>&1 && echo "SZ $t D `size f$t.o`"\n' \
         '\tcc -O -c f$t.c > /dev/null 2>&1 && echo "SZ $t O `size f$t.o`"\n' \
         '\tas -o p$t.o p$t.s > /dev/null 2>&1 && echo "SZ $t P `size p$t.o`"\n' \
         '\t/lib/c2 p$t.s q$t.s > /dev/null 2>&1 && as -o q$t.o q$t.s > /dev/null 2>&1 && echo "SZ $t C `size q$t.o`"\n' \
         '\tcc -O -S f$t.c > /dev/null 2>&1 && mv f$t.s d$t.s\n' \
         '\t/lib/c2 p$t.s q$t.s > /dev/null 2>&1\n' \
         '\trm -f f$t.o p$t.o q$t.o\n' \
         'done\necho ALLDONE\n'
    files['run.sh'] = sh.encode()
    disk = os.path.join(work, 'disk')
    shutil.copyfile(os.environ['VAX32V_DISK'], disk)
    vax32v.put_files(disk, files)
    port = int(os.environ.get('VAX32V_PORT', '10024'))
    ini = os.path.join(work, 'boot.ini')
    open(ini, 'w').write(vax32v.BOOT_INI.format(disk=disk, port=port))
    v = vax32v.Vax(ini, port)
    sizes = {}
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
        v.wait_for('ALLDONE', 60 + 40 * len(ids), start)
        for m in re.finditer(r'SZ (\d+) (\w) (\d+)\+(\d+)\+(\d+)', v.text()[start:]):
            i, k, t, d, b = m.groups()
            sizes.setdefault(int(i), {})[k] = int(t) + int(d)
        if keep:
            for _ in range(3):
                v.send('sync'); v.poll(2.0)
    finally:
        v.close()
    if keep:
        os.makedirs(keep, exist_ok=True)
        mnt = tempfile.mkdtemp(prefix='vaxdiff.mnt.')
        filsys = os.environ.get('FILSYS', os.path.expanduser('~/claude/facetlang/pdp11/filsys/mount.filsys'))
        subprocess.run([filsys, '-v', 'vax32', '-r', disk, mnt], capture_output=True)
        for i, name in zip(ids, names):
            for pre, tag in (('d', 'dmr'), ('p', 'pcc'), ('q', 'pccc2')):
                src = os.path.join(mnt, 'tmp', '%s%d.s' % (pre, i))
                if os.path.exists(src):
                    shutil.copyfile(src, os.path.join(keep, '%s.%s.s' % (name, tag)))
        for _ in range(10):
            if subprocess.run(['fusermount', '-u', mnt], capture_output=True).returncode == 0:
                break
            time.sleep(0.5)
    tot = {'D': 0, 'O': 0, 'P': 0, 'C': 0}
    cnt = 0
    print('%-14s %8s %8s %8s %8s' % ('file', '32V cc', '32V -O', 'pcc', 'pcc+c2'))
    for i, name in zip(ids, names):
        s = sizes.get(i, {})
        if not all(k in s for k in 'DOPC'):
            print('%-14s  (incomplete: %s)' % (name, ''.join(sorted(s))))
            continue
        cnt += 1
        for k in tot:
            tot[k] += s[k]
        print('%-14s %8d %8d %8d %8d' % (name, s['D'], s['O'], s['P'], s['C']))
    print('%-14s %8d %8d %8d %8d   (%d files; %d skipped)' % ('TOTAL', tot['D'], tot['O'], tot['P'], tot['C'], cnt, len(skipped)))
    for n, why in skipped:
        print('skipped', n, why)
    shutil.rmtree(work, ignore_errors=True)


if __name__ == '__main__':
    main()
