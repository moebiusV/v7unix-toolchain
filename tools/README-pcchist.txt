Speed proxy for the PDP-11 code (no cycle-accurate simulator available):

  pcchist.sh [pcc flags] x.c      like pcctest.sh, but leaves simh's CPU
                                  history in $HISTOUT (default /tmp/pcchist.out)
  pcchist-analyze.py $HISTOUT     instructions executed, instruction words
                                  fetched, operand reads and writes
  pcchist-bench/*.c               small kernels (int test()) used for the
                                  comparison in DMR-OPTIMIZATIONS-OTHER-TARGETS.md

The history holds the last 100000 instructions, so kernels must be short.
Memory references are a fair proxy for speed on these machines, but this is
not a timing model.
