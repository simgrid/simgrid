dnl AC_CHECK_UCONTEXT: Check whether ucontext are working

dnl it uses AC_RUN and assume the worse while cross-compiling

AC_DEFUN([AC_CHECK_UCONTEXT],
 [
AC_MSG_CHECKING([whether ucontext'es exist and are usable...])
AC_RUN_IFELSE(AC_LANG_PROGRAM([#include <ucontext.h>],
                              [ucontext_t uc; 
			       if (getcontext (&uc) != 0) return -1;
			      ]),
	      AC_MSG_RESULT(yes)
	      ac_check_ucontext=yes,
	      AC_MSG_RESULT(no)
	      ac_check_ucontext=no,
	      AC_MSG_RESULT(assuming the worse in cross-compilation)
	      ac_check_ucontext=no)
])
