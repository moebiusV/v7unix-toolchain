#!/usr/bin/env python3
"""
x86exp.py ARCH N -- is ours smaller than stock on amd64/i386 because of its optimizations or because stock gets
something wrong?  Compiles about N sample files three ways: stock, the fork without -xdmr, the fork with it.
Without -xdmr the fork should be the same compiler as stock (it is within a percent or two); the rest is -xdmr.
Needs the corpora and builds of tools/corpus.py.
"""
import os
HERE = os.path.dirname(os.path.abspath(__file__))
CORPUS_PY = os.path.join(HERE, '..', 'corpus.py')
import sys, importlib.util, random, json
RESULTS = os.environ.get('CORPUS_RESULTS', '/tmp/co-final/results.jsonl')
sp=importlib.util.spec_from_file_location('corpus',CORPUS_PY); m=importlib.util.module_from_spec(sp); sp.loader.exec_module(m)
from concurrent.futures import ThreadPoolExecutor
arch=sys.argv[1]; N=int(sys.argv[2])
random.seed(5)
jobs=[]
for corp in ('lua','zlib','bearssl','mbedtls','chibicc','lz4','cjson','c-testsuite','gcc-torture'):
    if corp not in m.CORPORA: continue
    fs=m.CORPORA[corp]['files']()
    if m.CORPORA[corp].get('stride'): fs=fs[::m.CORPORA[corp]['stride']]
    random.shuffle(fs)
    jobs += [(corp,arch,f) for f in fs[:max(4,N//9)]]
orig=m.ccomflags
def run(mode):
    # mode: 'full' = the harness' flags for ours; 'nodmr' = the same without -xdmr
    m.ccomflags = (lambda a_,o: orig(a_,o)) if mode=='full' else (lambda a_,o: orig(a_,False))
    with ThreadPoolExecutor(4) as ex:
        return list(ex.map(lambda j: m.one(j,None), jobs))
full=run('full'); nodmr=run('nodmr')
tot=dict(stock=0,full=0,nodmr=0,n=0); bad=0
for f,n in zip(full,nodmr):
    if 'stock-O' in f['size'] and 'ours-O' in f['size'] and 'ours-O' in n['size']:
        tot['stock']+=f['size']['stock-O']; tot['full']+=f['size']['ours-O']; tot['nodmr']+=n['size']['ours-O']; tot['n']+=1
print(arch, tot, 'ours-without-dmr vs stock: %+.2f%%  ours-with-dmr vs stock: %+.2f%%'%(100*(tot['nodmr']-tot['stock'])/tot['stock'],100*(tot['full']-tot['stock'])/tot['stock']))
