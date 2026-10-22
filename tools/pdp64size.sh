#!/bin/sh
#
# pdp64size -- instruction counts and .text bytes of the pdp64 pcc back end, with our amd64, aarch64 and i386 on the
# same files, and the instruction-length, mnemonic and addressing-mode statistics of the pdp64 code.
# Default files: the fair corpus (what corpus.py's aarch64, amd64 and i386 builds all compile).
#
# usage: pdp64size.sh [--set fair|pcctests|all] [--arch pdp64,amd64] [-j N] [--nowrap] [--keep DIR] [files.c ...]
# needs: tools/corpus-build.sh pdp64 amd64 aarch64 i386 (builds/ours/<arch>/ccom), ~/claude/pdp64/build/as64
#
exec python3 "$(dirname "$0")/pdp64size.py" "$@"
