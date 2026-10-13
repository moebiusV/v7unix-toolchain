#!/usr/bin/env python3
"""
v7progs -- run real V7 programs compiled by pcc inside a real V7 on SIMH.

    v7progs.py [-f "ccom flags"] [prog ...]

For each program (default: a list of the V7 commands), pcc -O -S compiles
/usr/src/cmd/<prog>.c of the V7 disk image on the host; the assembly and some
input files are put into /usr/tmp of a copy of the image (mounted with filsys),
V7 is booted on SIMH's pdp11, and a shell script builds the program twice
-- with V7's own cc (the reference) and from our assembly with cc -- runs both
on the same arguments and compares the output with cmp.

Prints one line per test: OK, DIFF, REFFAIL (V7's cc failed), PCCFAIL (pcc
failed on the host), ASFAIL (V7 could not assemble/link ours), and a total.

Environment:
    PCC        the pcc driver     (default ~/claude/pcc/install/bin/pcc)
    V7IMG      a V7 RP06 image    (default ~/claude/facetlang/pdp11/prebsd/images/v7-bostic.disk)
    FILSYS     mount.filsys       (default ~/claude/facetlang/pdp11/filsys/mount.filsys)
    SIMH       the pdp11 binary   (default pdp11)
    V7PORT     console port       (default 10027)
"""
import os, re, shutil, socket, subprocess, sys, tempfile, time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from vax32v import negotiate

HOME = os.path.expanduser('~')
PCC = os.environ.get('PCC', HOME + '/claude/pcc/install/bin/pcc')
IMG = os.environ.get('V7IMG', HOME + '/claude/facetlang/pdp11/prebsd/images/v7-bostic.disk')
FILSYS = os.environ.get('FILSYS', HOME + '/claude/facetlang/pdp11/filsys/mount.filsys')
SIMH = os.environ.get('SIMH', 'pdp11')
PORT = int(os.environ.get('V7PORT', '10027'))
USR_OFFSET = 9416704

# ---------------------------------------------------------------- test data

IN1 = ''.join('line %d: the quick brown fox %s jumps over %d lazy dogs\n' % (i, 'foo' if i % 3 == 0 else 'bar', i * 7 % 13)
              for i in range(1, 60)) + 'last line without much\n'
IN2 = ''.join('line %d: the quick brown fox %s jumps over %d lazy dogs\n' % (i, 'foo' if i % 4 == 0 else 'bar', i * 5 % 11)
              for i in range(1, 60))
SORTED1 = ''.join('%s\n' % w for w in sorted(['apple', 'banana', 'cherry', 'date', 'elder', 'fig', 'grape', 'honey', 'iris']))
SORTED2 = ''.join('%s\n' % w for w in sorted(['banana', 'cherry', 'fig', 'kiwi', 'lemon', 'mango']))
DUPS = ''.join('%s\n' % w for w in ['a', 'a', 'b', 'c', 'c', 'c', 'd', 'e', 'e'])
NUMS = ''.join('%d\n' % n for n in [10, 9, 100, 1, 55, 3, 3, 2000, 7])
J1 = ''.join('%d one%d\n' % (i, i) for i in range(1, 12, 2))
J2 = ''.join('%d two%d\n' % (i, i) for i in range(1, 12, 3))
CSRC = 'main(){int i,j;for(i=0;i<10;i++){if(i%2)j=i*2;else j=i;printf("%d\\n",j);}}\n'
TSIN = 'a b\nb c\nc d\na c\nd e\n'

FILES = {'in1': IN1, 'in2': IN2, 's1': SORTED1, 's2': SORTED2, 'dups': DUPS, 'nums': NUMS,
         'j1': J1, 'j2': J2, 'src.c': CSRC, 'tsin': TSIN}

# program -> list of command lines; $P is the program under test
TESTS = {
    'cat': ['$P in1', '$P in1 in2', '$P -u s1'],
    'cmp': ['$P in1 in2', '$P in1 in1', '$P -l in1 in2'],
    'comm': ['$P s1 s2', '$P -12 s1 s2', '$P -3 s1 s2'],
    'echo': ['$P a b c', '$P -n x y', '$P'],
    'basename': ['$P /a/b/c.c .c', '$P x'],
    'wc': ['$P in1', '$P -l in1', '$P -w -c in1 in2'],
    'tr': ['$P a-z A-Z <in1', '$P -d 0-9 <in1', '$P -s " " <in1'],
    'uniq': ['$P dups', '$P -c dups', '$P -d dups', '$P -u dups'],
    'od': ['$P in1', '$P -c in1', '$P -x s1'],
    'sort': ['$P in1', '$P -r in1', '$P -n nums', '$P -u dups'],
    'tail': ['$P in1', '$P -5 in1', '$P +50 in1'],
    'tee': ['$P /usr/tmp/tee.out <in1 >/usr/tmp/tee.std; cat /usr/tmp/tee.out /usr/tmp/tee.std'],
    'sum': ['$P in1', '$P in1 in2'],
    'cal': ['$P 1977', '$P 5 1977', '$P 2 1980'],
    'col': ['$P <in1'],
    'grep': ['$P foo in1', '$P -n the in1', '$P -v e in1', '$P -c a in1 in2'],
    'join': ['$P j1 j2', '$P -a1 j1 j2'],
    'look': ['$P ba s1', '$P -f BA s1'],
    'rev': ['$P <in1'],
    'cb': ['$P <src.c'],
    'dd': ['$P if=in1 conv=ucase', '$P if=in1 bs=100 count=3'],
    'file': ['$P in1 src.c /bin/ls'],
    'tsort': ['$P tsin'],
    'test': ['$P -f in1 && echo yes || echo no', '$P 5 -lt 7 && echo lt', '$P -d /bin && echo dir'],
    'pwd': ['$P'],
    'size': ['$P /bin/ls /bin/cat'],
    'nm': ['$P /bin/ls'],
    'ar': ['$P t /lib/libc.a'],
    'ls': ['$P -l /bin', '$P -i /etc'],
    'find': ['$P /etc -name passwd -print', '$P /bin -type f -print'],
    'cp': ['$P in1 /usr/tmp/cp.out; cat /usr/tmp/cp.out'],
    'pr': ['$P in1', '$P -3 in2'],
    'fgrep': ['$P foo in1', '$P -x -f s1 s2'],
    'ed': ['$P in1 <<!\n1,3p\ns/the/THE/\n$p\nw /usr/tmp/ed.out\nq\n!\ncat /usr/tmp/ed.out'],
    'diff': ['$P in1 in2', '$P s1 s2'],
}
DEFAULT = ['cat', 'cmp', 'comm', 'echo', 'basename', 'wc', 'tr', 'uniq', 'od', 'sort', 'tail',
           'tee', 'sum', 'cal', 'col', 'grep', 'join', 'look', 'rev', 'cb', 'dd', 'file',
           'tsort', 'test', 'pwd', 'size', 'nm', 'ar', 'ls', 'find', 'cp', 'pr', 'fgrep', 'ed']


class Pdp:
    def __init__(self, ini, port):
        self.proc = subprocess.Popen([SIMH, ini], stdin=subprocess.DEVNULL,
                                     stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
        self.sock = None
        for _ in range(120):
            try:
                self.sock = socket.create_connection(('127.0.0.1', port), timeout=2)
                break
            except OSError:
                time.sleep(0.25)
        if not self.sock:
            self.proc.kill()
            raise RuntimeError('cannot connect to the simulator console')
        self.sock.settimeout(0.2)
        self.buf = bytearray()

    def poll(self, d=0.5):
        end = time.time() + d
        while time.time() < end:
            self.buf.extend(negotiate(self.sock))
            time.sleep(0.03)

    def text(self):
        return self.buf.decode('latin1', 'replace')

    def wait_for(self, s, timeout, start=0):
        end = time.time() + timeout
        while time.time() < end and s not in self.text()[start:]:
            self.poll(0.4)
        return s in self.text()[start:]

    def send(self, s):
        self.sock.sendall((s + '\r').encode())

    def close(self):
        try:
            self.sock.close()
        except OSError:
            pass
        self.proc.terminate()
        try:
            self.proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            self.proc.kill()


def mount(img, mnt, offset=None):
    cmd = [FILSYS, '-v', '7']
    if offset:
        cmd += ['-o', 'offset=%d' % offset]
    r = subprocess.run(cmd + [img, mnt], capture_output=True, text=True)
    if r.returncode:
        sys.exit('mount.filsys failed: ' + r.stderr)


def umount(mnt):
    for _ in range(20):
        if subprocess.run(['fusermount', '-u', mnt], capture_output=True).returncode == 0:
            return
        time.sleep(0.5)


def main():
    args = sys.argv[1:]
    flags = ''
    if args[:1] == ['-f']:
        flags = args[1]
        args = args[2:]
    progs = args or DEFAULT
    work = tempfile.mkdtemp(prefix='v7progs.')
    disk = os.path.join(work, 'v7.disk')
    shutil.copyfile(IMG, disk)
    mnt = os.path.join(work, 'mnt')
    os.mkdir(mnt)

    mount(disk, mnt, USR_OFFSET)
    src = os.path.join(mnt, 'src', 'cmd')
    inc = os.path.join(mnt, 'include')
    asm = {}
    results = []
    for p in progs:
        c = os.path.join(src, p + '.c')
        if p not in TESTS or not os.path.exists(c):
            results.append((p, 0, 'NOSOURCE'))
            continue
        out = os.path.join(work, p + '.s')
        r = subprocess.run([PCC, '-O', '-S'] + flags.split() + ['-I' + inc, '-o', out, c],
                           capture_output=True, text=True)
        if r.returncode or not os.path.exists(out):
            results.append((p, 0, 'PCCFAIL ' + (r.stderr.strip().split('\n') or [''])[0][:80]))
            continue
        asm[p] = open(out, 'rb').read()

    tmp = os.path.join(mnt, 'tmp')
    for name, data in FILES.items():
        open(os.path.join(tmp, name), 'w').write(data)
    sh = ['cd /usr/tmp']
    order = []
    for p in progs:
        if p not in asm:
            continue
        open(os.path.join(tmp, p + '.s'), 'wb').write(asm[p])
        order.append(p)
        sh.append('echo "== %s"' % p)
        sh.append('cc -O -o ref_%s /usr/src/cmd/%s.c > /dev/null 2>&1 || echo "REFFAIL %s"' % (p, p, p))
        sh.append('cc -o pcc_%s %s.s %s || echo "ASFAIL %s"' % (p, p, '2>&1 | tail -4' if os.environ.get('V7VERBOSE') else '> /dev/null 2>&1', p))
        for k, t in enumerate(TESTS[p], 1):
            ref = t.replace('$P', './ref_' + p)
            new = t.replace('$P', './pcc_' + p)
            sh.append('( %s ) > r.out 2>&1' % ref.replace('\n', '\n'))
            sh.append('( %s ) > p.out 2>&1' % new.replace('\n', '\n'))
            more = '; diff r.out p.out | head -6' if os.environ.get('V7VERBOSE') else ''
            sh.append('if cmp -s r.out p.out; then echo "OK %s %d"; else echo "DIFF %s %d"%s; fi' % (p, k, p, k, more))
        sh.append('rm -f ref_%s pcc_%s %s.s' % (p, p, p))
    sh.append('echo ALLDONE')
    open(os.path.join(tmp, 'run.sh'), 'w').write('\n'.join(sh) + '\n')
    umount(mnt)

    ini = os.path.join(work, 'v7.ini')
    open(ini, 'w').write('set cpu 11/70\nset cpu 2M\nset cpu idle\nset rp0 rp06\natt rp0 %s\n'
                         'set console telnet=%d\nboot rp0\n' % (disk, PORT))
    v = Pdp(ini, PORT)
    text = ''
    try:
        v.poll(2.0)
        for send, expect in (('boot', ':'), ('hp(0,0)hptmunix', 'mem =')):
            v.send(send)
            if not v.wait_for(expect, 120):
                sys.exit('V7 did not boot: ' + v.text()[-200:])
        v.poll(1.0)
        v.send('stty -lcase')
        v.poll(1.0)
        v.send('/etc/mount /dev/rp3 /usr')
        v.poll(3.0)
        start = len(v.text())
        v.send('sh /usr/tmp/run.sh')
        if not v.wait_for('ALLDONE', int(os.environ.get('V7WAIT', 120 + 90 * len(order))), start):
            sys.stderr.write('timeout waiting for the tests\n')
        text = v.text()[start:]
    finally:
        if os.environ.get('V7LOG'):
            open(os.environ['V7LOG'], 'w').write(v.text())
        v.close()

    ok = diff = 0
    for line in text.replace('\r', '').split('\n'):
        m = re.match(r'(OK|DIFF) (\S+) (\d+)', line)
        if m:
            results.append((m.group(2), int(m.group(3)), m.group(1)))
            ok += m.group(1) == 'OK'
            diff += m.group(1) == 'DIFF'
        elif line.startswith(('REFFAIL', 'ASFAIL')):
            results.append((line.split()[1], 0, line.split()[0]))
    for p, k, r in results:
        if r != 'OK':
            print('%-10s %s %s' % (p, k or '', r))
    bad = sum(1 for _, _, r in results if r != 'OK')
    print('v7progs: %d tests OK, %d DIFF, %d other problems (%d programs)' % (ok, diff, bad - diff, len(progs)))
    shutil.rmtree(work, ignore_errors=True)
    return 0 if bad == 0 else 1


if __name__ == '__main__':
    sys.exit(main())
