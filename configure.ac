dnl Copyright 2015 Endless Mobile, Inc.

AC_INIT([eos-composite-mode], [0],
    [], [eos-composite-mode], [http://endlessm.com])
AM_INIT_AUTOMAKE([-Wall -Wno-portability foreign 1.11 tar-ustar no-dist-gzip dist-xz subdir-objects])
AM_SILENT_RULES([yes])

PKG_PROG_PKG_CONFIG
GLIB_GSETTINGS

AC_PROG_CC
AC_CACHE_SAVE

PKG_CHECK_MODULES([EOS_COMPOSITE_MODE], [gtk+-3.0 x11 xrandr])

AC_CONFIG_FILES([
    Makefile
])
AC_OUTPUT
