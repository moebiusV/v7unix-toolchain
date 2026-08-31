#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([v7unix-toolchain], [0.1.0], [david@clearbrookdistillery.com])
AC_CONFIG_SRCDIR([modern/usr/src/cmd/cc.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIRS([m4])
AM_INIT_AUTOMAKE([foreign subdir-objects])

# Resolve a configured directory variable to an absolute path and AC_DEFINE it
# into config.h.  The Makefile keeps the unresolved ${...} form so DESTDIR and
# --prefix relocation keep working; only config.h (the cc driver / as / ld)
# needs the absolute path.  Handles the `NONE` prefix default.
AC_DEFUN([V7_DEFINE_DIR], [
  ac_saved_prefix=$prefix
  ac_saved_exec_prefix=$exec_prefix
  test "x$prefix" = xNONE && prefix=$ac_default_prefix
  test "x$exec_prefix" = xNONE && exec_prefix=$prefix
  eval ac_v7_dir="\"[$]$1\""
  eval ac_v7_dir="\"$ac_v7_dir\""
  AC_DEFINE_UNQUOTED([$2], ["$ac_v7_dir"], [$3])
  prefix=$ac_saved_prefix
  exec_prefix=$ac_saved_exec_prefix
])

# --- programs ------------------------------------------------------------
AC_PROG_CC
AC_PROG_LN_S

# -fno-strict-aliasing is a gcc/clang flag; c0/c1 need it so the optimizer
# does not exploit the strict-aliasing UB in the V7 sources, but pcc may not
# accept it.  Check before using so modern/ builds with gcc, clang and pcc.
AC_CACHE_CHECK([whether $CC accepts -fno-strict-aliasing],
  [ac_cv_c_no_strict_aliasing],
  [saved_CFLAGS=$CFLAGS
   CFLAGS="$CFLAGS -fno-strict-aliasing -Werror"
   AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [])],
     [ac_cv_c_no_strict_aliasing=yes],
     [ac_cv_c_no_strict_aliasing=no])
   CFLAGS=$saved_CFLAGS])
AS_IF([test "x$ac_cv_c_no_strict_aliasing" = xyes],
      [NO_STRICT_ALIASING=-fno-strict-aliasing],
      [NO_STRICT_ALIASING=])
AC_SUBST([NO_STRICT_ALIASING])

# cpp's #if grammar (modern/usr/src/cmd/cpp/cpy.y) is processed by bison/yacc.
AC_PROG_YACC

# c2's host source is generated from c2.c by tools/build-host.py.
AC_PATH_PROG([PYTHON3], [python3], [no])
AS_IF([test "x$PYTHON3" = "xno"],
      [AC_MSG_WARN([python3 not found; c2 will not build])])

# --- install locations ----------------------------------------------------
# Every host binary (cc, cpp, c0, c1, c2, as, as2, ld) installs under
# ${libexecdir}/v7unix; the v7-prefixed symlinks (v7cc/v7as/v7ld) go in
# ${bindir}; the PDP-11 target runtime (crt0.o, libc.a) under ${libdir}/v7unix;
# future target headers under ${includedir}/v7unix.  Custom *dir variables (so
# automake installs to the v7unix subdir): a distro that wants Debian/Arch's
# /usr/lib convention passes --libexecdir=/usr/lib (the /v7unix suffix is
# still appended) at configure time.
v7unix_libexecdir='${libexecdir}/v7unix'
v7unix_libdir='${libdir}/v7unix'
v7unix_includedir='${includedir}/v7unix'
AC_SUBST([v7unix_libexecdir])
AC_SUBST([v7unix_libdir])
AC_SUBST([v7unix_includedir])

# Bake the absolute paths into config.h for cc/as/ld.
V7_DEFINE_DIR([v7unix_libexecdir], [V7_LIBEXECDIR], [dir holding the v7unix host binaries])
V7_DEFINE_DIR([v7unix_libdir], [V7_LIBDIR], [dir holding the v7unix PDP-11 target runtime])
V7_DEFINE_DIR([v7unix_includedir], [V7_INCLUDEDIR], [dir holding the v7unix target headers])

AC_CONFIG_FILES([
  Makefile
  modern/usr/src/cmd/Makefile
  modern/usr/src/cmd/c/Makefile
  modern/usr/src/cmd/as/Makefile
  modern/usr/src/cmd/cpp/Makefile
  modern/usr/src/cmd/make/Makefile
  modern/usr/src/cmd/yacc/Makefile
  lib/Makefile
  man/Makefile
])
AC_OUTPUT
