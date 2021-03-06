AC_PREREQ(2.61)
AC_INIT([libtcpbus], m4_esyscmd_s([dpkg-parsechangelog | sed -n 's/^Version: //p']), [])
AM_INIT_AUTOMAKE([foreign dist-bzip2 -Wall])
LT_INIT

PACKAGE_VERSION_MAJOR=`echo "$PACKAGE_VERSION" | sed 's/\(@<:@@<:@:digit:@:>@@:>@\+\)\.\(@<:@@<:@:digit:@:>@@:>@\+\)\.\(@<:@@<:@:digit:@:>@@:>@\+\)/\1/' `
PACKAGE_VERSION_MINOR=`echo "$PACKAGE_VERSION" | sed 's/\(@<:@@<:@:digit:@:>@@:>@\+\)\.\(@<:@@<:@:digit:@:>@@:>@\+\)\.\(@<:@@<:@:digit:@:>@@:>@\+\)/\2/' `
PACKAGE_VERSION_REVISION=`echo "$PACKAGE_VERSION" | sed 's/\(@<:@@<:@:digit:@:>@@:>@\+\)\.\(@<:@@<:@:digit:@:>@@:>@\+\)\.\(@<:@@<:@:digit:@:>@@:>@\+\)/\3/' `
AC_SUBST(PACKAGE_VERSION_MAJOR)
AC_SUBST(PACKAGE_VERSION_MINOR)
AC_SUBST(PACKAGE_VERSION_REVISION)

AC_MSG_CHECKING([whether this is a Git repository])
PACKAGE_GITREVISION=`git describe --tags --always HEAD 2>/dev/null`
which git > /dev/null; \
if test $? -ne 0 || test "x$PACKAGE_GITREVISION" = "x"; then
	PACKAGE_GITREVISION="`cat $srcdir/GITREVISION`-cached"
	AC_MSG_RESULT([no, using stored version info: $PACKAGE_GITREVISION])
else
	echo $PACKAGE_GITREVISION > $srcdir/GITREVISION
	AC_MSG_RESULT([yes, current version info: $PACKAGE_GITREVISION])
fi
AC_SUBST(PACKAGE_GITREVISION)
AC_DEFINE_UNQUOTED([PACKAGE_GITREVISION],["$PACKAGE_GITREVISION"],[Package version with Git revision info])

AC_CONFIG_SRCDIR([src/libtcpbus.c])
AC_CONFIG_HEADERS([config.h])

# Check for options.
####################
AC_ARG_ENABLE([ipv6],
	AC_HELP_STRING([--disable-ipv6],[Disable IPv6 support]),
	[enable_ipv6=$enableval],[enable_ipv6=yes])
AS_IF([test x$enable_ipv6 == xyes], [AC_DEFINE([ENABLE_IPV6],[1],[Define to 1 to enable IPv6 support])] )


# Checks for programs.
######################
AC_PROG_CC
AC_PROG_CPP
AM_PROG_CC_C_O # per product options in Makefile.am

AC_PROG_CXX


# Checks for libraries.
#######################
AC_CHECK_LIB(ev, ev_run, , [AC_MSG_ERROR([Couldn't find libev])]) dnl '


# Checks for header files.
##########################


# Checks for typedefs, structures, and compiler characteristics.
################################################################


# Checks for library functions.
###############################


# Add some info to config.h
AC_DEFINE_UNQUOTED([CONFIGURE_ARGS],["$ac_configure_args"],[Configure flags])
AC_DEFINE_UNQUOTED([CFLAGS],["$CFLAGS"],[Used C flags])
AC_DEFINE_UNQUOTED([CXXFLAGS],["$CXXFLAGS"],[Used C++ flags])
AC_DEFINE_UNQUOTED([CPPFLAGS],["$CPPFLAGS"],[Used C-preprocessor flags])

# Output
########
AC_CONFIG_FILES([
	Makefile
	include/Makefile
	src/Makefile
	Socket/Makefile
	Socket/test/Makefile
	test/Makefile
	${PACKAGE_NAME}.pc
	])
AC_OUTPUT

echo "
--------------------------------------------------------------------------------
${PACKAGE_NAME} version ${PACKAGE_VERSION} (${PACKAGE_GITREVISION})

 Prefix:        '${prefix}'
 C Compiler:    '${CC} ${CFLAGS} ${CPPFLAGS}'
 C++ Compiler:  '${CXX} ${CXXFLAGS} ${CPPFLAGS}'
 Libraries:     '${LIBS}'

 Configured with:
  IPv6: $enable_ipv6
--------------------------------------------------------------------------------
"
