AC_PREREQ(2.59)

#We need a recent ACI
ACI_PREREQ(2003.01.16)

AC_INIT([GRAS],[0.7],[martin.quinson@ens-lyon.fr])
AC_CONFIG_SRCDIR([include/gras.h])
AC_CONFIG_HEADERS([src/gras_config.h])

AC_REVISION($Revision$)
AC_CANONICAL_TARGET
AC_LANG([C])

AM_INIT_AUTOMAKE(gnu)
# MACRO_DIR should tell aclocal to search for my macro. That's the autoconf
# maintainer plan, but automake does not implement this yet (as in 1.8)
AC_CONFIG_MACRO_DIR(acmacro) 
# It seems to be called ACLOCAL_INCLUDE...
# A M_ACLOCAL_INCLUDE(acmacro)




AC_PROG_LIBTOOL

# declare the modules (no optional module)

dnl
dnl Load anything under acmacro/*.m4
dnl
dnl test -n "$ACLOCAL_FLAGS" && ACLOCAL="$ACLOCAL $ACLOCAL_FLAGS"
ACLOCAL="$ACLOCAL -I acmacro"


AC_PROG_CC
AM_SANITY_CHECK
AC_PROG_MAKE_SET

# Check architecture signature begin
GRAS_ARCH
# Check architecture signature end
GRAS_CHECK_STRUCT_COMPACTION

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stddef.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
dnl A C_FUNC_MEMCMP
dnl A C_CHECK_FUNCS([memset strchr strerror usleep])

# Can we rebuild the parsers?
# We really want flex and refuse other lex. So, the parser is portable and
# does not induce extra lib dependency
AC_PROG_LEX
if test "$LEX" != flex; then
  LEX="$SHELL $missing_dir/missing flex"
fi
								  
# Can we rebuild the documentation?
GTK_DOC_CHECK()
AC_SUBST([htmldir],         ['${datadir}/doc/gras/html'])dnl

dnl ####[ Search libs ]#######################################################
ACI_PACKAGE([SimGrid],[the SimGrid simulator],[SG_init],[-lsimgrid],[simgrid.h],,:)
AM_CONDITIONAL(HAVE_SG,test x$HAVE_SimGrid = xyes)

dnl A C_CHECK_LIB(pthread, pthread_mutex_lock, LIBS="$LIBS -lpthread")
AC_CHECK_LIB(nsl, gethostbyname, [LIBS="$LIBS -lnsl"])
AC_CHECK_LIB(socket, connect,    [LIBS="$LIBS -lsocket"])
	
dnl ####[ maint mode ]#######################################################
AM_MAINTAINER_MODE
if test x$USE_MAINTAINER_MODE = xyes 
then
#   enable_iso_c=yes             # Let's go funky
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
  include/Makefile
  src/Makefile
  examples/Makefile 
    examples/ping/Makefile      examples/ping/test_sg
  doc/Makefile
  tools/compile-remote-worker
  testsuite/Makefile
    testsuite/run_tests    
    testsuite/gras/trp_tcp_usage
    testsuite/gras/trp_file_usage
],[
     test -e testsuite/run_tests          && chmod +x testsuite/run_tests;
     test -e testsuite/gras/trp_tcp_usage && chmod +x testsuite/gras/trp_tcp_usage;
     test -e testsuite/gras/trp_file_usage&& chmod +x testsuite/gras/trp_file_usage;
     test -e tools/compile-remote-worker  && chmod +x tools/compile-remote-worker;
     test -e examples/ping/test_sg        && chmod +x examples/ping/test_sg;
     chmod +x $srcdir/tools/gras-check-arch;
     chmod +x $srcdir/examples/ping/test_rl
])

#    src/amok/Makefile

#    examples/bandwidth/Makefile examples/bandwidth/test_sg
#    examples/saturate/Makefile  examples/saturate/test_sg
#    examples/alnem/Makefile     examples/alnem/test_sg

#    examples/pastry/Makefile    examples/pastry/test_sg

AC_OUTPUT

echo "

Configuration of package \`${PACKAGE}' on $gras_arch_name (=$gras_arch):

	Compiler:	${CC}

	CFlags:		${CFLAGS}
	LDFlags:	${LDFLAGS}
"

exit 0;
