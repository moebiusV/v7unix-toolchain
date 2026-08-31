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

# --- froot1: the minimal self-hosting V7 cross-compilation root --------------
# froot1/ is a synthetic V7 root (bin/ lib/ usr/include usr/src) holding the
# toolchain plus the phase-1 makefile commands (sh/cp/mv/rm/cmp) -- the minimal
# set that compiles itself from the orig/ source tree.  `make froot1` assembles
# it; `make froot1-dist` tars it as froot1.tar.gz (needs the unixtree V7
# headers; set V7CHECK_UNIXTREE if they aren't at ../../unixtree).
.PHONY: froot1 froot1-dist
froot1: all
	$(srcdir)/tools/mkfroot.sh
froot1-dist: froot1
	tar czf froot1.tar.gz froot1
	@echo "built froot1.tar.gz"
