dnl @synopsis AC_FUNC_ASPRINTF
dnl
dnl Checks for a compilable asprintf. 

dnl Define NEED_ASPRINTF  (to 1) if no working asprintf is found
dnl Define NEED_VASPRINTF (to 1) if no working vasprintf is found
dnl
dnl Note: the mentioned replacement is freely available and
dnl may be used in any project regardless of it's licence (just like
dnl the autoconf special exemption).
dnl
dnl @category C
dnl @author Martin Quinson <Martin.Quinson@loria.fr>
dnl @version 2009-12-11
dnl @license AllPermissive

AC_DEFUN([AC_FUNC_ASPRINTF],
[AC_CHECK_FUNCS(asprintf vasprintf)
AC_MSG_CHECKING(for working asprintf)
AC_CACHE_VAL(ac_cv_have_working_asprintf,
[AC_TRY_RUN(
[#include <stdio.h>

int main(void)
{
    char *buff;
    int i = asprintf(&buff, "%s","toto");
    if (strcmp (buff, "toto")) exit (2);
    if (i != 4) exit (3);
    exit(0);
}], ac_cv_have_working_asprintf=yes, ac_cv_have_working_asprintf=no, ac_cv_have_working_asprintf=cross)])
AC_MSG_RESULT([$ac_cv_have_working_asprintf])
AC_MSG_CHECKING(for working vasprintf)
AC_CACHE_VAL(ac_cv_have_working_vasprintf,
[AC_TRY_RUN(
[#include <stdio.h>
#include <stdarg.h>

int my_vasprintf (char **buf, const char *tmpl, ...)
{
    int i;
    va_list args;
    va_start (args, tmpl);
    i = vasprintf (buf, tmpl, args);
    va_end (args);
    return i;
}

int main(void)
{
    char *buff;
    int i = my_vasprintf(&buff, "%s","toto");
    if (strcmp (buff, "toto")) exit (2);
    if (i != 4) exit (3);
    exit(0);
}], ac_cv_have_working_vasprintf=yes, ac_cv_have_working_vasprintf=no, ac_cv_have_working_vasprintf=cross)])
AC_MSG_RESULT([$ac_cv_have_working_vasprintf])
if test x$ac_cv_have_working_asprintf$ac_cv_have_working_vasprintf != "xyesyes"; then
#  AC_LIBOBJ(asprintf)
  AC_MSG_WARN([Replacing missing/broken (v)asprintf() with internal version.])
  AC_DEFINE(NEED_ASPRINTF,  1, enable the asprintf replacement)
  AC_DEFINE(NEED_VASPRINTF, 1, enable the vasprintf replacement)
AC_DEFINE(PREFER_PORTABLE_ASPRINTF, 1, "enable replacement (v)asprintf if system (v)asprintf is broken")
fi])
  