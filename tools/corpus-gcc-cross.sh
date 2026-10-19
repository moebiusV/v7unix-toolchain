#!/bin/sh
#
# corpus-gcc-cross -- build GNU binutils and a C-only GCC cross compiler for the two old targets GCC
# still has back ends for (vax-netbsdelf, pdp11-aout), so corpus.py can put GCC next to stock and our
# pcc on them.  Compiler only (no libc, no libgcc): enough for `gcc -c` and `size`.
#
# usage: corpus-gcc-cross.sh [dir]     (default $CORPORA/gcc-cross; result in dir/prefix)
#
set -u
D=${1:-${CORPORA:-$HOME/claude/corpora}/gcc-cross}
BV=${BINUTILS:-2.43.1}; GV=${GCC:-14.2.0}
mkdir -p "$D" && cd "$D"
P=$D/prefix
[ -f binutils-$BV.tar.xz ] || curl -sSLO https://ftp.gnu.org/gnu/binutils/binutils-$BV.tar.xz
[ -f gcc-$GV.tar.xz ] || curl -sSLO https://ftp.gnu.org/gnu/gcc/gcc-$GV/gcc-$GV.tar.xz
[ -d binutils-$BV ] || tar xf binutils-$BV.tar.xz
[ -d gcc-$GV ] || tar xf gcc-$GV.tar.xz
[ -d gcc-$GV/mpfr ] || (cd gcc-$GV && ./contrib/download_prerequisites >/dev/null 2>&1)
for t in vax-netbsdelf pdp11-aout; do
	if [ ! -x $P/bin/$t-as ]; then
		mkdir -p b-$t && (cd b-$t && ../binutils-$BV/configure --target=$t --prefix=$P --disable-nls \
		    --disable-gdb --disable-werror MAKEINFO=true >cfg.log 2>&1 && nice make -j2 MAKEINFO=true >mk.log 2>&1 && make install MAKEINFO=true >inst.log 2>&1)
	fi
	if [ ! -x $P/bin/$t-gcc ]; then
		mkdir -p g-$t && (cd g-$t && ../gcc-$GV/configure --target=$t --prefix=$P --enable-languages=c \
		    --without-headers --with-newlib --disable-nls --disable-shared --disable-threads --disable-libssp \
		    --disable-libquadmath --disable-libgomp --disable-multilib --disable-bootstrap --disable-lto \
		    --disable-werror MAKEINFO=true >cfg.log 2>&1 && nice make -j2 all-gcc MAKEINFO=true >mk.log 2>&1 && make install-gcc MAKEINFO=true >inst.log 2>&1)
	fi
	echo "$t: $( [ -x $P/bin/$t-gcc ] && echo ok || echo FAILED)"
done
