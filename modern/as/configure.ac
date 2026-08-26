dnl configure.ac -- pdp11-as (the V7 PDP-11 assembler, as C)
dnl
dnl This is the faithful .s -> C translation of V7 /usr/src/cmd/as: pass 1
dnl (`as', from as11.s..as19.s) and pass 2 (`as2', from as21.s..as29.s).
dnl Both are plain C99 and build with any C99 compiler; nothing here is
dnl target-dependent, since the goal is a host-side cross-assembler.

AC_INIT([pdp11-as], [0.1], [david@example.invalid])
AC_CONFIG_SRCDIR([as1.c])
AC_PROG_CC
AC_PROG_INSTALL

dnl Plain C99 is asked for explicitly rather than relying on the compiler
dnl default, which varies (pcc's is K&R-ish, gcc's is gnu17, tcc's is C99).
AC_MSG_CHECKING([whether $CC accepts -std=c99])
save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -std=c99"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],
	[AC_MSG_RESULT([yes])],
	[AC_MSG_RESULT([no]); CFLAGS="$save_CFLAGS"])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
