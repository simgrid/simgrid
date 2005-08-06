

dnl SG_COMPILE_FLAGS
dnl Turn on many useful compiler warnings
dnl For now, only sets extra flags on GCC

AC_DEFUN([SG_COMPILE_FLAGS],[
  AC_ARG_ENABLE(compile-warnings,
    AS_HELP_STRING([--enable-compile-warnings], [use compiler warnings (default=no, unless in maintainer mode)]),
    enable_compile_warnings=$withval,enable_compile_warnings=no)

  AC_ARG_ENABLE(compile-optimizations,
    AS_HELP_STRING([--disable-compile-optimizations], [use compiler optimizations (default=yes, unless if CFLAGS is explicitly set)]),
    enable_compile_optimizations=$enableval,enable_compile_optimizations=auto)

  if test "x$cflags_set" != "xyes" ; then 
    # if user didn't specify CFLAGS explicitely
    
    # AC PROG CC tests whether -g is accepted. 
    # Cool, but it also tries to set -O2. I don't want it with gcc
    saveCFLAGS="$CFLAGS"
    CFLAGS=
    case " $saveCFLAGS " in
    *-g*) CFLAGS="-g" ;; 
    esac
    case " $saveCFLAGS " in
    *-O2*) test "x$CC" = xgcc || CFLAGS="$CFLAGS -O2" ;; 
    esac
    
    # damn AC PROG CC, why did you set -O??
    CFLAGS="-g"
  fi

  if test "x$enable_compile_warnings" = "xyes" ; then
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
      if test "x$enable_compile_warnings" = "xyes"; then
        warnCFLAGS=`echo $warnCFLAGS  -Wmissing-prototypes -Wmissing-declarations \
        -Wpointer-arith -Wchar-subscripts -Wcomment -Wformat -Wwrite-strings \
        -Wno-unused-function  \
        -Werror \
	| sed 's/ +/ /g'`
	# -Wno-unused-variable  -Wno-unused-label
      fi
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
        optCFLAGS="$optCFLAGS -finline-functions -ffast-math -funroll-loops -fno-strict-aliasing"
      
        GCC_VER=`gcc --version | head -n 1 | sed 's/^[^0-9]*\([^ ]*\).*$/\1/'`
        GCC_VER_MAJ=`echo $GCC_VER | sed 's/^\(.\).*$/\1/'`
        if test "x$target_cpu" = "xpowerpc" && test "x$GCC_VER_MAJ" != "x2" ; then
          # avoid gcc bug #12828, which is fixed in 3.4.0, but this version
          # isn't propagated enough to desserve an extra check
          
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
  fi
  
])
