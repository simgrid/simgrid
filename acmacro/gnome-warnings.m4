

dnl GNOME_COMPILE_WARNINGS
dnl Turn on many useful compiler warnings
dnl For now, only works on GCC
AC_DEFUN([GNOME_COMPILE_WARNINGS],[
  AC_ARG_ENABLE(compile-warnings, 
    [  --enable-compile-warnings=[no/minimum/yes]       Turn on compiler warnings.],,enable_compile_warnings=yes)

  AC_MSG_CHECKING(what warning flags to pass to the C compiler)
  warnCFLAGS=
  if test "x$GCC" != xyes; then
    enable_compile_warnings=no
  fi

  if test "x$enable_compile_warnings" != "xno"; then
    if test "x$GCC" = "xyes"; then
      case " $CFLAGS " in
      *-Wall*) ;;
      *) warnCFLAGS="-g -Wall -Wunused" ;;
      esac

      ## -W is not all that useful.  And it cannot be controlled
      ## with individual -Wno-xxx flags, unlike -Wall
      if test "x$enable_compile_warnings" = "xyes"; then
        warnCFLAGS="$warnCFLAGS  -Wmissing-prototypes -Wmissing-declarations\
	            -finline-functions  -Wshadow -Wpointer-arith \
		    -Wwrite-strings -Wno-unused-function -Werror"
      fi
    fi
  fi
  AC_MSG_RESULT($warnCFLAGS)

  AC_ARG_ENABLE(iso-c,
    [  --enable-iso-c          Try to warn if code is not ISO C ],,
    enable_iso_c=no)

  AC_MSG_CHECKING(what language compliance flags to pass to the C compiler)
  complCFLAGS=
  if test "x$enable_iso_c" != "xno"; then
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
  fi
  AC_MSG_RESULT($complCFLAGS)
  if test "x$cflags_set" != "xyes"; then
    CFLAGS="$CFLAGS $warnCFLAGS $complCFLAGS"
    cflags_set=yes
    AC_SUBST(cflags_set)
  fi
])
