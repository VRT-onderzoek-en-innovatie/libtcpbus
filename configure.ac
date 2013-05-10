AC_PREREQ(2.61)
AC_INIT([libtcpbus], [0.0.0], [])
AM_INIT_AUTOMAKE([dist-bzip2 -Wall])
LT_INIT

AC_MSG_CHECKING([whether this is a Git repository])
PACKAGE_GITREVISION=`git describe --tags HEAD 2>/dev/null`
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

# Checks for programs.
######################
AC_PROG_CC
AC_PROG_CPP
AM_PROG_CC_C_O # per product options in Makefile.am

# Checks for libraries.
#######################
AC_CHECK_LIB(ev, ev_run, , [AC_MSG_ERROR([Couldn't find libev])]) dnl '
AC_CHECK_LIB(log, LogAtLevel, , [AC_MSG_ERROR([Couldn't find liblog])]) dnl '

# Checks for header files.
##########################
AC_CHECK_HEADERS([liblog.h])


# Checks for typedefs, structures, and compiler characteristics.
################################################################


# Checks for library functions.
###############################



# Output
########
AC_CONFIG_FILES([
	Makefile
	include/Makefile
	src/Makefile
	test/Makefile
	])
AC_OUTPUT

echo "
--------------------------------------------------------------------------------
${PACKAGE_NAME} version ${PACKAGE_VERSION} (${PACKAGE_GITREVISION})

 Prefix:       '${prefix}'
 C Compiler:   '${CC} ${CFLAGS} ${CPPFLAGS}'
 Libraries:    '${LIBS}'
--------------------------------------------------------------------------------
"
