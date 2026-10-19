#!/usr/bin/env python3
"""
llcount.py -- for every PDP-11 corpus file: how many calls to the long long helpers (and to the unsigned long
shift helper) the fork's output has.  Writes $LLCOUNT_OUT (default /tmp/llcount.json); llsplit.py reads it.
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
rows=[json.loads(l) for l in open(RESULTS)]
rows=[r for r in rows if r['arch']=='pdp11' and 'stock-O' in r['size'] and 'ours-O' in r['size'] and r['corp']!='v7']
def one(r):
    c=m.CORPORA[r['corp']]; src=os.path.join(ROOT,r['src']) if not os.path.isabs(r['src']) else r['src']
    incs=[]
    for i in c['inc']: incs+=['-I',os.path.join(ROOT,i)]
    incs+=['-I',os.path.dirname(src)]
    sysinc=c.get('sys') or os.path.join(ROOT,'sysroot',a['musl'],'include')
    std=[] if c.get('sys') else ['-std=gnu99','-D__STDC_VERSION__=199901L','-D__STDC_HOSTED__=1']
    cmd=['gcc','-E','-P','-undef','-nostdinc']+std+a['cpp']+c['defs']+incs+['-isystem',sysinc,src]
    p=subprocess.run(cmd,capture_output=True,text=True,errors='replace')
    if p.returncode: return None
    t=re.sub(r'int :8\*\(sizeof\(time_t\)-sizeof\(long\)\)\*\(1234[!=]=4321\);','',p.stdout)
    with tempfile.TemporaryDirectory() as w:
        open(w+'/i.c','w').write(t)
        res={}
        for k,fl in (('ours',['-xdmr','-xtemps','-xdeljumps','-xinline','-xdce']),):
            q=subprocess.run([B+'/ours/pdp11/ccom']+fl+[w+'/i.c',w+'/o.s'],capture_output=True,text=True,errors='replace',timeout=120)
            if q.returncode: return None
            s=open(w+'/o.s',errors='replace').read()
            calls=collections.Counter(re.findall(r'jsr\tpc,\*\$(__ll\w+|__[a-z]+di\w*)',s))
            return dict(src=r['src'],corp=r['corp'],stock=r['size']['stock-O'],ours=r['size']['ours-O'],gcc=r['size'].get('gcc-Os',0),ll=sum(calls.values()),calls=dict(calls))
out=[]
with ThreadPoolExecutor(4) as ex:
    for x in ex.map(one, rows):
        if x: out.append(x)
json.dump(out,open(os.environ.get('LLCOUNT_OUT','/tmp/llcount.json'),'w'))
print(len(out),'files')
