#!/usr/bin/env python3
"""
vax32v -- run C programs compiled by the VAX pcc under 32V on SIMH.

    vax32v.py [-f "ccom flags"] --ccom CCOM test1.c test2.c ...

For each file: preprocess, compile with the VAX ccom (a target
whose OS is bsd writes the assembler dialect of 32V and 4.xBSD), copy the results into /tmp of a copy of a 32V RP06 disk
image (mounted with filsys, the FUSE driver for old Unix filesystems), boot it
on open-simh's vax780, build each test with the guest's cc (with a main()
that prints test()'s value) and run it.  Prints "name value" per test.

Environment:
    VAX780      open-simh vax780 (the Ubuntu 'simh' 3.8-1 binary crashes 32V)
    VAX32V_DISK a pristine installed 32V disk (see prebsd/vax/install32v.py)
    VAX32V_PORT console port (default 10024)
                (the saved disk's free list must be rebuilt once, or files put in
                by FUSE share blocks with what the guest writes later and read back
                as garbage:  fsck.filsys -v vax32 -s -y -f disk)
    FILSYS      mount.filsys (default: ~/claude/facetlang/pdp11/filsys/mount.filsys)
"""
import os, re, shutil, socket, subprocess, sys, tempfile, time

IAC, WILL, WONT, DO, DONT, SB, SE = 0xFF, 0xFB, 0xFC, 0xFD, 0xFE, 0xFA, 0xF0

BOOT_INI = """set tto 7b
set rq dis
set lpt dis
set rl dis
set hk dis
set rqb dis
set rqc dis
set rqd dis
set ry dis
set ts dis
set tq dis
set dz lines=8
set rp0 rp06
at rp0 {disk}
set console telnet={port}
D 30000 00009FDE
D 30004 D0512001
D 30008 D004A101
D 3000C 0400C113
D 30010 10008F32
D 30014 D40424C1
D 30018 8FD00CA1
D 3001C 80000000
D 30020 320800C1
D 30024 A1FE008F
D 30028 28C1D410
D 3002C 14C1D404
D 30030 C139D004
D 30034 c1d00400
D 30038 e1500404
D 3003C 00f75007
go 30000
go 2
"""

# ---------------------------------------------------------------- the disk

def put_files(disk, files):
    """Write files into /tmp of the 32V root filesystem inside the disk image."""
    filsys = os.environ.get('FILSYS', os.path.expanduser('~/claude/facetlang/pdp11/filsys/mount.filsys'))
    mnt = tempfile.mkdtemp(prefix='vax32v.mnt.')
    r = subprocess.run([filsys, '-v', 'vax32', disk, mnt], capture_output=True, text=True)
    if r.returncode:
        sys.exit('mount.filsys failed: ' + r.stderr)
    try:
        for name, data in files.items():
            with open(os.path.join(mnt, 'tmp', name), 'wb') as f:
                f.write(data)
    finally:
        for _ in range(10):
            if subprocess.run(['fusermount', '-u', mnt], capture_output=True).returncode == 0:
                break
            time.sleep(0.5)
        os.rmdir(mnt)


# ---------------------------------------------------------------- console

def negotiate(sock):
    try:
        data = sock.recv(65536)
    except (socket.timeout, OSError):
        return b''
    out = b''
    rep = b''
    i = 0
    n = len(data)
    while i < n:
        b = data[i]
        if b == IAC:
            if i + 1 >= n:
                break
            c = data[i + 1]
            if c in (WILL, WONT, DO, DONT):
                if i + 2 >= n:
                    break
                o = data[i + 2]
                rep += bytes([IAC, DONT if c == WILL else WONT if c == DO else DONT if c == WONT else WONT, o])
                i += 3
            elif c == IAC:
                out += bytes([IAC])
                i += 2
            elif c == SB:
                j = data.find(bytes([IAC, SE]), i)
                i = n if j < 0 else j + 2
            else:
                i += 2
        else:
            out += bytes([b])
            i += 1
    if rep:
        try:
            sock.sendall(rep)
        except OSError:
            pass
    return out


class Vax:
    def __init__(self, ini, port):
        self.proc = subprocess.Popen([os.environ.get('VAX780', 'vax780'), ini],
                                     stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                                     stderr=subprocess.STDOUT)
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

    def wait_for(self, s, timeout=120, start=0):
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


# ---------------------------------------------------------------- main

def compile_one(ccom, flags, src, work):
    name = os.path.splitext(os.path.basename(src))[0]
    i = os.path.join(work, name + '.i')
    s = os.path.join(work, name + '.s')
    r = subprocess.run(['gcc', '-E', '-P', '-undef', '-D__vax__', src, '-o', i], capture_output=True, text=True)
    if r.returncode:
        return None, 'cpp: ' + r.stderr[-200:]
    r = subprocess.run([ccom] + flags.split() + [i, s], capture_output=True, text=True, errors='replace')
    if r.returncode or not os.path.exists(s):
        return None, 'ccom: ' + (r.stderr or r.stdout)[-200:].strip()
    return open(s, errors='replace').read(), None


def main():
    args = sys.argv[1:]
    flags = ''
    ccom = os.environ.get('VAXCCOM')
    keep = None
    while args and args[0].startswith('-'):
        if args[0] == '-f':
            flags = args[1]; args = args[2:]
        elif args[0] == '--ccom':
            ccom = args[1]; args = args[2:]
        elif args[0] == '--keep':
            keep = args[1]; args = args[2:]
        else:
            sys.exit('usage: vax32v.py [-f flags] --ccom CCOM files.c')
    if not ccom or not args:
        sys.exit('usage: vax32v.py [-f flags] --ccom CCOM files.c')
    disk0 = os.environ['VAX32V_DISK']
    port = int(os.environ.get('VAX32V_PORT', '10024'))
    work = tempfile.mkdtemp(prefix='vax32v.')
    files = {}
    names = []
    ids = []
    failed = {}
    for n, src in enumerate(args):
        name = os.path.splitext(os.path.basename(src))[0]
        asm, err = compile_one(ccom, flags, src, work)
        if err:
            failed[name] = err
            continue
        # V7 file names are 14 characters at most: t<n>.s, m<n>.c
        files['t%d.s' % n] = asm.encode()
        files['m%d.c' % n] = ('main() { alarm(' + os.environ.get('VAX32V_TESTSECS', '30') +
                              '); printf("RESULT %s %d\\n", "' + name + '", test()); exit(0); }\n').encode()
        names.append(name)
        ids.append(n)
    sh = 'cd /tmp\nfor t in ' + ' '.join('%d' % n for n in ids) + '\ndo\n' \
         '\t# the first as after a boot sometimes fails with a bogus syntax error: retry once\n' \
         '\tif cc -o t$t.out m$t.c t$t.s > e$t 2>&1 || cc -o t$t.out m$t.c t$t.s > e$t 2>&1\n\tthen\n\t\t./t$t.out || echo "RESULT n$t CRASH"\n' \
         '\telse\n\t\techo "RESULT n$t BUILDFAIL"; head -3 e$t\n\tfi\ndone\necho ALLDONE\n'
    files['run.sh'] = sh.encode()
    if keep:
        os.makedirs(keep, exist_ok=True)
        for k, v in files.items():
            open(os.path.join(keep, k), 'wb').write(v)
    disk = os.path.join(work, 'disk')
    shutil.copyfile(disk0, disk)
    if names:
        put_files(disk, files)
    ini = os.path.join(work, 'boot.ini')
    open(ini, 'w').write(BOOT_INI.format(disk=disk, port=port))
    results = {}
    if names:
        v = Vax(ini, port)
        try:
            v.poll(2.0)
            for _ in range(60):
                if 'file' in v.text():
                    break
                v.poll(0.5)
            v.send('unix')
            if not v.wait_for('#', 240):
                sys.exit('32V did not boot:\n' + v.text()[-300:])
            def cmd(c, w=1.0):
                v.send(c); v.poll(w)
            cmd('/etc/mount /dev/rp0h /usr', 3.0)
            cmd('cd /tmp')
            start = len(v.text())
            cmd('sh run.sh', 1.0)
            if not v.wait_for('ALLDONE', 120 + 60 * len(names), start):
                sys.stderr.write('timeout waiting for the tests\n')
            for m in re.finditer(r'RESULT (\S+) (-?\w+)', v.text()):
                k = m.group(1)
                if re.fullmatch(r'n\d+', k) and int(k[1:]) in ids:
                    k = names[ids.index(int(k[1:]))]
                results.setdefault(k, m.group(2))
        finally:
            v.close()
    for n in sorted(set(names) | set(failed)):
        if n in failed:
            print(n, 'COMPILEFAIL', failed[n].replace('\n', ' ')[:100])
        else:
            print(n, results.get(n, 'NORESULT'))
    shutil.rmtree(work, ignore_errors=True)


if __name__ == '__main__':
    main()
