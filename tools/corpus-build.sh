#!/bin/sh
#
# corpus-build -- build a stock (origin/master) and our (working tree) pcc ccom
# for every target pcc and clang have in common, under $CORPORA/builds/{stock,ours}/<arch>/.
#
# usage: corpus-build.sh [arch ...]      (default: all)
#
set -u
PCC=${PCC:-$HOME/claude/pcc}
B=${CORPORA:-$HOME/claude/corpora}/builds
STOCKREF=${STOCKREF:-origin/master}
# arch:configure-triple (riscv and sparc64 are only accepted for the BSDs)
ALL="aarch64:aarch64-unknown-linux amd64:x86_64-unknown-linux i386:i386-unknown-linux
arm:arm-unknown-linux m68k:m68k-unknown-linux mips:mips-unknown-linux
mips64:mips64-unknown-linux powerpc:powerpc-unknown-linux
riscv64:riscv64-unknown-netbsd sparc64:sparc64-unknown-netbsd
pdp11:pdp11-bsd vax:vax-unknown-unix32v pdp64:pdp64-unknown-linux"
want=${*:-all}
mkdir -p "$B"
for kind in ${KINDS:-stock ours}; do
	for ent in $ALL; do
		a=${ent%%:*}; t=${ent#*:}
		# stock has no 32V target; its plain VAX/BSD back end is the same code generator
		[ $kind = stock ] && [ $a = vax ] && t=vax-unknown-bsd
		case " $want " in *" all "*|*" $a "*) ;; *) continue ;; esac
		d=$B/$kind/$a
		# the source this build would be made from; an unchanged tree is not rebuilt
		if [ $kind = stock ]; then
			id=$(cd "$PCC" && git rev-parse "$STOCKREF")
		else
			# only what this target is built from: the shared compiler and this target's own directory
			# (and, for the two old targets, the c2 that comes with it)
			ad=$a; [ $a = riscv64 ] && ad=riscv
			id=$(cd "$PCC" && git ls-files -z -- cc mip common os configure.ac "arch/$ad" cc2 f77 2>/dev/null | grep -zv 'pdp11-bsd-' | xargs -0 sha1sum 2>/dev/null | sha1sum | cut -c1-40)
		fi
		if [ -x "$d/ccom" ] && [ "$(cat "$d/.srcid" 2>/dev/null)" = "$id-$t" ]; then
			echo "$kind/$a: unchanged"
			continue
		fi
		rm -rf "$d"; mkdir -p "$d"
		if [ $kind = stock ]; then
			(cd "$PCC" && git archive "$STOCKREF") | tar xf - -C "$d"
		else
			# tracked files only (the working tree's versions): the checkout also holds a PDP-11
			# build whose generated files would poison every other target
			(cd "$PCC" && git ls-files -z | tar --null -T - -cf -) | tar xf - -C "$d"
		fi
		# the branch tracks a prebuilt pdp11-bsd-ccom; it must not be mistaken for this target's
		rm -f "$d"/cc/ccom/*-ccom "$d"/cc/ccom/*-cc2
		(
			cd "$d" && ./configure --target=$t >cfg.log 2>&1 &&
			    cd cc/ccom && make CFLAGS='-g -O1 -std=gnu89 -w -fcommon' >mk.log 2>&1
			c=$(ls -t "$d"/cc/ccom/*-ccom 2>/dev/null | head -1)
			if [ -n "$c" ] && [ -x "$c" ]; then
				ln -sf "$c" "$d/ccom"
				echo "$id-$t" > "$d/.srcid"
				echo "$kind/$a: ok"
			else
				echo "$kind/$a: FAILED ($(grep -m1 -E 'error' "$d/cfg.log" "$d/cc/ccom/mk.log" 2>/dev/null | head -1))"
			fi
		) &
	done
done
wait
# the peephole c2s the old targets run on -O output, built for the host (corpus.py looks in $CORPORA/builds)
gcc -std=gnu17 -w -o "$B/c2vax" "$PCC/cc2/cc2.vax.c" && gcc -std=gnu17 -w -o "$B/c2pdp11" "$PCC/cc2/cc2.pdp11.c"
