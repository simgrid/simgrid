dnl AC_CHECK_PRINTF_NULL: Check whether printf("%s",NULL) works or segfault

dnl it uses AC_RUN and assume the worse while cross-compiling

dnl Then, you can:
dnl #ifndef PRINTF_NULL_WORKING
dnl #  define PRINTF_STR(a) (a)?:"(null)" 
dnl #else
dnl #  define PRINTF_STR(a) (a)
dnl #endif

AC_DEFUN([AC_CHECK_PRINTF_NULL],
 [
AC_MSG_CHECKING([whether printf("%s",NULL) works...])
AC_RUN_IFELSE(AC_LANG_PROGRAM([#include <stdio.h>],
                              [printf("%s",NULL);]),
              AC_DEFINE(PRINTF_NULL_WORKING,
	                1,
			[Indicates whether printf("%s",NULL) works])
	      AC_MSG_RESULT(yes),
	      AC_MSG_RESULT(no),
	      AC_MSG_RESULT(assuming the worse in cross-compilation))
])
