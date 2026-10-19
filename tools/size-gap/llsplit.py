#!/usr/bin/env python3
"""
llsplit.py -- the PDP-11 sizes split by whether the fork's output calls a real 64-bit helper: the files
where stock's 32-bit "long long" is wrong, and the others.  Reads $LLCOUNT_OUT.
"""
import os
HERE = os.path.dirname(os.path.abspath(__file__))
CORPUS_PY = os.path.join(HERE, '..', 'corpus.py')
RESULTS = os.environ.get('CORPUS_RESULTS', '/tmp/co-final/results.jsonl')

import json
d = json.load(open(os.environ.get('LLCOUNT_OUT', '/tmp/llcount.json')))
def n64(x): return sum(v for k, v in x['calls'].items() if k != '__llulr')   # __llulr is a 32-bit shift helper
for lab, sel in (('files whose output calls 64-bit helpers', lambda x: n64(x) > 0), ('files that do not', lambda x: n64(x) == 0)):
    xs = [x for x in d if sel(x)]
    g = [x for x in xs if x['gcc']]
    st = sum(x['stock'] for x in xs); ou = sum(x['ours'] for x in xs)
    print('%-42s %3d files  stock %8d  ours %8d  (x%.2f)  | gcc, %3d files: ours %8d gcc %8d (x%.2f)' % (
        lab, len(xs), st, ou, ou / st, len(g), sum(x['ours'] for x in g), sum(x['gcc'] for x in g),
        sum(x['ours'] for x in g) / sum(x['gcc'] for x in g)))
