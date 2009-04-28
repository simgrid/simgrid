dnl Copyright (C) 2004-2007. The SimGrid team. All rights reserved.

dnl This file is part of the SimGrid project. This is free software:
dnl You can redistribute and/or modify it under the terms of the
dnl GNU LGPL (v2.1) licence.

######################
## Setup the autotools
##

AC_PREREQ(2.59)
AC_INIT([simgrid],[3.3],[simgrid-devel@lists.gforge.inria.fr])
AC_CONFIG_SRCDIR([include/gras.h])
AC_CONFIG_HEADERS([src/gras_config.h])
# A CI_PREREQ(2003.01.16) # We need a recent ACI when having sub-modules

AC_REVISION($Revision$)
AC_CANONICAL_TARGET
AC_LANG([C])
AC_PROG_CXX
AM_PROG_GCJ

AM_INIT_AUTOMAKE([gnu -Wno-portability])
AC_CONFIG_MACRO_DIR(acmacro) 
ACLOCAL="$ACLOCAL -I acmacro"

dnl Next few lines is a hack to prevent libtool checking for F77
m4_undefine([AC_PROG_F77])
m4_defun([AC_PROG_F77],[])

dnl We do build a proper DLL when using win32
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

###############
## SVN version check
##
if test -e .svn ; then
  VERSION="$VERSION-r`svnversion`"
  PACKAGE_VERSION="$PACKAGE_VERSION-r`svnversion`"
  PACKAGE_STRING="PACKAGE_STRING r`svnversion`"  
fi

###############
## System checks
##
SG_CONFIGURE_PART([System checks...])
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
		  execinfo.h\
		  signal.h ])
AC_CHECK_FUNCS([gettimeofday usleep \
                getdtablesize \
                sysconf\
                readv\
		popen\
		signal\
		getline])
                
# check for a working snprintf (or use xbt/snprintf.c, which comes from http://www.ijs.si/software/snprintf/)
AC_FUNC_SNPRINTF
# check for asprintf function familly (or request the replacements from xbt/snprintf.c)
AC_CHECK_FUNC(  asprintf, :,   AC_DEFINE(NEED_ASPRINTF,  1,  enable the asprintf   replacement))
AC_CHECK_FUNC( vasprintf, :,   AC_DEFINE(NEED_VASPRINTF, 1,  enable the vasprintf  replacement))

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

SG_CONFIGURE_PART([Checking threads, contexts or assimilated...])

dnl #
dnl #  1. determine possibilities
dnl #

dnl #  check for MCSC method (ucontexts)
AC_CHECK_MCSC(mcsc=yes, mcsc=no) 

dnl #  check for pthread method
AC_CHECK_HEADERS([pthread.h])
AC_CHECK_LIB(pthread,pthread_create,pthread=yes, pthread=no)

if test x$pthread = xyes ; then
  AC_CHECK_LIB(pthread, sem_init, HAVE_SEM_INIT=yes, HAVE_SEM_INIT=no)
  if test ${HAVE_SEM_INIT} = yes ; then
     AC_MSG_CHECKING(if sem_init is compilable)
     AC_TRY_COMPILE([#include <semaphore.h>],
	            [sem_t s; sem_init(&s);], HAVE_SEM_INIT=yes, HAVE_SEM_INIT=no)
     if test ${HAVE_SEM_INIT} = yes ; then
        AC_DEFINE([HAVE_SEM_INIT],1,[Define if sem_init() is avaible or not (part of XPG6 standard only)])
        AC_MSG_RESULT(yes)
     else
	AC_MSG_RESULT(no)
     fi
  fi
  AC_CHECK_LIB(pthread, sem_timedwait, HAVE_SEM_TIMEDWAIT=yes, HAVE_SEM_TIMEDWAIT=no)
  if test ${HAVE_SEM_TIMEDWAIT} = yes ; then
     AC_MSG_CHECKING(if sem_timedwait is compilable)
     AC_TRY_COMPILE([#include <semaphore.h>],
	            [sem_t *s; const struct timespec * t; sem_timedwait(s, t);], HAVE_SEM_TIMEDWAIT=yes, HAVE_SEM_TIMEDWAIT=no)
     if test ${HAVE_SEM_TIMEDWAIT} = yes ; then
        AC_DEFINE([HAVE_SEM_TIMEDWAIT],1,[Define if sem_timedwait() is avaible or not (part of XPG6 standard only)])
        AC_MSG_RESULT(yes)
     else
	AC_MSG_RESULT(no)
     fi
  fi

  AC_CHECK_LIB(pthread, pthread_mutex_timedlock, HAVE_MUTEX_TIMEDLOCK=yes, HAVE_MUTEX_TIMEDLOCK=no)
  if test ${HAVE_MUTEX_TIMEDLOCK} = yes ; then
     AC_MSG_CHECKING(if pthread_mutex_timedlock is compilable)
     AC_TRY_COMPILE([#include <pthread.h>],
	            [pthread_mutex_t s; const struct timespec t; sem_timedlock(&s, &t);], HAVE_MUTEX_TIMEDLOCK=yes, HAVE_MUTEX_TIMEDLOCK=no)
     if test ${HAVE_MUTEX_TIMEDLOCK} = yes ; then
        AC_DEFINE([HAVE_MUTEX_TIMEDLOCK],1,[Define if pthread_mutex_timedlock() is avaible or not (part of XPG6 standard only?)])
        AC_MSG_RESULT(yes)
     else
	AC_MSG_RESULT(no)
     fi
  fi
fi

dnl #
dnl #  2. make a general decision
dnl #

if test ".$mcsc" = .yes; then
   # ucontext found
   mcsc=yes
elif test ".$pthread" = .yes; then
   # ucontext not found, but pthread found
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
AM_CONDITIONAL(IS_WINDOWS,test x$windows_context = xyes)


dnl #
dnl #  3. allow decision to be overridden by user
dnl #

AC_MSG_CHECKING(what kind of backend should we use)

AC_ARG_WITH(pthread, [  --with-pthread   Use pthread instead of ucontext.],with_context=pthread)
AC_ARG_WITH(pthread, [  --with-pthreads   Use pthread instead of ucontext.],with_context=pthread)

AC_ARG_WITH(context,
  [  --with-context=[ucontext/pthread]   Use either (System V) swapcontext or pthread [[default=auto]].],,
  with_context=auto)

if test "x$with_context$mcsc" = "xucontextno" ; then 
  AC_MSG_ERROR([--with-context=ucontext specified but ucontext unusable. Relaunch configure without this argument.], 77)
fi

case $with_context in
 ucontext) ;;
 pthread) ;;
 pthreads) ;;
 windows) ;;
 auto) with_context=ucontext;;
 *) AC_MSG_ERROR([--with-context must be either ucontext or pthread]) ;;
esac

if test "x$with_context" = "xucontext" ; then
  if test ".$mcsc" = .yes; then
    AC_MSG_RESULT(found working ucontext. Great!)
    AC_DEFINE([CONTEXT_UCONTEXT],1,[Define if xbt contexts are based on ucontext or not])
    AC_CHECK_HEADERS([valgrind/valgrind.h])
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
  if test x$pthread != xyes ; then # reuse previous checks
    AC_MSG_ERROR([[Cannot find pthreads (try --with-context=ucontext if you haven't already tried).]])
  fi
  AC_DEFINE([CONTEXT_THREADS],1,[Define if xbt contexts are based on our threads implementation or not])
  AC_MSG_RESULT(You have pthreads and requested for them. Fine.)
fi
AM_CONDITIONAL(CONTEXT_THREADS,test "x$with_context" != xucontext)

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
SMPI_DEP=""
GRAS_DEP=""

if test xpthread=xyes ; then 
  # if the pthreads are usable
  if test "x$with_context" = "xpthread" ; then
    # if we use them to implement the xbt_context
    SIMGRID_DEP="-lpthread"
  fi
  # we need them in any case for the gras lib (which is multithreaded), but on windows (of course)
  if test "x$with_context" != "xwindows" ; then
    GRAS_DEP="-lpthread"
  fi
fi

#########################################
## Build optional modules (gtnets)
##
gtnets=no
AC_ARG_WITH(gtnets,
  AS_HELP_STRING([--with-gtnets], [Path to GTNetS installation (default to empty, ie not using GTNetS)]),
  gtnets_path="$withval",gtnets_path="no")
if test "x$gtnets_path" = "xno" ; then
  AC_MSG_RESULT(Eventually you will come to GTNetS.)
else  
  AC_MSG_RESULT(***** You have decided to use the experimental GTNetS. We hope you know what you're doing.. *****)
  AC_MSG_CHECKING(for gtnets)
  AC_LANG_PUSH([C++])  
  GTNETS_LDFLAGS="-lgtnets -L$gtnets_path/lib"
  GTNETS_CPPFLAGS="-I$gtnets_path/include -I$gtnets_path/include/gtnets"
  LDFLAGS_SAV=$LDFLAGS	
  CPPFLAGS_SAV=$CPPFLAGS
  CPPFLAGS+=$GTNETS_CPPFLAGS
  LDFLAGS+=$GTNETS_LDFLAGS
  AC_TRY_LINK([ #include <simulator.h>
              ], [Simulator s; s.RunUntilNextCompletion();], gtnets=yes, gtnets=no)
  CPPFLAGS=$CPPFLAGS_SAV
  LDFLAGS=$LDFLAGS_SAV
  AC_LANG_POP([C++])  
fi

if test "x$gtnets" = "xyes" ; then
  AC_MSG_RESULT(Enabling GTNETS support.)
  AC_DEFINE([HAVE_GTNETS], 1, [Indicates that we have GTNETS support])
  LDFLAGS+="-lgtnets -L$gtnets_path/lib"
  CPPFLAGS+="-I$gtnets_path/include -I$gtnets_path/include/gtnets"
fi
AM_CONDITIONAL(HAVE_GTNETS, test "x$gtnets" != "xno")




#########################################
## Build optional modules (csdp)
##
csdp=no
AC_ARG_WITH(csdp,
  AS_HELP_STRING([--with-csdp], [Path to csdp installation (default to empty, i.e. not using csdp)]),
  csdp_path="$withval",csdp_path="no")
  AC_MSG_CHECKING(for CSDP option:)
if test "x$csdp_path" = "xno" ; then
  AC_MSG_RESULT([no, eventually you will come to csdp.])
else  
  AC_MSG_RESULT(***** You have decided to use csdp. Let's check whether it works or not *****)
  AC_MSG_CHECKING(for csdp)
  CSDP_LDFLAGS="-L$csdp_path/lib -lsdp -llapack -lblas -lm"
  CSDP_CPPFLAGS="-I$csdp_path/include -I$csdp_path/include/csdp"

  AC_CHECK_LIB(sdp,easy_sdp, csdp_lib=yes, csdp_lib=no,$CSDP_LDFLAGS)
  
  CPPFLAGS_SAV=$CPPFLAGS
  CPPFLAGS+=$CSDP_CPPFLAGS
  AC_CHECK_HEADER(declarations.h, csdp_header=yes, csdp_header=no, [])
  CPPFLAGS=$CPPFLAGS_SAV

  

  if test "x$csdp_lib = xyes" -a "x$csdp_header = xyes" ; then
     csdp=yes;
  else 
     csdp=no;
  fi
  if test "x$csdp" = xyes; then 
    AM_CPPFLAGS="$AM_CPPFLAGS $CSDP_CPPFLAGS"
    SIMGRID_DEP="$SIMGRID_DEP $CSDP_LDFLAGS"
    AC_MSG_RESULT(Found working sdp library.)
    AC_DEFINE([HAVE_SDP], 1, [Indicates whether we have the CSDP library or not])
  else
    AC_MSG_RESULT(Could not find any working sdp library.)
  fi;
fi
AM_CONDITIONAL(HAVE_SDP,test x$csdp != xno)

AC_SUBST([SIMGRID_DEP])
AC_SUBST([SMPI_DEP])

AC_CHECK_LIB(nsl, gethostbyname, [GRAS_DEP="$GRAS_DEP -lnsl"])
AC_CHECK_LIB(socket, connect,    [GRAS_DEP="$GRAS_DEP -lsocket"])

AC_MSG_CHECKING(for extra dependencies of libgras)
case $host_os in
  *mingw* ) GRAS_DEP="$GRAS_DEP -lws2_32" ; SIMGRID_DEP="$SIMGRID_DEP -lws2_32";
            AM_CPPFLAGS="$AM_CPPFLAGS -DDLL_EXPORT"
            AC_DEFINE(CONTEXT_THREADS,1);;
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

# Check whether we are doing a regular build or a GRAMINE (minimal) one
# Next line is modified by sed when building gramine source tree
gramine_mode=no
if test x$gramine_mode = xyes
then
  USE_MAINTAINER_MODE=no
fi
AM_CONDITIONAL(GRAMINE_MODE,test x$gramine_mode != xno)


AM_MAINTAINER_MODE
if test x$USE_MAINTAINER_MODE = xyes 
then
   # Maintainers have no choice ! I'm a BOFH, and I plainly assume. [Mt]
   enable_compile_warnings=yes   
fi
SG_COMPILE_FLAGS

AC_SUBST([CFLAGS])
AC_SUBST([CPPFLAGS])
AC_SUBST([LDFLAGS])
AC_SUBST([AM_CPPFLAGS])
AC_SUBST([abs_builddir])
AC_SUBST([abs_srcdir])
AC_SUBST([abs_top_builddir])
AC_SUBST([abs_top_srcdir])

##############################################
## Specific mode for build daemons.
## (they compile the SVN without having the autotools installed)
AC_ARG_ENABLE(botbuild,
	      AS_HELP_STRING([--enable-botbuild], [Compilation mode for build daemons -- do not use it]),
              botbuild=$enableval,botbuild=no)
AM_CONDITIONAL(BOTBUILD_MODE,test x$botbuild != xno)

if test -e .svn && test x$USE_MAINTAINER_MODE != xyes && test x$botbuild != xyes ; then
  echo "ERROR: "
  echo "ERROR: You have to enable the maintainer mode to compile the SVN."
  echo "ERROR: For this, just call configure this way:"
  echo "ERROR:"
  echo "ERROR: ./configure '--enable-maintainer-mode' $ac_configure_args"
  echo "ERROR:"
  exit 1
fi


SG_CONFIGURE_PART(Checking Java bindings...)
# Java cruft
AC_ARG_ENABLE(java,
	      AS_HELP_STRING([--disable-java], [To not compile the Java bindings even if the tools are found]),
	      disable_java=$enableval,disable_java=yes)
if test "x$disable_java" != "xyes" ; then
   use_java="disabled by user"
else
  AC_PATH_PROG([JAVAC], [javac], `which javac`)
  AC_PATH_PROG([JAVA],  [java] , `which java`)
  AC_PATH_PROG([JAR],   [jar]  , `which jar`)
  AC_CHECK_HEADERS(jni.h)
  if test -n "$JAVAC" && test -n "$JAVA" && test -n "$JAR" && test "x$ac_cv_header_jni_h" = "xyes" ; then
     use_java="yes"
     AC_DEFINE(HAVE_JAVA,1, [defines whether Java bindings must be compiled or not])
  else   
     use_java="no"
  fi
fi
AC_MSG_RESULT(decide whether to compile java bindings... $use_java)
AM_CONDITIONAL(HAVE_JAVA,test "x$use_java" = "xyes")


#####################
## Check for programs
##

SG_CONFIGURE_PART(Checking programs...)
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

if test x$USE_MAINTAINER_MODE = xyes 
then
   # Check for doxygen when in maintainer mode since dist* targets fail
   # without it
   AC_PATH_PROG(DOXYGEN,doxygen,`which doxygen`)
   if test x$DOXYGEN = x ; then 
     AC_MSG_ERROR([doxygen is mandatory in maintainer mode])
   fi
fi

# Can we extract really usable backtraces? (also need the popen function)
AC_PATH_PROG(ADDR2LINE, addr2line)
if test x$ADDR2LINE != x ; then
  AC_DEFINE_UNQUOTED(ADDR2LINE,"$ADDR2LINE",[Path to the addr2line tool])
fi

AC_SUBST(ac_configure_args)
AC_DEFINE_UNQUOTED(AC_CONFIGURE_ARGS,"$ac_configure_args",[Arguments passed to the configure script])
# Can we make status line about the compilation result?
dnl A C_CHECK_PROG(MD5SUM,md5sum,`which md5sum`)
build_id="$PACKAGE ver=$VERSION args=\"$ac_configure_args\""
AC_SUBST(build_id)

###################
## Makes the output
##


#
#      examples/gras/chord/Makefile  examples/gras/chord/test_sg examples/gras/chord/test_rl
#  src/amok/Makefile

SG_CONFIGURE_PART(Generating files...)

# Core of the libraries

AC_CONFIG_FILES([
  Makefile
  include/Makefile
  src/Makefile
  src/context_sysv_config.h
])

# Tools being embeeded in gramine (stub generator)
AC_CONFIG_FILES([
  tools/Makefile
    tools/gras/Makefile
    tools/tesh/Makefile
])
#    tools/tesh2/Makefile


# GRAMINE_CUT_BEGIN

# Tools NOT being embeeded in gramine
AC_CONFIG_FILES([
  tools/graspe-slave
],[
    for file in                                                 \
     tools/graspe-slave           tools/graspe-master           \
     checkall                     examples/java/runtest         \
    ; do                                                        \
      test -e $file && chmod +x $file;                          \
    done
])

# Testsuite

AC_CONFIG_FILES([  
  testsuite/Makefile
  testsuite/run_tests    
],[
    for file in                                                 \
     testsuite/run_tests                                        \
    ; do                                                        \
      test -e $file && chmod +x $file;                          \
    done
])

# Teshsuite (testsuite using tesh)
AC_CONFIG_FILES([  
  teshsuite/Makefile
])

# Documentation
AC_CONFIG_FILES([
  doc/Makefile
  doc/Doxyfile
])

# Examples

#      examples/gras/p2p/Makefile
#      examples/gras/p2p/chord/Makefile
#      examples/gras/p2p/can/Makefile

AC_CONFIG_FILES([
    examples/java/Makefile
      examples/java/basic/Makefile
      examples/java/comm_time/Makefile
      examples/java/suspend/Makefile
      examples/java/ping_pong/Makefile
])

AC_CONFIG_FILES([
  examples/Makefile 
    examples/msg/Makefile
    examples/simdag/Makefile
    examples/gras/Makefile
      examples/gras/ping/Makefile   
      examples/gras/rpc/Makefile
      examples/gras/spawn/Makefile
      examples/gras/synchro/Makefile
      examples/gras/timer/Makefile
      examples/gras/properties/Makefile
      examples/gras/chrono/Makefile
      examples/gras/mutual_exclusion/simple_token/Makefile
      examples/gras/mmrpc/Makefile
      examples/gras/pmm/Makefile
      examples/gras/all2all/Makefile
    examples/amok/Makefile       
])

AC_CONFIG_FILES([
  src/smpi/smpicc
  src/smpi/smpirun
])

# GRAMINE_CUT_END

AC_OUTPUT

echo "

Configuration of package \`${PACKAGE}' (version ${VERSION}) on $gras_arch_name (=$gras_arch):

	Compiler:	 ${CC} (version: ${GCC_VERSION})
	
	CFlags:		 ${CFLAGS}
	CPPFlags:	 ${CPPFLAGS}
	LDFlags:	 ${LDFLAGS}

	Context backend: ${with_context}
	Compile Java:    ${use_java}
	
	Maintainer mode: ${USE_MAINTAINER_MODE}

"
if test x$botbuild = xyes ; then
  echo "        This is a bot build. Do not specify --enable-botbuild if you are not a bot."
  echo
fi

if test -e .svn && test x$USE_MAINTAINER_MODE != xyes && test x$botbuild = xyes ; then
  echo "WARNING: "
  echo "WARNING: You are compiling the SVN in botbuild mode. If you are not a daemon, don't do so. "
  echo "WARNING: Remove the --enable-botbuild from your configure line, and add --enable-maintainer-mode instead."
  echo "WARNING:"
  echo "WARNING: You won't be able to build archives until then (make dist is likely to fail). "
  echo "WARNING:"
fi

echo $build_id > stamp.configure

exit 0;
