#!/usr/bin/env python3
"""
fnscan.py -- instructions per function, the fork against gcc, for the PDP-11 files where the fork is larger.
Writes $FN_OUT (default /tmp/fn.json).  Needs llcount.json (llcount.py) and the gcc cross build.
"""
import os
HERE = os.path.dirname(os.path.abspath(__file__))
CORPUS_PY = os.path.join(HERE, '..', 'corpus.py')
RESULTS = os.environ.get('CORPUS_RESULTS', '/tmp/co-final/results.jsonl')
import sys, importlib.util, os, subprocess, re, json, tempfile, collections
from concurrent.futures import ThreadPoolExecutor
sp=importlib.util.spec_from_file_location('corpus',CORPUS_PY); m=importlib.util.module_from_spec(sp); sp.loader.exec_module(m)
a=m.ARCH['pdp11']; ROOT=m.ROOT
B=os.path.expanduser('~/claude/corpora/builds')
G=os.path.expanduser('~/claude/corpora/gcc-cross/prefix/bin/pdp11-aout-gcc')
d=json.load(open(os.environ.get('LLCOUNT_OUT','/tmp/llcount.json')))
def n64(x): return sum(v for k,v in x['calls'].items() if k!='__llulr')
sel=[x for x in d if n64(x)==0 and x['gcc']>200 and x['ours']>x['gcc']*1.05]
def count(path, pat):
    out=collections.OrderedDict(); cur=None
    for l in open(path,errors='replace'):
        l=l.rstrip('\n')
        mm=re.match(pat,l)
        if mm: cur=mm.group(1); out.setdefault(cur,0); 
        l2=re.sub(r'^[A-Za-z_0-9~]+:','',l)
        if cur and l2 and not l2.startswith(('.','~~')) and re.match(r'^\s*[a-z]',l2) : out[cur]+=1
    return out
def one(x):
    r=x; c=m.CORPORA[x['corp']]; src=os.path.join(ROOT,x['src'])
    incs=[]
    for i in c['inc']: incs+=['-I',os.path.join(ROOT,i)]
    incs+=['-I',os.path.dirname(src)]
    sysinc=os.path.join(ROOT,'sysroot',a['musl'],'include')
    cmd=['gcc','-E','-P','-undef','-nostdinc','-std=gnu99','-D__STDC_VERSION__=199901L','-D__STDC_HOSTED__=1']+a['cpp']+c['defs']+incs+['-isystem',sysinc,src,'-D__ILP32__']
    p=subprocess.run(cmd,capture_output=True,text=True,errors='replace')
    if p.returncode: return []
    t=re.sub(r'int :8\*\(sizeof\(time_t\)-sizeof\(long\)\)\*\(1234[!=]=4321\);','',p.stdout)
    with tempfile.TemporaryDirectory() as w:
        open(w+'/i.c','w').write(t)
        q=subprocess.run([B+'/ours/pdp11/ccom','-xdmr','-xtemps','-xdeljumps','-xinline','-xdce',w+'/i.c',w+'/o.s'],capture_output=True,text=True,errors='replace',timeout=120)
        q2=subprocess.run([G,'-std=gnu99','-w','-Os','-S','-x','c','-o',w+'/g.s',w+'/i.c'],capture_output=True,text=True,errors='replace',timeout=120)
        if q.returncode or q2.returncode: return []
        o=count(w+'/o.s',r'^_(\w+):$'); g=count(w+'/g.s',r'^_(\w+):$')
        return [(x['src'].split('/')[-1],f,g[f],o[f]) for f in o if f in g and g[f]>0]
rows=[]
with ThreadPoolExecutor(4) as ex:
    for r in ex.map(one, sel): rows+=r
json.dump(rows,open(os.environ.get('FN_OUT','/tmp/fn.json'),'w'))
print(len(sel),'files',len(rows),'functions')
