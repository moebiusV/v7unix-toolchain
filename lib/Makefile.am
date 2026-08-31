# lib — PDP-11 target runtime, installed under ${libdir} (= .../lib/v7unix).
#
# The startup objects (crt0.o, ...) are V7 /usr/src/libc/csu, assembled by our
# own `as`; they are byte-identical to the V7 disk image's.  libc.a is built
# from V7's own /usr/src/libc (orig/) by the ported cc/as — see build-libc.sh.

AS = $(top_builddir)/modern/usr/src/cmd/as/as

v7unix_lib_DATA = crt0.o fcrt0.o mcrt0.o fmcrt0.o libc.a

%.o: %.s
	$(AS) -u -o $@ $<

# libc.a: compile V7's /usr/src/libc (compall order) and archive it (mklib
# order) with the ported toolchain, inside the v7check harness (V7 headers +
# cc/as/ld on PATH).  The member bytes are byte-identical to the V7 image's.
libc.a: $(srcdir)/build-libc.sh $(top_srcdir)/tools/mkvar.py $(top_srcdir)/tools/v7check.sh
	$(top_srcdir)/tools/v7check.sh $(srcdir)/build-libc.sh $@

CLEANFILES = $(v7unix_lib_DATA)
EXTRA_DIST = crt0.s fcrt0.s mcrt0.s fmcrt0.s build-libc.sh README
