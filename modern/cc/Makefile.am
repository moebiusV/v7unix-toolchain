# cc — the V7 C compiler driver, host-ported.
#
# Just the driver: it forks/execs the passes (cpp, c0, c1, c2), as and ld.
# The pass paths default to V7_LIBEXECDIR (baked in via config.h) and can be
# redirected with -B <dir> or the V7_* environment variables.

AM_CFLAGS = -std=c99 -O1

v7unix_libexec_PROGRAMS = cc
cc_SOURCES = cc.c

EXTRA_DIST = README
