AC_PREREQ(2.59)

#We need a recent ACI
ACI_PREREQ(2003.01.16)

AC_INIT([GRAS],[0.2],[martin.quinson@ens-lyon.fr])
AC_CONFIG_SRCDIR([src/include/gras.h])
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
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_CHECK_FUNCS([memset strchr strerror usleep])

# Can we rebuild the parsers?
# We really want flex and refuse other lex. So, the parser is portable and
# does not induce extra lib dependency
AC_PROG_LEX
if test "$LEX" != flex; then
  LEX="$SHELL $missing_dir/missing flex"
fi
								  
# Can we rebuild the documentation?
GTK_DOC_CHECK()

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
    src/base/Makefile
      src/base/Tests/Makefile
      src/base/Tests/run_tests
        src/base/Tests/trp_tcp_usage
        src/base/Tests/trp_file_usage
    src/examples/Makefile 
      src/examples/ping/Makefile 
  doc/Makefile
  tools/compile-remote-worker
],[
     test -e src/base/Tests/trp_tcp_usage && chmod +x src/base/Tests/trp_tcp_usage;
     test -e src/base/Tests/trp_file_usage && chmod +x src/base/Tests/trp_file_usage;
     test -e src/base/Tests/run_tests     && chmod +x src/base/Tests/run_tests;
     test -e tools/compile-remote-worker  && chmod +x tools/compile-remote-worker;
     chmod +x tools/gras-check-arch;
     chmod +x src/examples/ping/test_rl; chmod +x src/examples/ping/test_sg])

#      src/examples/pastry/Makefile
#    src/modules/Makefile
#      src/examples/bandwidth/Makefile src/examples/saturate/Makefile
#      src/examples/alnem/Makefile 

AC_OUTPUT

echo "

Configuration of package \`${PACKAGE}' on $gras_arch_name (=$gras_arch):

	Compiler:	${CC}

	CFlags:		${CFLAGS}
	LDFlags:	${LDFLAGS}
"

exit 0;
