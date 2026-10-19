"""
inlexp*.py -- the experiment behind the inlining size limit: total size of a sample of the corpus for several limits
(PCC_INLMAX is gone from the compiler; rebuild with -DINLMAX=n to repeat it).  inlexp.py is the PDP-11, inlexp2.py the others.
"""
import os
import sys, importlib.util, os, random, tempfile, shutil
from concurrent.futures import ThreadPoolExecutor
sp=importlib.util.spec_from_file_location('corpus',os.path.join(os.path.dirname(os.path.abspath(__file__)),'..','corpus.py')); m=importlib.util.module_from_spec(sp); sp.loader.exec_module(m)
S='/tmp/claude-1000/-home-david-claude-facetlang-pdp11-v7unix-toolchain/d4ecc0bd-4d06-42fb-bfd3-3e331cd4baac/scratchpad/'
BIN={'amd64':S+'a64/cc/ccom/x86_64-unknown-linux-ccom','i386':S+'x86/cc/ccom/i386-unknown-linux-ccom','vax':S+'v32c/cc/ccom/vax-unknown-unix32v-ccom','aarch64':S+'arm64/cc/ccom/aarch64-unknown-linux-ccom'}
arch=sys.argv[1]; N=int(sys.argv[2]); lims=sys.argv[3:]
T=tempfile.mkdtemp(); os.makedirs(T+'/ours/'+arch)
os.symlink(BIN[arch],T+'/ours/'+arch+'/ccom')
for f in ('c2pdp11','c2vax'): os.symlink(os.path.join(m.BUILDS,f),T+'/'+f)
real=m.BUILDS; m.BUILDS=T; m.ALLKEYS=('ours-O',)
random.seed(9); jobs=[]
for corp in ('lua','zlib','bearssl','mbedtls','chibicc','lz4','cjson','c-testsuite','gcc-torture'):
    fs=m.CORPORA[corp]['files']()
    if m.CORPORA[corp].get('stride'): fs=fs[::m.CORPORA[corp]['stride']]
    random.shuffle(fs); jobs+=[(corp,arch,f) for f in fs[:max(4,N//9)]]
res={}
for lim in lims:
    if lim=='inf': os.environ.pop('PCC_INLMAX',None)
    else: os.environ['PCC_INLMAX']=lim
    with ThreadPoolExecutor(4) as ex: r=list(ex.map(lambda j: m.one(j,None), jobs))
    res[lim]={j[2]:x['size'].get('ours-O') for j,x in zip(jobs,r)}
keys=[j[2] for j in jobs]
common=[k for k in keys if all(res[l][k] for l in res)]
base=sum(res[lims[0]][k] for k in common)
print(arch,'files',len(common),'of',len(keys))
for l in lims:
    t=sum(res[l][k] for k in common); print('  limit %-5s total %9d (%+.1f%%)'%(l,t,100.0*(t-base)/base))
