#!/usr/bin/env python3
"""
pcchist-analyze FILE -- read a simh `show cpu history` dump and report the
instructions executed and a count of memory references (instruction words
fetched plus operand reads and writes), a rough speed proxy for the PDP-11.
"""
import re, sys

TWO = {'MOV','MOVB','ADD','SUB','CMP','CMPB','BIT','BITB','BIC','BICB','BIS','BISB','XOR','MUL','DIV','ASH','ASHC'}
STORE = {'MOV','MOVB'}            # destination written, not read
COMPARE = {'CMP','CMPB','BIT','BITB'}  # destination read, not written
ONE = {'CLR','CLRB','COM','COMB','INC','INCB','DEC','DECB','NEG','NEGB','ADC','SBC','TST','TSTB',
       'ROR','ROL','ASR','ASL','RORB','ROLB','ASRB','ASLB','SXT','SWAB','JMP'}
ONE_WRITEONLY = {'CLR','CLRB','SXT'}
ONE_READONLY = {'TST','TSTB'}

def operand(s):
    """-> (extra instruction words, data refs for one access, is_reg)"""
    s = s.strip()
    d = 1 if s.startswith('@') else 0      # deferred: pointer fetch
    t = s.lstrip('@')
    if re.fullmatch(r'(R[0-5]|SP|PC)', t):
        return (0, 0, True)
    if re.fullmatch(r'#.*', t):
        return (1, 0, False) if d == 0 else (1, 1, False)
    if re.fullmatch(r'\(\w+\)\+?', t) or re.fullmatch(r'-\(\w+\)', t):
        return (0, 1 + d, False)
    if re.fullmatch(r'.+\(\w+\)', t):      # index
        return (1, 1 + d, False)
    return (1, 1 + d, False)               # pc-relative / absolute

def main():
    n = words = reads = writes = 0
    for line in open(sys.argv[1], errors='replace'):
        m = re.match(r'^[0-7]{6} [0-7]{6}\|.{6,14}\s{2}(\S.*)$', line.rstrip('\n'))
        if not m:
            m = re.match(r'^[0-7]{6} [0-7]{6}\|.{7}\s+.{7}\s+(\S.*)$', line.rstrip('\n'))
        if not m:
            continue
        text = m.group(1).strip()
        parts = text.split(None, 1)
        op = parts[0]
        args = [a for a in re.split(r',(?![^(]*\))', parts[1])] if len(parts) > 1 else []
        n += 1
        words += 1
        if op in TWO and len(args) == 2:
            for k, a in enumerate(args):
                ew, rf, isreg = operand(a)
                words += ew
                if isreg: continue
                if k == 0:
                    reads += rf
                else:
                    if op in STORE:
                        writes += 1; reads += rf - 1
                    elif op in COMPARE:
                        reads += rf
                    else:
                        reads += rf; writes += 1
        elif op in ONE and len(args) == 1:
            ew, rf, isreg = operand(args[0])
            words += ew
            if not isreg:
                if op in ONE_WRITEONLY: writes += 1; reads += rf - 1
                elif op in ONE_READONLY: reads += rf
                else: reads += rf; writes += 1
        elif op == 'JSR':
            writes += 1
            if len(args) == 2:
                ew, rf, isreg = operand(args[1]); words += ew
        elif op == 'RTS':
            reads += 1
        elif op in ('MOVF','MOVIF','MOVFI','ADDF','SUBF','MULF','DIVF','CMPF','LDF','STF','NEGF','TSTF'):
            for a in args:
                ew, rf, isreg = operand(a)
                words += ew
                if not isreg: reads += 2 * rf
    print("instructions %d  words-fetched %d  data-reads %d  data-writes %d  memory-refs %d" %
          (n, words, reads, writes, words + reads + writes))
main()
