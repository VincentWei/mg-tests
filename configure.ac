dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.60)
AC_INIT(mg-tests,4.0.0)
AC_CONFIG_SRCDIR(src/helloworld.c)

dnl ========================================================================
dnl Need for cross-compiling
AC_CANONICAL_SYSTEM

dnl ========================================================================
dnl Init automake
AM_INIT_AUTOMAKE

dnl ========================================================================
dnl Checks for programs.
AC_PROG_MAKE_SET
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LD
AC_PROG_RANLIB
AC_PROG_LIBTOOL

dnl ========================================================================
dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

dnl ========================================================================
dnl Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_HEADER_TIME
AC_CHECK_HEADERS(sys/time.h unistd.h)

if test "$ac_cv_prog_gcc" = "yes"; then
    CFLAGS="$CFLAGS -D_DEFAULT_SOURCE -std=c99 -Wall -Werror -Wstrict-prototypes -pipe"
fi

dnl ========================================================================
dnl check for installation of MiniGUI
dnl ========================================================================
PKG_CHECK_MODULES([MINIGUI], [minigui >= 4.0.0])

LIBS="$LIBS $MINIGUI_LIBS -lefence"

dnl ========================================================================
dnl Write Output

AC_CHECK_DECLS(_MGRM_PROCESSES, minigui_runmode="procs", foo=bar, [#include <minigui/common.h>])
AM_CONDITIONAL(MGRM_PROCESSES, test "x$minigui_runmode" = "xprocs")

AC_OUTPUT(
    Makefile
    m4/Makefile
    src/Makefile
    4.0/Makefile
    drm/Makefile
)
