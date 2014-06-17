dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.61)
AC_INIT([sparometer], 1, chlor@schou.dk)
AM_INIT_AUTOMAKE(sparometer, 0.0.4)
AM_CONFIG_HEADER(config.h)
AM_SYS_POSIX_TERMIOS
CFLAGS="-Wall -pedantic -O2"
AC_PROG_CC
AC_STDC_HEADERS
AC_C_CONST

AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_FUNC_MALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STRFTIME
AC_HEADER_TIME
AC_PROG_GCC_TRADITIONAL
AC_STRUCT_TM

AC_CHECK_HEADERS([stdio.h])
AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([ctype.h])
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([sys/ioctl.h])
AC_CHECK_HEADERS([termios.h])
AC_CHECK_HEADERS([errno.h])
AC_CHECK_HEADERS([string.h])
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_HEADERS([time.h])
AC_CHECK_HEADERS([sys/select.h])
AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_HEADERS([sys/types.h])
AC_CHECK_HEADERS([getopt.h])

AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([select])
AC_CHECK_FUNCS([gettimeofday])
AC_CHECK_FUNCS([strerror])


ISODATE=`date -I`
AC_DEFINE_UNQUOTED([ISODATE], "$ISODATE", [Date when configure was done])

YEAR=`date +%Y`
AC_DEFINE_UNQUOTED([YEAR], "$YEAR", [Year when configure was done])

AC_ARG_ENABLE(
	[debug],
	AS_HELP_STRING(
		[--enable-debug],
		[Enable early debug (developer)]
		),
	AC_DEFINE(
		[DEBUG],
		[1],
		[Yes debug]
		),
	AC_DEFINE(
		[DEBUG],
		[0],
		[No debug]
		)
)

SERIAL_DEFAULT=/dev/ttyS0
AC_ARG_ENABLE(
	[serial_device],
	AS_HELP_STRING(
		[--enable-serial-device=/dev/ttyS0],
		[Default serial device when none specified on command line]
		),
	AC_DEFINE_UNQUOTED(
		[SDEVICE], 
		["$enableval"], 
		[Default serial device B]
		),
	AC_DEFINE_UNQUOTED(
		[SDEVICE], 
		"$SERIAL_DEFAULT", 
		[Default serial device C]
		)
)

AC_OUTPUT(Makefile)
