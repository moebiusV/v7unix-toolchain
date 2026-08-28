# c1 — V7's PDP-11 code generator, host-ported.
#
# The committed host c1*.c are generated from the K&R sources; see the
# c1.rules.json / c1.spec.json specs in tools/.  table.c is re-serialized from
# the assembled machine-description table (tools/table2c.py).

AM_CFLAGS = -std=c99 -O1 @NO_STRICT_ALIASING@

v7unix_libexec_PROGRAMS = c1
c1_SOURCES = c10.c c11.c c12.c c13.c table.c globals.c
noinst_HEADERS = c1.h
