AC_PREREQ(2.57)

#We need a recent ACI
ACI_PREREQ(2003.01.16)

AC_INIT([GRAS],[0.0.040304],[martin.quinson@ens-lyon.fr])
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

# Check architecture signature
GRAS_ARCH

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
ACI_PACKAGE_SAVED(XML,[XML library (version XML2)],xml2-config,,,:)
ACI_PACKAGE([SimGrid],[the SimGrid simulator],[SG_init],[-lsimgrid],[simgrid.h])

dnl A C_CHECK_LIB(pthread, pthread_mutex_lock)
	
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
        src/base/Tests/gs_example
        src/base/Tests/trp_tcp_usage
        src/base/Tests/trp_file_usage
    src/modules/Makefile
    src/examples/Makefile 
      src/examples/ping/Makefile 
      src/examples/bandwidth/Makefile src/examples/saturate/Makefile
      src/examples/alnem/Makefile 
  doc/Makefile
],[( cd src/include ; test -e gras || ln -s . gras )
     test -e src/base/Tests/trp_tcp_usage && chmod +x src/base/Tests/trp_tcp_usage;
     test -e src/base/Tests/trp_file_usage && chmod +x src/base/Tests/trp_file_usage;
     test -e src/base/Tests/gs_example    && chmod +x src/base/Tests/gs_example;
     test -e src/base/Tests/run_tests     && chmod +x src/base/Tests/run_tests])

AC_OUTPUT

echo "

Configuration of package \`${PACKAGE}':

	Compiler:	${CC}

	CFlags:		${CFLAGS}
	LDFlags:	${LDFLAGS}
"

exit 0;
