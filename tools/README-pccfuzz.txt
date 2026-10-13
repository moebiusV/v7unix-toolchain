Compiler robustness tools for the pdp11 pcc (run with PCC=/path/to/pcc):

  pccaudit.py            compile every .c under orig/pcc99/modern with -S, -O,
                         -O -xssa and report compiler crashes (retry disabled)
  pccfuzz.py SEED N      mutate real sources (N mutants per file, 60 files),
                         report crashes; paths of crashers go to
                         $TMPDIR/pccfuzz/crashes.txt
  pcctriage.py F@DIR...  reduce each crasher and print a gdb backtrace
                         (CCOM=/path/to/pdp11-bsd-ccom, needs gdb)

All set PCC_NORETRY=1 so the silent "retry without register pinning"
cannot hide a failure.  "too many errors" for as1.c (missing int8_t
header) is a clean diagnostic, not a crash.
