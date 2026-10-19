"""
inlexp*.py -- the experiment behind the inlining size limit: total size of a sample of the corpus for several limits
(PCC_INLMAX is gone from the compiler; rebuild with -DINLMAX=n to repeat it).  inlexp.py is the PDP-11, inlexp2.py the others.
"""
import os
import sys, importlib.util, os, subprocess, re, json, tempfile, random
from concurrent.futures import ThreadPoolExecutor
sp=importlib.util.spec_from_file_location('corpus',os.path.join(os.path.dirname(os.path.abspath(__file__)),'..','corpus.py')); m=importlib.util.module_from_spec(sp); sp.loader.exec_module(m)
a=m.ARCH['pdp11']; ROOT=m.ROOT
CCOM=os.path.expanduser('~/claude/pcc/cc/ccom/pdp11-bsd-ccom')
d=[x for x in json.load(open('/tmp/llcount.json')) if x['gcc']>0 and sum(v for k,v in x['calls'].items() if k!='__llulr')==0]
random.seed(3); random.shuffle(d); d=d[:int(sys.argv[1])]
# preprocess once
pre={}
def prep(x):
    c=m.CORPORA[x['corp']]; src=os.path.join(ROOT,x['src'])
    incs=[]
    for i in c['inc']: incs+=['-I',os.path.join(ROOT,i)]
    incs+=['-I',os.path.dirname(src)]
    sysinc=os.path.join(ROOT,'sysroot',a['musl'],'include')
    cmd=['gcc','-E','-P','-undef','-nostdinc','-std=gnu99','-D__STDC_VERSION__=199901L','-D__STDC_HOSTED__=1']+a['cpp']+c['defs']+incs+['-isystem',sysinc,src]
    t=subprocess.run(cmd,capture_output=True,text=True,errors='replace').stdout
    return x['src'], re.sub(r'int :8\*\(sizeof\(time_t\)-sizeof\(long\)\)\*\(1234[!=]=4321\);','',t)
with ThreadPoolExecutor(4) as ex:
    for k,t in ex.map(prep,d): pre[k]=t
def size(item, env):
    k,t=item
    with tempfile.TemporaryDirectory() as w:
        open(w+'/i.c','w').write(t)
        e=dict(os.environ); e.update(env)
        q=subprocess.run([CCOM,'-xdmr','-xtemps','-xdeljumps','-xinline','-xdce',w+'/i.c',w+'/o.s'],capture_output=True,text=True,errors='replace',env=e,timeout=120)
        if q.returncode:
            print('  FAIL', k, (q.stderr or q.stdout).strip().split(chr(10))[0][:100]); return None
        subprocess.run([m.c2_for('pdp11','ours'),w+'/o.s',w+'/c.s'])
        sz,err=m.old_assemble('pdp11',w+'/c.s',w+'/o.o',w)
        return sz
res={}
keys=list(pre)
for lim in sys.argv[2:]:
    env={} if lim=='inf' else {'PCC_INLMAX':lim}
    with ThreadPoolExecutor(4) as ex: r=list(ex.map(lambda it: size(it,env), pre.items()))
    res[lim]=dict(zip(keys,r))
common=[k for k in keys if all(res[l][k] for l in res)]
base=sum(res[sys.argv[2]][k] for k in common)
print('files compiled under every limit:',len(common),'of',len(keys))
for l in res:
    t=sum(res[l][k] for k in common)
    print('limit %-5s total %d (%+.1f%% vs %s)'%(l,t,100.0*(t-base)/base,sys.argv[2]))
