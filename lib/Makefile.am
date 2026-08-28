# lib — PDP-11 target runtime, installed under ${libdir} (= .../lib/v7unix).
#
# The startup objects (crt0.o, ...) are V7 /usr/src/libc/csu, assembled by our
# own `as`; they are byte-identical to the V7 disk image's.  libc.a is the
# prebuilt V7 C library, copied from a real V7 image, shipped until a
# from-source build of /usr/src/libc lands.

AS = $(top_builddir)/modern/as/as

v7unix_lib_DATA = crt0.o fcrt0.o mcrt0.o fmcrt0.o
dist_v7unix_lib_DATA = libc.a

%.o: %.s
	$(AS) -u -o $@ $<

CLEANFILES = $(v7unix_lib_DATA)
EXTRA_DIST = crt0.s fcrt0.s mcrt0.s fmcrt0.s README
