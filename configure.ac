
######################
## Setup the autotools
##

AC_PREREQ(2.59)
AC_INIT([simgrid],[3.0.1-cvs],[simgrid-devel@lists.gforge.inria.fr])
AC_CONFIG_SRCDIR([include/gras.h])
AC_CONFIG_HEADERS([src/gras_config.h])
# A CI_PREREQ(2003.01.16) # We need a recent ACI when having sub-modules

AC_REVISION($Revision$)
AC_CANONICAL_TARGET
AC_LANG([C])

AM_INIT_AUTOMAKE(gnu)
AC_CONFIG_MACRO_DIR(acmacro) 
ACLOCAL="$ACLOCAL -I acmacro"
AC_PROG_LIBTOOL

###############
## System checks
##
SG_CONFIGURE_PART(System checks...)
AC_PROG_CC(xlC gcc cc)
AM_SANITY_CHECK
AC_PROG_MAKE_SET
AC_CHECK_PRINTF_NULL
AC_CHECK_VA_COPY

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_TIME
AC_CHECK_HEADERS([sys/socket.h \
                  sys/stat.h \
		  windows.h winsock.h winsock2.h \
		  sys/time.h \
		  errno.h unistd.h \
		  execinfo.h])
AC_CHECK_FUNCS([gettimeofday usleep \
                getdtablesize \
                sysconf\
                readv])
                
# check for a working snprintf (or use xbt/snprintf.c, which comes from http://www.ijs.si/software/snprintf/)
AC_FUNC_SNPRINTF
# check for asprintf function familly (or request the replacements from xbt/snprintf.c)
AC_CHECK_FUNC(  asprintf, :,   AC_DEFINE(NEED_ASPRINTF,  1,  enable the asprintf   replacement))
AC_CHECK_FUNC( vasprintf, :,   AC_DEFINE(NEED_VASPRINTF, 1,  enable the vasprintf  replacement))
# we don't use those:
# A C_CHECK_FUNC( asnprintf, :,   A C_DEFINE(NEED_ASNPRINTF,  1, enable the asnprintf  replacement))
# A C_CHECK_FUNC(vasnprintf, :,   A C_DEFINE(NEED_VASNPRINTF, 1, enable the vansprintf replacement))


# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

###################################
## SimGrid and GRAS specific checks
##

SG_CONFIGURE_PART(Checking GRAS architecture signature...)
# Check architecture signature begin
GRAS_ARCH
# Check architecture signature end
GRAS_CHECK_STRUCT_COMPACTION


dnl ##
dnl ##  CONTEXT IMPLEMENTATION
dnl ##

SG_CONFIGURE_PART([Checking for threads, contexts or assimilated...])

dnl #
dnl #  1. determine possibilities
dnl #

dnl #  check for MCSC method
AC_MSG_CHECKING(on top of what can we build the contexts)
AC_CHECK_HEADER(ucontext.h,,, [#include <sys/types.h>])
AC_CHECK_FUNCS(makecontext swapcontext getcontext setcontext)
AC_CHECK_MCSC(mcsc=yes, mcsc=no)

dnl #  check for pthread method
AC_CHECK_HEADERS([pthread.h])
AC_CHECK_LIB(pthread,pthread_create,pthread=yes, pthread=no)

dnl #
dnl #  2. make a general decision
dnl #

if test ".$mcsc" = .yes; then
   mcsc=yes
elif test ".$pthread" = .yes; then
   pthread=yes
else
    ac_header=windows.h
    as_ac_Header=`echo "ac_cv_header_$ac_header" | $as_tr_sh`
    if test `eval echo '${'$as_ac_Header'}'` = yes; then
      windows_context=yes
    else
      AC_ERROR([no appropriate backend found])
    fi
fi

dnl #
dnl #  3. allow decision to be overridden by user
dnl #

AC_MSG_CHECKING(what kind of backend should we use)
AC_ARG_WITH(context,
  [  --with-context=[ucontext/pthread]   Use either (System V) swapcontext or pthread [[default=auto]].],,
  with_context=auto)
case $with_context in
 ucontext) ;;
 pthread) ;;
 windows) ;;
 auto) with_context=ucontext;;
 *) AC_MSG_ERROR("--with-context must be either ucontext or pthread") ;;
esac

if test "x$with_context" = "xucontext" ; then
  if test ".$mcsc" = .yes; then
    AC_MSG_RESULT(found working ucontext. Great!)
    AC_DEFINE([USE_UCONTEXT],1,[Define if we use ucontext or not])
  else
    if test ".$windows_context" = .yes ; then
      AC_MSG_RESULT(use windows context portability layer.)
      with_context=windows
    else
      AC_MSG_RESULT([[no working ucontext found. Try pthreads as a fallback]])
      with_context=pthread
    fi
  fi
fi

if test "x$with_context" = "xpthread"; then
  AC_CHECK_HEADERS([pthread.h])
  AC_CHECK_LIB(pthread,pthread_create,,
    [AC_MSG_ERROR([[Cannot find pthreads, no way (try --with-context=ucontext if you haven't already tried).]])])
  AC_DEFINE([USE_PTHREADS],1,[Define if we use pthreads or not])
  AC_MSG_RESULT(You have pthreads. Let's use them.)
fi

dnl #
dnl #  4. determine a few additional details
dnl #

if test "x$with_context" = "xucontext" ; then
dnl #  direction of stack grow
  AC_CHECK_STACKGROWTH(PTH_STACKGROWTH)
  if test ".$ac_cv_check_stackgrowth" = ".down"; then
      PTH_STACK_GROWTH="down"
  else
      PTH_STACK_GROWTH="up"
  fi
  AC_SUBST(PTH_STACK_GROWTH)
  
  AC_CHECK_STACKSETUP(makecontext, pth_skaddr_makecontext, pth_sksize_makecontext)
fi

#########################################
## Check for libraries extra-dependencies
##

SG_CONFIGURE_PART(Checking extra libraries dependencies...)
SIMGRID_DEP=""
AC_SUBST([SIMGRID_DEP])

GRAS_DEP=""
AC_CHECK_LIB(nsl, gethostbyname, [GRAS_DEP="$GRAS_DEP -lnsl"])
AC_CHECK_LIB(socket, connect,    [GRAS_DEP="$GRAS_DEP -lsocket"])

AC_MSG_CHECKING(for extra dependencies of libgras)
case $host_os in
  *mingw* ) GRAS_DEP="$GRAS_DEP -lws2_32" ; SIMGRID_DEP="$SIMGRID_DEP -lws2_32" ;;
esac
	   
if test "x$GRAS_DEP" = x; then 
   AC_MSG_RESULT(none)
else
   AC_MSG_RESULT($GRAS_DEP)
fi
AC_SUBST([GRAS_DEP])

##############################################
## Enhance maintainer mode and SUBST variables
## (must be placed after any compilation tests since our overprotective flags
##  let some tests fail)

AM_MAINTAINER_MODE
if test x$USE_MAINTAINER_MODE = xyes 
then
   # Maintainers have no choice ! I'm a BOFH, and I plainly assume. [Mt]
   enable_compile_warnings=yes
fi
SG_COMPILE_FLAGS

AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)

#####################
## Check for programs
##

SG_CONFIGURE_PART(Check for programs...)
AC_CHECK_PROG(BASH, bash, `which bash`, /bin/sh)
WARNING="This file is generated, do not edit"
AC_SUBST(WARNING)

# Can we rebuild the parsers?
# We really want flex and refuse other lex. So, the parser is portable and
# does not induce extra lib dependency
AC_PROG_FLEX(2.5.30)
		
# Can we rebuild the xml-lexers from the XML specification?
# if not, simply touch the flex source files (which are distributed in
#  tarballs even if generated by flexml) and hope for the best.
AC_CHECK_PROG(FLEXML,flexml,`which flexml`,NOTFOUND)
AM_CONDITIONAL(HAVE_FLEXML,test x$FLEXML != xNOTFOUND)

###################
## Makes the output
##


#    examples/pastry/Makefile    examples/pastry/test_sg
#      examples/gras/chord/Makefile  examples/gras/chord/test_sg examples/gras/chord/test_rl
#  src/amok/Makefile

SG_CONFIGURE_PART(Generating files...)
AC_CONFIG_FILES([
  Makefile
  include/Makefile
  src/Makefile
  src/ucontext_stack.h
  examples/Makefile 
    examples/msg/Makefile           examples/msg/run_msg_test
    examples/gras/Makefile 
      examples/gras/ping/Makefile   examples/gras/ping/test_sg  examples/gras/ping/test_rl
      examples/gras/timer/Makefile  examples/gras/timer/test_sg examples/gras/timer/test_rl
      examples/gras/chrono/Makefile examples/gras/chrono/test_sg examples/gras/chrono/test_rl
      examples/gras/tokenS/Makefile examples/gras/tokenS/test_sg examples/gras/tokenS/test_rl
      examples/gras/mmrpc/Makefile examples/gras/mmrpc/test_sg examples/gras/mmrpc/test_rl
    examples/amok/Makefile       
      examples/amok/bandwidth/Makefile examples/amok/bandwidth/test_sg examples/amok/bandwidth/test_rl
  doc/Makefile
    doc/Doxyfile
  tools/graspe-slave            tools/Makefile
    tools/gras/Makefile
  testsuite/Makefile
  testsuite/run_tests    
  testsuite/gras/trp_tcp_usage  testsuite/gras/trp_file_usage
],[
    for file in                                                 \
     testsuite/run_tests                                        \
     testsuite/gras/trp_tcp_usage testsuite/gras/trp_file_usage \
     tools/graspe-slave           tools/graspe-master           \
     \
     examples/amok/bandwidth/test_sg examples/amok/bandwidth/test_rl \
     \
     examples/gras/ping/test_sg   examples/gras/ping/test_rl    \
     examples/gras/timer/test_sg  examples/gras/timer/test_rl   \
     examples/gras/chrono/test_sg examples/gras/chrono/test_rl  \
     examples/gras/tokenS/test_sg examples/gras/tokenS/test_rl  \
     examples/gras/mmrpc/test_sg examples/gras/mmrpc/test_rl  \
     examples/msg/run_msg_test                                  \
    ; do                                                        \
      test -e $file && chmod +x $file;                          \
    done
])


#     examples/gras/chord/test_sg  examples/gras/chord/test_rl   \
#    examples/gras/pastry/test_sg   



#    examples/gras/saturate/Makefile  examples/gras/saturate/test_sg
#    examples/gras/alnem/Makefile     examples/gras/alnem/test_sg



AC_OUTPUT

echo "

Configuration of package \`${PACKAGE}' (version ${VERSION}) on $gras_arch_name (=$gras_arch):

	Compiler:	 ${CC} (version: ${GCC_VERSION})
	
	CFlags:		 ${CFLAGS}
	LDFlags:	 ${LDFLAGS}

	Context backend: ${with_context}
"

exit 0;
