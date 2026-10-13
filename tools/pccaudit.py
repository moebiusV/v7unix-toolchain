#!/usr/bin/env python3
# pccaudit: see tools/README-pccfuzz.txt
import subprocess,glob,os,re,sys,collections,concurrent.futures as cf
top=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
files=[]
for root in ('orig/usr/src','pcc99/usr/src','modern/usr/src'):
    for d,_,fs in os.walk(os.path.join(top,root)):
        for f in fs:
            if f.endswith('.c') and not f.endswith('.host.c'): files.append(os.path.join(d,f))
files.sort()
configs=[['-S'],['-O','-S'],['-O','-xssa','-S']]
P=os.environ.get('PCC','pcc')
env=dict(os.environ,PCC_NORETRY='1')
bad=re.compile(r'internal compiler error|major internal|Cannot generate|cannot color|usednodes|compiler error|Assertion|Segmentation|terminated with status|cannot recover')
def one(args):
    f,cfg=args
    d=os.path.dirname(f)
    inc=['-I'+d,'-I'+top+'/orig/usr/include','-I'+top+'/modern/usr/include','-I'+top+'/pcc99/usr/include','-Dunix']
    try:
        r=subprocess.run([P]+cfg+inc+['-o','/dev/null',f],capture_output=True,text=True,env=env,timeout=120)
    except subprocess.TimeoutExpired:
        return (f,cfg,'TIMEOUT')
    if r.returncode!=0:
        m=[l for l in r.stderr.split('\n') if bad.search(l) and 'warning' not in l]
        # classify: a diagnostic about the source is not a crash
        crash=[l for l in m if re.search(r'internal compiler error|major internal|Cannot generate|cannot color|usednodes|Assertion|Segmentation|compiler error',l)]
        if crash: return (f,cfg,crash[0])
    return None
jobs=[(f,c) for f in files for c in configs]
res=[]
with cf.ThreadPoolExecutor(8) as ex:
    for r in ex.map(one,jobs):
        if r: res.append(r)
cat=collections.defaultdict(list)
for f,c,m in res:
    k=re.sub(r'^.*?(compiler error: |error: )','',m)
    k=re.sub(r'0x[0-9a-f]+','0x…',k); k=re.sub(r'\d+','N',k)
    cat[k].append((os.path.relpath(f,top),' '.join(c)))
print(len(files),"files x",len(configs),"configs;",len(res),"crashes")
for k,v in sorted(cat.items(),key=lambda x:-len(x[1])):
    print("\n[%d] %s"%(len(v),k))
    for f,c in v[:6]: print("    ",f,"(",c,")")
