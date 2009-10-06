dnl SG_COMPILE_FLAGS
dnl Turn on many useful compiler warnings
dnl For now, only sets extra flags on GCC


dnl Copyright (C) 2004, 2005, 2007. Martin Quinson. All rights reserved.

dnl This file is part of the SimGrid project. This is free software:
dnl You can redistribute and/or modify it under the terms of the
dnl GNU LGPL (v2.1) licence.


AC_DEFUN([SG_COMPILE_FLAGS],[
  AC_ARG_ENABLE(compile-warnings,
    AS_HELP_STRING([--enable-compile-warnings], [use compiler warnings (default=no, unless in maintainer mode)]),
    enable_compile_warnings=$enableval,enable_compile_warnings=no)

  AC_ARG_ENABLE(compile-optimizations,
    AS_HELP_STRING([--disable-compile-optimizations], [use compiler optimizations (default=yes, unless if CFLAGS is explicitly set)]),
    enable_compile_optimizations=$enableval,enable_compile_optimizations=auto)

    # AC PROG CC tests whether -g is accepted. 
    # Cool, but it also tries to set -O2 and -g. 
    # I don't want it with gcc, but -O3 and -g2, and optimization only when asked by user
    case $CC in 
      *gcc)
      if test "$CFLAGS" = "-g -O2" ; then
        CFLAGS="-g3"
      fi
      if test "$CXXFLAGS" = "-g -O2" ; then
        CXXFLAGS="-g3"
      fi
      if test "$GCJFLAGS" = "-g -O2" ; then
        CXXFLAGS="-g3"
      fi;;
    esac

  if test "x$enable_compile_warnings" = "xyes" ||
     test "x$force_compile_warnings" = "xyes"; then
    AC_MSG_CHECKING(the warning flags for this compiler)
    warnCFLAGS=
    if test "x$CC" = "xgcc" || test "x$GCC" = "xyes" ; then
      case " $CFLAGS " in
      *-Wall*) ;;
      *) warnCFLAGS="-Wall -Wunused" ;;
      esac

      ## -W is not all that useful.  And it cannot be controlled
      ## with individual -Wno-xxx flags, unlike -Wall
      
      ## -Wformat=2 chokes on the snprintf replacement because the format is passed to real sprintf
      ## -Wshadow chokes on try{ try{} } constructs
      warnCFLAGS=`echo $warnCFLAGS  -Wmissing-prototypes -Wmissing-declarations \
        -Wpointer-arith -Wchar-subscripts -Wcomment -Wformat -Wwrite-strings \
        -Wno-unused-function -Wno-unused-parameter -Wno-strict-aliasing -Wno-format-nonliteral \
        -Werror \
	| sed 's/ +/ /g'`
	# -Wno-unused-variable  -Wno-unused-label
    fi
    AC_MSG_RESULT($warnCFLAGS)
    # placed before since gcc remembers the last one on conflict
    CFLAGS="$warnCFLAGS $CFLAGS"
  fi
  
  if test "x$enable_compile_optimizations" = "xyes" ||
     test "x$enable_compile_optimizations" = "xauto" ; then
    AC_MSG_CHECKING(the optimization flags for this compiler)
    optCFLAGS=
    if test "x$CC" = "xgcc" || test "x$GCC" = "xyes" ; then
        case " $CFLAGS " in
        *-O*) ;;
        *) optCFLAGS="$optCFLAGS -O3" ;;
        esac
        optCFLAGS="$optCFLAGS -finline-functions -funroll-loops -fno-strict-aliasing"
	# now that surf uses advanced maths in lagrangian, -ffast-math do break things
      
        GCC_VER=`gcc --version | head -n 1 | sed 's/^[^0-9]*\([^ ]*\).*$/\1/'`
        GCC_VER_MAJ=`echo $GCC_VER | sed 's/^\(.\).*$/\1/'`
        if test "x$target_cpu" = "xpowerpc" && test "x$GCC_VER_MAJ" == "x3" ; then
          # avoid gcc bug #12828, which apeared in 3.x branch and is fixed in 3.4.0
          # but the check would be too complicated to get 3.4. 
	  # Instead, rule out any 3.x compiler.
          
          # Note that the flag didn't exist before gcc 3.0
          optCFLAGS="$optCFLAGS -fno-loop-optimize"
        fi
        dnl A C_MSG_WARN(GCC_VER_MAJ=$GCC_VER_MAJ)
    fi
    AC_MSG_RESULT($optCFLAGS)
    # Take it only if CFLAGS not explicitly set. Unless the flag was explicitly given
    if test "x$cflags_set" != "xyes" ; then  
      CFLAGS="$optCFLAGS $CFLAGS"
    fi
  else
    CFLAGS="$CFLAGS -O0"
  fi

  if test x$lt_cv_prog_gnu_ld = xyes ; then
    LD_DYNAMIC_FLAGS=-Wl,--export-dynamic
  else
    LD_DYNAMIC_FLAGS=
  fi
  AC_SUBST(LD_DYNAMIC_FLAGS) 
  
])
