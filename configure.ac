 AC_PREREQ(2.57)

#We need a recent ACI
ACI_PREREQ(2003.01.16)

AC_INIT([GRAS],[0.0.1b],[martin.quinson@ens-lyon.fr])
AC_CONFIG_SRCDIR([src/include/gras.h])
AC_REVISION($Revision$)
dnl A C_CONFIG_SUBDIRS(src/nws_portability)

AC_CANONICAL_TARGET
AC_LANG([C])

AM_CONFIG_HEADER(src/gras_config.h)
AM_INIT_AUTOMAKE

AM_PROG_LIBTOOL

# declare the modules (no optional module)

dnl
dnl Load anything under acmacro/*.m4
dnl
test -n "$ACLOCAL_FLAGS" && ACLOCAL="$ACLOCAL $ACLOCAL_FLAGS"
ACLOCAL="$ACLOCAL -I acmacro"


AC_PROG_CC
AM_SANITY_CHECK
AC_PROG_MAKE_SET

# Check architecture signature begin
GRAS_ARCH
# Check architecture signature end

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stddef.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_CHECK_FUNCS([memset strchr strerror usleep])

# Can we rebuild the documentation?
GNOME_GTKDOC_CHECK

dnl ####[ Search libs ]#######################################################
dnl A CI_PACKAGE_SAVED(XML,[XML library (version XML2)],xml2-config,,,:)
ACI_PACKAGE([SimGrid],[the SimGrid simulator],[SG_init],[-lsimgrid],[simgrid.h],,:)
AM_CONDITIONAL(HAVE_SG,test x$HAVE_SimGrid = xyes)

dnl A C_CHECK_LIB(pthread, pthread_mutex_lock, LIBS="$LIBS -lpthread")
AC_CHECK_LIB(nsl, gethostbyname, [LIBS="$LIBS -lnsl"])
AC_CHECK_LIB(socket, connect,    [LIBS="$LIBS -lsocket"])
	
dnl ####[ maint mode ]#######################################################
AM_MAINTAINER_MODE
if test x$USE_MAINTAINER_MODE = xyes 
then
   GNOME_COMPILE_WARNINGS(yes)
fi

AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)

dnl ####[ check for some programms ]###########################################
AC_CHECK_PROG(BASH, bash, `which bash`, /bin/sh)
WARNING="This file is generated, do not edit"
AC_SUBST(WARNING)

dnl ####[ Makes the output ]###################################################
AC_CONFIG_FILES([
  Makefile
  src/Makefile
    src/include/Makefile
      src/include/modules/Makefile
    src/base/Makefile
      src/base/Tests/Makefile
      src/base/Tests/run_tests
        src/base/Tests/trp_tcp_usage
        src/base/Tests/trp_file_usage
    src/examples/Makefile 
      src/examples/ping/Makefile 
  doc/Makefile
],[( cd src/include ; test -e gras || ln -s . gras )
     test -e src/base/Tests/trp_tcp_usage && chmod +x src/base/Tests/trp_tcp_usage;
     test -e src/base/Tests/trp_file_usage && chmod +x src/base/Tests/trp_file_usage;
     test -e src/base/Tests/run_tests     && chmod +x src/base/Tests/run_tests;
     chmod +x src/examples/ping/test_rl; chmod +x src/examples/ping/test_sg])

#    src/modules/Makefile
#      src/examples/bandwidth/Makefile src/examples/saturate/Makefile
#      src/examples/alnem/Makefile 

AC_OUTPUT

echo "

Configuration of package \`${PACKAGE}':

	Compiler:	${CC}

	CFlags:		${CFLAGS}
	LDFlags:	${LDFLAGS}
"

exit 0;
