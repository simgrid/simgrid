

dnl GNOME_COMPILE_WARNINGS
dnl Turn on many useful compiler warnings
dnl For now, only works on GCC
AC_DEFUN([GNOME_COMPILE_WARNINGS],[
  AC_ARG_ENABLE(compile-warnings, 
    [  --enable-compile-warnings=[no/minimum/yes]       Turn on compiler warnings.],,enable_compile_warnings=yes)

  AC_MSG_CHECKING(the warning flags for this compiler)
  warnCFLAGS=
  if test "x$GCC" = "xyes"; then
    case " $CFLAGS " in
    *-Wall*) ;;
    *) warnCFLAGS="-g -Wall -Wunused" ;;
    esac

    ## -W is not all that useful.  And it cannot be controlled
    ## with individual -Wno-xxx flags, unlike -Wall
    if test "x$enable_compile_warnings" = "xyes"; then
      warnCFLAGS="$warnCFLAGS  -Wmissing-prototypes -Wmissing-declarations \
 -finline-functions  -Wshadow -Wpointer-arith -Wchar-subscripts -Wcomment \
 -Wformat=2 -Wno-unused-variable -Wno-unused-function -Wno-unused-label -Wwrite-strings \
 -Werror -O0"
    fi
  fi

  AC_MSG_RESULT($warnCFLAGS)

  AC_ARG_ENABLE(iso-c,
    [  --enable-iso-c          Try to warn if code is not ISO C ],,
    enable_iso_c=no)

  AC_MSG_CHECKING(the language compliance flags for this compiler)
  complCFLAGS=
  if test "x$GCC" = "xyes"; then
    case " $CFLAGS " in
    *-ansi*) ;;
    *) complCFLAGS="$complCFLAGS -ansi" ;;
    esac

    case " $CFLAGS " in
    *-pedantic*) ;;
    *) complCFLAGS="$complCFLAGS -pedantic" ;;
    esac
  fi
  AC_MSG_RESULT($complCFLAGS)
  
  
  AC_MSG_CHECKING(the optimization flags for this compiler)
  optCFLAGS=
  if test "x$GCC" = "xyes" ; then
      optCFLAGS="-ffast-math -funroll-loops -fno-strict-aliasing"
      case " $CFLAGS " in
      *-O*) ;;
      *) optCFLAGS="$optCFLAGS -O3" ;;
      esac
  fi
  AC_MSG_RESULT($optCFLAGS)

  # Take the right flags
  if test "x$cflags_set" != "xyes" ; then  
    if test "x$enable_iso_c" != "xno"; then
      CFLAGS="$CFLAGS $complCFLAGS $optCFLAGS"
    fi
    if test "x$enable_compile_warnings" != "xno" ; then
      CFLAGS="$CFLAGS $warnCFLAGS $optCFLAGS"
    fi  
    
    cflags_set=yes
    AC_SUBST(cflags_set)
  fi
])
