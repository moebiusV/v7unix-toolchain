# v7unix-toolchain — V7 PDP-11 C toolchain, modernized for a modern host.
#
# Top-level build.  SUBDIRS build the passes in dependency order (lib needs
# our own `as`), then install everything.  `orig/`, `c99/`, `tools/` and
# `dist/` are source/packaging trees shipped verbatim in the tarball.

SUBDIRS = modern/usr/src/cmd lib man

EXTRA_DIST = \
	orig \
	c99 \
	tools \
	dist/rpm \
	dist/debian \
	dist/arch \
	dist/alpine \
	dist/freebsd \
	dist/openbsd \
	dist/netbsd \
	dist/gentoo \
	dist/nixos \
	dist/slackware \
	modern/usr/include \
	PORTING.md \
	README.md \
	README.distributions \
	TODO

# v7-prefixed convenience symlinks in $(bindir) → the libexec/v7unix binaries.
# (All the real binaries live in $(libexecdir)/v7unix under their original V7
# names; the v7- prefix is just the user-facing entry point.)
install-exec-hook:
	mkdir -p $(DESTDIR)$(bindir)
	cd $(DESTDIR)$(bindir) && \
		$(LN_S) -f $(libexecdir)/v7unix/cc v7cc && \
		$(LN_S) -f $(libexecdir)/v7unix/as v7as && \
		$(LN_S) -f $(libexecdir)/v7unix/ld v7ld

uninstall-hook:
	rm -f $(DESTDIR)$(bindir)/v7cc \
	      $(DESTDIR)$(bindir)/v7as \
	      $(DESTDIR)$(bindir)/v7ld

# --- froot: the self-contained V7 cross-compilation root ---------------------
# froot/ is a synthetic V7 root (bin/ lib/ usr/include usr/src) holding only the
# toolchain's binaries plus the V7 headers and reference source tree -- enough to
# compile the whole V7 source tree with its original makefiles and pathnames.
# `make froot` assembles it; `make froot-dist` tars it as froot.tar.gz (needs the
# unixtree V7 headers; set V7CHECK_UNIXTREE if they aren't at ../../unixtree).
.PHONY: froot froot-dist
froot: all
	$(srcdir)/tools/mkfroot.sh
froot-dist: froot
	tar czf froot.tar.gz froot
	@echo "built froot.tar.gz"
