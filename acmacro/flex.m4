dnl AC_PROG_FLEX: Check whether the LEXer is flex, and which version it has
dnl The first arg must be a version number with 3 parts.

dnl hint: I guess you want to check for a version number higher to 2.5.31

AC_DEFUN([_AC_PROG_FLEX_HELPER_TOO_OLD],[
  AC_MSG_NOTICE([Found flex is too old. Parsers won't get updated (Found v$FLEX_VERSION < v$1)])
  LEX="$SHELL $missing_dir/missing flex";
  AC_SUBST(LEXLIB, '')
])

AC_DEFUN([AC_PROG_FLEX],
 [
  AC_PROG_LEX
  if test "$LEX" != flex; then
    AC_MSG_NOTICE([Flex not found. Parsers won't get updated.])
    LEX="$SHELL $missing_dir/missing flex";
    AC_SUBST(LEXLIB, '')
  else
    if test "x$1" != "x" ; then
      dnl
      dnl We were asked to check the version number
      dnl
      set -x
      changequote(<<, >>)dnl because of the regexp [blabla]
      FLEX_VERSION=`flex --version | sed -e 's/^[^0-9]*//' -e 's/[^0-9]*$//'`

      FLEX_VER_MAJ=`echo "$FLEX_VERSION" | sed 's/\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\)/\1/'`
      FLEX_VER_MED=`echo "$FLEX_VERSION" | sed 's/\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\)/\2/'`
      FLEX_VER_MIN=`echo "$FLEX_VERSION" | sed 's/\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\)/\3/'`

      WANT_VER_MAJ=`echo $1 | sed 's/\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\)/\1/'`;
      WANT_VER_MED=`echo $1 | sed 's/\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\)/\2/'`;
      WANT_VER_MIN=`echo $1 | sed 's/\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\)/\3/'`;
      changequote([, ])dnl back to normality, there is no regexp afterward
 
      if test "$FLEX_VER_MAJ" -lt "$WANT_VER_MAJ" ||
         test "$FLEX_VER_MAJ" -eq "$WANT_VER_MAJ" -a "$FLEX_VER_MED" -lt "$WANT_VER_MED"  ||
         test "$FLEX_VER_MAJ" -eq "$WANT_VER_MAJ" -a "$FLEX_VER_MED" -eq "$WANT_VER_MED" -a "$FLEX_VER_MIN" -lt "$WANT_VER_MIN" ;
      then
        AC_MSG_NOTICE([Found flex is too old. Parsers won't get updated (Found v$FLEX_VERSION < v$1)])
        LEX="$SHELL $missing_dir/missing flex";
        AC_SUBST(LEXLIB, '')
      fi
      set +x
    fi
  fi
  AC_MSG_NOTICE([Flex found. Parsers will get updated])
  exit 0
])
