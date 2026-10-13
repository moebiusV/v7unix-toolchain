#!/bin/bash
#
# pccboot -- bootstrap test of pcc on x86-64: pcc compiles pcc, twice.
#
# stage0  pcc built by gcc
# stage1  pcc built by stage0 (ccom flags below, -xdmr and the rest)
# stage2  pcc built by stage1
# stage1 and stage2 must be byte identical: the compiler, the preprocessor,
# the driver and every object file.  Any difference, or a stage that does not
# run, is a miscompile.
#
# usage: pccboot.sh [WORKDIR [ccom flags...]]
#   PCCSRC=path to the pcc git tree (default ~/claude/pcc); HEAD is used.
#   The system headers are glibc's, with __GNUC__ undefined; stdarg.h and
#   stddef.h come from WORKDIR/inc (pcc ships no freestanding headers).
#
set -u
W=${1:-$PWD/pccboot-work}; [ $# -gt 0 ] && shift
XF="${*:--xdmr -xtemps -xdeljumps -xdce -xinline}"
PCCSRC=${PCCSRC:-$HOME/claude/pcc}
SRC=$W/src; INST=$W/inst; INCD=$W/inc
INC="-I$INCD -I/usr/include/x86_64-linux-gnu -I/usr/include"
DIRS="cc/cpp cc/ccom cc/cc"
log() { echo "== $(date +%T) $*"; }

mkdir -p "$W" "$INCD"
cat > $INCD/stddef.h <<'EOH'
#ifndef _PCC_STDDEF_H
#define _PCC_STDDEF_H
typedef unsigned long size_t;
typedef long ptrdiff_t;
typedef int wchar_t;
#define NULL ((void *)0)
#define offsetof(t, m) ((size_t)&(((t *)0)->m))
#endif
EOH
cat > $INCD/stdarg.h <<'EOH'
#ifndef _PCC_STDARG_H
#define _PCC_STDARG_H
typedef __builtin_va_list va_list;
typedef __builtin_va_list __gnuc_va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, t) __builtin_va_arg(ap, t)
#define va_end(ap) __builtin_va_end(ap)
#define va_copy(d, s) __builtin_va_copy(d, s)
#endif
EOH

log "fresh source tree and stage0 (gcc)"
rm -rf $SRC $INST $W/s1 $W/s2; mkdir -p $SRC $W/s1 $W/s2
git -C "$PCCSRC" archive HEAD | tar x -C $SRC
(cd $SRC && ./configure --prefix=$INST > $W/cfg.log 2>&1) || { echo "configure failed"; exit 1; }
for d in $DIRS; do (cd $SRC/$d && make -k > $W/mk0-$(basename $d).log 2>&1; make -k install > /dev/null 2>&1); done
[ -x $INST/bin/pcc ] && [ -x $INST/libexec/ccom ] || { echo "stage0 did not build"; exit 1; }

wrapper() {	# compile with pcc ($1), link with gcc
cat > $W/pccw <<EOW
#!/bin/sh
compile=
for a; do [ "\$a" = -c ] && compile=1; done
if [ -n "\$compile" ]; then exec $1 $XF -U__GNUC__ $INC "\$@"; fi
args=
for a; do case \$a in -x*|-U__GNUC__) ;; *) args="\$args \$a";; esac; done
exec gcc \$args
EOW
chmod +x $W/pccw
}

stage() {	# $1 = name, $2 = pcc driver
  log "stage $1 with $2"
  wrapper $2
  for d in $DIRS; do
    (cd $SRC/$d && rm -f *.o && make -k CC=$W/pccw CFLAGS= > $W/mk-$1-$(basename $d).log 2>&1)
    if grep -q "\*\*\*" $W/mk-$1-$(basename $d).log; then echo "BUILD FAILED in $d (stage $1):"; grep -m3 -B2 "\*\*\*" $W/mk-$1-$(basename $d).log; exit 1; fi
  done
  cp $SRC/cc/ccom/ccom $W/$1/ccom; cp $SRC/cc/cpp/cpp $W/$1/cpp
  cp $SRC/cc/cc/pcc $W/$1/pcc 2>/dev/null || cp $SRC/cc/cc/cc $W/$1/pcc
  mkdir -p $W/$1/o; for d in $DIRS; do cp $SRC/$d/*.o $W/$1/o/ 2>/dev/null; done
}

stage s1 $INST/bin/pcc
cp $W/s1/ccom $INST/libexec/ccom; cp $W/s1/cpp $INST/libexec/cpp; cp $W/s1/pcc $INST/bin/pcc
echo 'int printf(const char *, ...); int main(void) { printf("hello %d\n", 6*7); return 0; }' > $W/hello.c
$INST/bin/pcc -S $INC -o $W/hello.s $W/hello.c || { echo "stage1 compiler does not run"; exit 1; }
stage s2 $INST/bin/pcc
log compare
rc=0
for f in ccom cpp pcc; do cmp $W/s1/$f $W/s2/$f && echo "$f identical" || rc=1; done
n=0; d=0
for o in $W/s1/o/*.o; do b=$(basename $o); n=$((n+1)); cmp -s $o $W/s2/o/$b || { d=$((d+1)); echo "object differs: $b"; rc=1; }; done
echo "objects: $n compared, $d differ"
[ $rc = 0 ] && echo "BOOTSTRAP OK" || echo "BOOTSTRAP FAILED"
exit $rc
