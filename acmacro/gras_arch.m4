# BEGIN OF GRAS ARCH CHECK
dnl
dnl GRAS_DO_CHECK_SIZEOF: Get the size of a datatype (even in cross-compile)
dnl  1: type tested
dnl  2: extra include lines
dnl  3: extra sizes to test
dnl ("adapted" from openldap)
dnl
AC_DEFUN([GRAS_DO_CHECK_SIZEOF],
[changequote(<<, >>)dnl 
 dnl The cache variable name (and of the result). 
 define(<<GRAS_CHECK_SIZEOF_RES>>, translit(ac_cv_sizeof_$1, [ *()], [_pLR]))dnl
 changequote([, ])dnl 
 AC_CACHE_VAL(GRAS_CHECK_SIZEOF_RES, 
 [for ac_size in 4 8 1 2 16 $3 ; do # List sizes in rough order of prevalence. 
   AC_TRY_COMPILE([#include "confdefs.h" 
   #include <sys/types.h> 
   $2 
   ], [switch (0) case 0: case (sizeof ($1) == $ac_size):;], GRAS_CHECK_SIZEOF_RES=$ac_size) 
   if test x$GRAS_CHECK_SIZEOF_RES != x ; then break; fi 
  done 
 ]) 
])

dnl
dnl GRAS_CHECK_SIZEOF: Get the size of a datatype (even in cross-compile), with msg display, and define the result
dnl  1: type tested
dnl  2: extra include lines
dnl  3: extra sizes to test
dnl ("adapted" from openldap)
dnl
AC_DEFUN([GRAS_CHECK_SIZEOF],
[AC_MSG_CHECKING(size of $1) 
GRAS_DO_CHECK_SIZEOF($1,$2)
if test x$GRAS_CHECK_SIZEOF_RES = x ; then 
  AC_MSG_ERROR([cannot determine a size for $1]) 
fi 
AC_MSG_RESULT($GRAS_CHECK_SIZEOF_RES) 
changequote(<<, >>)dnl 
dnl The name to #define. 
define(<<TYPE_NAME>>, translit(sizeof_$1, [a-z *()], [A-Z_P__]))dnl 
changequote([, ])dnl 
AC_DEFINE_UNQUOTED(TYPE_NAME, $GRAS_CHECK_SIZEOF_RES, [The number of bytes in type $1]) 
undefine([TYPE_NAME])dnl 
])

dnl
dnl GRAS_TWO_COMPLIMENT: Make sure the type is two-compliment
dnl
AC_DEFUN([GRAS_TWO_COMPLIMENT],
[
AC_TRY_COMPILE([#include "confdefs.h"
union {
   signed $1 testInt;
   unsigned char bytes[sizeof($1)];
} intTester;
],[
   intTester.testInt = -2;
   return ((unsigned int)intTester.bytes[0] +
	   (unsigned int)intTester.bytes[sizeof($1) - 1]) != 509;
],[],[AC_MSG_ERROR([GRAS works only two-compliment integers (yet)])])dnl end of AC_TRY_COMPILE
])

dnl
dnl GRAS_SIGNED_SIZEOF: Get the size of the datatype, and make sure that
dnl signed, unsigned and unspecified have the same size
dnl
AC_DEFUN([GRAS_SIGNED_SIZEOF],
[AC_MSG_CHECKING(size of $1)
GRAS_DO_CHECK_SIZEOF($1,$2)
unspecif=$GRAS_CHECK_SIZEOF_RES
if test x$unspecif = x ; then
  AC_MSG_ERROR([cannot determine a size for $1]) 
fi 

GRAS_DO_CHECK_SIZEOF(signed $1,$2)
signed=$GRAS_CHECK_SIZEOF_RES
if test x$signed = x ; then
  AC_MSG_ERROR([cannot determine a size for signed $1]) 
fi 
if test x$unspecif != x$signed ; then
  AC_MSG_ERROR(['signed $1' and '$1' have different sizes ! ($signed != $unspecif)]) 
fi 

GRAS_DO_CHECK_SIZEOF(unsigned $1,$2)
unsigned=$GRAS_CHECK_SIZEOF_RES
if test x$unsigned = x ; then
  AC_MSG_ERROR([cannot determine a size for unsigned $1]) 
fi 
if test x$unsigned != x$signed ; then
  AC_MSG_ERROR(['signed $1' and 'unsigned $1' have different sizes ! ($signed != $unsigned)]) 
fi 

dnl Make sure we don't run on a non-two-compliment arch, since we dunno
dnl convert them
GRAS_TWO_COMPLIMENT($1)

AC_MSG_RESULT($GRAS_CHECK_SIZEOF_RES) 
changequote(<<, >>)dnl 
dnl The name to #define. 
define(<<TYPE_NAME>>, translit(sizeof_$1, [a-z *()], [A-Z_P__]))dnl 
changequote([, ])dnl 
AC_DEFINE_UNQUOTED(TYPE_NAME, $GRAS_CHECK_SIZEOF_RES, [The number of bytes in type $1]) 
undefine([TYPE_NAME])dnl 
])

dnl
dnl End of CHECK_SIGNED_SIZEOF
dnl

dnl CHECK_IEEE_FP: determines if floating points are IEEE754 compliant
dnl (inspired from NWS code)
dnl
AC_DEFUN([CHECK_IEEE_FP],
[AC_MSG_CHECKING(if floating point datatypes are IEEE 754 compliant) 
AC_TRY_COMPILE([#include "confdefs.h"
union {
        double testFP;
        unsigned char bytes[sizeof(double)];
} doubleTester;
union {
        float testFP;
        unsigned char bytes[sizeof(float)];
} floatTester;
],[
if (sizeof(double) != 8 || sizeof(float) != 4)
   return 1;

memset(&doubleTester, 0, sizeof(doubleTester));
memset(&floatTester, 0, sizeof(floatTester));

doubleTester.bytes[GRAS_BIGENDIAN ? sizeof(double) - 1 : 0]=192;
doubleTester.bytes[GRAS_BIGENDIAN ? sizeof(double) - 2 : 1] =
  (sizeof(double) == 4)  ? 128 :
  (sizeof(double) == 8)  ? 16 :
  (sizeof(double) == 16) ? 1 : 0;
if (doubleTester.testFP != -4.0) return 1;

floatTester.bytes[GRAS_BIGENDIAN ? sizeof(float) - 1 : 0]=192;
floatTester.bytes[GRAS_BIGENDIAN ? sizeof(float) - 2 : 1] =
  (sizeof(float) == 4)  ? 128 :
  (sizeof(float) == 8)  ? 16 :
  (sizeof(float) == 16) ? 1 : 0;
if (floatTester.testFP != -4.0) return 1;

return 0;],[IEEE_FP=yes
AC_DEFINE(IEEE_FP,1,[defines if this architecture floating point types are IEEE754 compliant])
],[IEEE_FP=no 
   AC_MSG_ERROR([GRAS works only for little or big endian systems (yet)])])dnl end of AC_TRY_COMPILE
AC_MSG_RESULT($IEEE_FP)
])dnl end of CHECK_IEEE_FP



dnl *************************8
dnl 
AC_DEFUN([GRAS_ARCH],
[
# Check for the architecture
AC_C_BIGENDIAN(endian=1,endian=0,AC_MSG_ERROR([GRAS works only for little or big endian systems (yet)]))
AC_DEFINE_UNQUOTED(GRAS_BIGENDIAN,$endian,[define if big endian])
          
GRAS_SIGNED_SIZEOF(char)
GRAS_SIGNED_SIZEOF(short int)
GRAS_SIGNED_SIZEOF(int)
GRAS_SIGNED_SIZEOF(long int)
GRAS_SIGNED_SIZEOF(long long int)
                                                                  

GRAS_CHECK_SIZEOF(void *)
GRAS_CHECK_SIZEOF(void (*) (void))
                                                                  
GRAS_CHECK_SIZEOF(float)
GRAS_CHECK_SIZEOF(double)
CHECK_IEEE_FP
								  
AC_MSG_CHECKING(the GRAS signature of this architecture)
if test x$endian = x1 ; then
  trace_endian=B
else
  trace_endian=l
fi
gras_arch=unknown
trace="$trace_endian"
trace="${trace}:${ac_cv_sizeof_char}.${ac_cv_sizeof_short_int}.${ac_cv_sizeof_int}.${ac_cv_sizeof_long_int}.${ac_cv_sizeof_long_long_int}"
trace="${trace}:${ac_cv_sizeof_void_p}.${ac_cv_sizeof_void_LpR_LvoidR}"
trace="${trace}:${ac_cv_sizeof_float}.${ac_cv_sizeof_double}"
case $trace in
  l:1.2.4.4.8:4.4:4.8) gras_arch=0; gras_arch_name=little32;;
  l:1.2.4.8.8:8.8:4.8) gras_arch=1; gras_arch_name=little64;;
  B:1.2.4.4.8:4.4:4.8) gras_arch=2; gras_arch_name=big32;;
  B:1.2.4.8.8:8.8:4.8) gras_arch=3; gras_arch_name=big64;;
esac
if test x$gras_arch = xunknown ; then
  AC_MSG_RESULT([damnit ($trace)])
  AC_MSG_ERROR([Impossible to determine the GRAS architecture signature.
Please report this architecture trace ($trace) and what it corresponds to.])
fi
echo "$as_me:$LINENO: GRAS trace of this machine: $trace" >&AS_MESSAGE_LOG_FD
AC_DEFINE_UNQUOTED(GRAS_THISARCH_NAME,"$gras_arch_name",[defines the GRAS architecture name of this machine])
AC_DEFINE_UNQUOTED(GRAS_THISARCH,$gras_arch,[defines the GRAS architecture signature of this machine])
AC_MSG_RESULT($gras_arch ($gras_arch_name))
])
# END OF GRAS ARCH CHECK
