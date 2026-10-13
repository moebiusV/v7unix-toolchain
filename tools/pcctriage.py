#!/usr/bin/env python3
# pcctriage: see tools/README-pccfuzz.txt
import subprocess,sys,re,os
T=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
P=os.environ.get('PCC','pcc')
INC=['-I'+T+'/orig/usr/include','-I'+T+'/modern/usr/include','-I'+T+'/pcc99/usr/include','-Dunix']
env=dict(os.environ,PCC_NORETRY='1')
SIG=re.compile(r'internal compiler error|major internal|cannot color|compiler error: (?!too many)|Cannot generate')
def run(path,extra,opt='-O'):
    return subprocess.run([P,opt,'-S']+extra+['-o','/dev/null',path],capture_output=True,text=True,encoding='utf-8',errors='replace',env=env)
for arg in sys.argv[1:]:
    f,_,odir=arg.partition('@')
    # find include dir by trying the source dirs that contain every #include "x"
    src=open(f,errors='replace').read()
    dirs=['-I'+odir] if odir else []
    for m in ([] if odir else re.finditer(r'#\s*include\s*"([^"]+)"',src)):
        for d,_,fs in os.walk(T):
            if m.group(1) in fs: dirs.append('-I'+d); break
    extra=dirs+INC+sorted(set(dirs[1:]))
    r=run(f,extra)
    if not any(SIG.search(l) for l in r.stderr.split('\n')):
        r=run(f,extra,'')
    msg=[l for l in r.stderr.split('\n') if SIG.search(l)]
    print('=====',os.path.basename(f),msg[:1])
    if not msg: continue
    ipath=''+os.path.join(os.environ.get('TMPDIR','/tmp'),'pcctriage-')+'tri.i'
    subprocess.run([P,'-E']+extra+['-o',ipath,f],capture_output=True)
    L=[l for l in open(ipath).read().split('\n') if not l.startswith('#')]
    key=re.sub(r'^.*?(compiler error|internal)','\\1',msg[0])[:30]
    def crash(ls):
        open(''+os.path.join(os.environ.get('TMPDIR','/tmp'),'pcctriage-')+'tri.c','w').write('\n'.join(ls)+'\n')
        rr=run(''+os.path.join(os.environ.get('TMPDIR','/tmp'),'pcctriage-')+'tri.c',[])
        return any(key[:20] in l for l in rr.stderr.split('\n'))
    if not crash(L): print("  (does not reproduce after preprocessing)"); continue
    cur=L;n=len(cur)//2
    while n>=1:
        i=0;ch=False
        while i<len(cur):
            t=cur[:i]+cur[i+n:]
            if crash(t): cur=t;ch=True
            else:i+=n
        if not ch:n//=2
    print('\n'.join(l for l in cur if l.strip()))
    open(''+os.path.join(os.environ.get('TMPDIR','/tmp'),'pcctriage-')+'tri.c','w').write('\n'.join(cur)+'\n')
    subprocess.run([P,'-E','-o',''+os.path.join(os.environ.get('TMPDIR','/tmp'),'pcctriage-')+'tri.i',''+os.path.join(os.environ.get('TMPDIR','/tmp'),'pcctriage-')+'tri.c'])
    g=subprocess.run(['gdb','-batch','-ex','run','-ex','bt','--args',''+os.environ.get('CCOM','pdp11-bsd-ccom')+'','-xtemps',''+os.path.join(os.environ.get('TMPDIR','/tmp'),'pcctriage-')+'tri.i',''+os.path.join(os.environ.get('TMPDIR','/tmp'),'pcctriage-')+'tri.out'],capture_output=True,text=True,encoding='utf-8',errors='replace',env=env)
    print('\n'.join(l[:150] for l in g.stdout.split('\n') if re.match(r'#[0-5] |Program received',l)))
