#!/usr/bin/env python3
# pccfuzz: see tools/README-pccfuzz.txt
import subprocess,os,random,re,sys,collections,concurrent.futures as cf,hashlib
top=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
seed=int(sys.argv[1]) if len(sys.argv)>1 else 1
nmut=int(sys.argv[2]) if len(sys.argv)>2 else 20
random.seed(seed)
files=[]
for root in ('orig/usr/src/libc','orig/usr/src/cmd','pcc99/usr/src/cmd'):
    for d,_,fs in os.walk(os.path.join(top,root)):
        for f in fs:
            if f.endswith('.c') and os.path.getsize(os.path.join(d,f))<30000: files.append(os.path.join(d,f))
files.sort(); random.shuffle(files); files=files[:60]
P=os.environ.get('PCC','pcc')
env=dict(os.environ,PCC_NORETRY='1')
work=os.path.join(os.environ.get('TMPDIR','/tmp'),'pccfuzz')
os.makedirs(work,exist_ok=True)
crash=re.compile(r'internal compiler error|major internal|Assertion|Cannot generate|cannot color|usednodes|compiler error: (?!too many)')
def mutate(src):
    L=src.split('\n')
    k=random.choice(['dline','dtok','swap','dchar','dup','brace'])
    if k=='dline' and len(L)>2:
        i=random.randrange(len(L)); del L[i]
    elif k=='dup' and len(L)>2:
        i=random.randrange(len(L)); L.insert(random.randrange(len(L)),L[i])
    elif k=='swap' and len(L)>3:
        i,j=random.randrange(len(L)),random.randrange(len(L)); L[i],L[j]=L[j],L[i]
    else:
        s='\n'.join(L)
        if k=='brace':
            idx=[m.start() for m in re.finditer(r'[{}()\[\];,]',s)]
            if idx:
                p=random.choice(idx); s=s[:p]+s[p+1:]
        elif k=='dtok':
            toks=[m for m in re.finditer(r'[A-Za-z_]\w*|\d+|[^\sA-Za-z_\d]',s)]
            if toks:
                m=random.choice(toks); s=s[:m.start()]+s[m.end():]
        else:
            if s:
                p=random.randrange(len(s)); s=s[:p]+s[p+1:]
        return s
    return '\n'.join(L)
jobs=[]
for f in files:
    src=open(f,errors='replace').read()
    for n in range(nmut):
        jobs.append((f,mutate(src),n))
def run(job):
    f,src,n=job
    h=hashlib.md5((f+str(n)+src).encode()).hexdigest()[:10]
    p=os.path.join(work,h+'.c')
    open(p,'w').write(src)
    d=os.path.dirname(f)
    for cfg in (['-S'],['-O','-S']):
        try:
            r=subprocess.run([P]+cfg+['-I'+d,'-I'+top+'/orig/usr/include','-I'+top+'/modern/usr/include','-I'+top+'/pcc99/usr/include','-Dunix','-o','/dev/null',p],capture_output=True,text=True,encoding='utf-8',errors='replace',env=env,timeout=60)
        except subprocess.TimeoutExpired:
            return (f,p,'TIMEOUT')
        if r.returncode<0 or crash.search(r.stderr):
            m=[l for l in r.stderr.split('\n') if crash.search(l)]
            return (f,p,m[0] if m else 'signal %d'%r.returncode)
    return None
res=[]
with cf.ThreadPoolExecutor(8) as ex:
    for r in ex.map(run,jobs):
        if r: res.append(r)
open(os.path.join(work,'crashes.txt'),'a').write(''.join(p+' '+os.path.dirname(f)+'\n' for f,p,m in res))
print(len(jobs),"mutants;",len(res),"crashes")
cat=collections.defaultdict(list)
for f,p,m in res:
    k=m[:120]
    cat[k].append(p)
for k,v in sorted(cat.items(),key=lambda x:-len(x[1])): print("[%d] %s   e.g. %s"%(len(v),k,v[0]))
