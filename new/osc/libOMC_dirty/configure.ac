# Process this file with autoconf to produce a configure script.
AC_PREREQ(2.57)
AC_INIT([libOMC],[0.1],[piksel])
AM_INIT_AUTOMAKE([AC_PACKAGE_NAME],[AC_PACKAGE_VERSION])
AC_CONFIG_SRCDIR([src/OSC-address-space.c])
AM_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h sys/time.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset])

# Checks for libraries.

# generate config.status
AC_CONFIG_FILES([Makefile
		 src/Makefile])
AC_OUTPUT
