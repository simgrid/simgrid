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
   AC_COMPILE_IFELSE(AC_LANG_PROGRAM([#include "confdefs.h" 
   #include <sys/types.h> 
   $2 
   ], [switch (0) case 0: case (sizeof ($1) == $ac_size):;]), GRAS_CHECK_SIZEOF_RES=$ac_size) 
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
])

dnl
dnl GRAS_TWO_COMPLEMENT([type]): Make sure the type is two-complement
dnl warning, this does not work with char (quite logical)
dnl
AC_DEFUN([GRAS_TWO_COMPLEMENT],
[
AC_MSG_CHECKING(whether $1 is two-complement)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include "confdefs.h"
union {
   signed $1 testInt;
   unsigned char bytes[sizeof($1)];
} intTester;
]],[[
   intTester.testInt = -2;
   switch (0) {
    case 0:
    case (((unsigned int)intTester.bytes[0] +
	   (unsigned int)intTester.bytes[sizeof($1) - 1]) - 509) == 0:
   }
]])], dnl end of AC LANG PROGRAM
[two_complement=yes],[two_complement=no] )dnl end of AC COMPILE IFELSE

AC_MSG_RESULT($two_complement)
if test x$two_complement != xyes ; then
  AC_MSG_ERROR([GRAS works only two-complement integers (yet)])
fi
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

AC_MSG_RESULT($GRAS_CHECK_SIZEOF_RES) 
])

dnl
dnl End of CHECK_SIGNED_SIZEOF
dnl

# GRAS_STRUCT_BOUNDARY(): Check the minimal alignment boundary of $1 in structures
# ---------------------
# (using integer, I hope no arch let it change with the content)
AC_DEFUN([GRAS_STRUCT_BOUNDARY],
[changequote(<<, >>)dnl 
 dnl The cache variable name (and of the result). 
 define(<<GRAS_STRUCT_BOUNDARY_RES>>, translit(ac_cv_struct_boundary_$1, [ *()], [_pLR]))dnl
 changequote([, ])dnl 
 AC_MSG_CHECKING(for the minimal structure boundary of $1)

 AC_CACHE_VAL(GRAS_STRUCT_BOUNDARY_RES, 
 [for ac_size in 1 2 4 8 16 32 64 3 ; do # List sizes in rough order of prevalence. 
   AC_COMPILE_IFELSE(AC_LANG_PROGRAM([#include "confdefs.h" 
     #include <sys/types.h> 
     struct s { char c; $1 i; };
     ],[switch (0) case 0: case (sizeof (struct s) == $ac_size+sizeof($1)):;]), GRAS_STRUCT_BOUNDARY_RES=$ac_size)
   if test x$GRAS_STRUCT_BOUNDARY_RES != x ; then break; fi 
  done 
 ])
 AC_MSG_RESULT($GRAS_STRUCT_BOUNDARY_RES)
 if test x$GRAS_STRUCT_BOUNDARY_RES = x ; then
   AC_MSG_ERROR([Cannot determine the minimal alignment boundary of $1 in structures])
 fi
])

# GRAS_ARCH(): check the gras_architecture of this plateform
# -----------
# The different cases here must be syncronized with the array in src/base/DataDesc/ddt_convert.c
#
AC_DEFUN([GRAS_ARCH],
[
# Check for the architecture
AC_C_BIGENDIAN(endian=1,endian=0,AC_MSG_ERROR([GRAS works only for little or big endian systems (yet)]))
dnl Make sure we don't run on a non-two-complement arch, since we dunno convert them
dnl G RAS_TWO_COMPLEMENT(int) don't do this, it breaks cross-compile and all
dnl archs conform to this
AC_DEFINE_UNQUOTED(GRAS_BIGENDIAN,$endian,[define if big endian])
          
GRAS_SIGNED_SIZEOF(char)              GRAS_STRUCT_BOUNDARY(char)
GRAS_SIGNED_SIZEOF(short int)         GRAS_STRUCT_BOUNDARY(short int)
GRAS_SIGNED_SIZEOF(int)               GRAS_STRUCT_BOUNDARY(int)
GRAS_SIGNED_SIZEOF(long int)          GRAS_STRUCT_BOUNDARY(long int)
GRAS_SIGNED_SIZEOF(long long int)     GRAS_STRUCT_BOUNDARY(long long int)

GRAS_STRUCT_BOUNDARY(float)           GRAS_STRUCT_BOUNDARY(double)

GRAS_CHECK_SIZEOF(void *)             GRAS_STRUCT_BOUNDARY(void *)
GRAS_CHECK_SIZEOF(void (*) (void))    
                                                                  

AC_MSG_CHECKING(the GRAS signature of this architecture)
if test x$endian = x1 ; then
  trace_endian=B
else
  trace_endian=l
fi
gras_arch=unknown
trace="$trace_endian"

trace="${trace}_C:${ac_cv_sizeof_char}/${ac_cv_struct_boundary_char}:"

trace="${trace}_I:${ac_cv_sizeof_short_int}/${ac_cv_struct_boundary_short_int}"
trace="${trace}:${ac_cv_sizeof_int}/${ac_cv_struct_boundary_int}"
trace="${trace}:${ac_cv_sizeof_long_int}/${ac_cv_struct_boundary_long_int}"
trace="${trace}:${ac_cv_sizeof_long_long_int}/${ac_cv_struct_boundary_long_long_int}:"

trace="${trace}_P:${ac_cv_sizeof_void_p}/${ac_cv_struct_boundary_void_p}"
trace="${trace}:${ac_cv_sizeof_void_LpR_LvoidR}/${ac_cv_struct_boundary_void_p}:"

trace="${trace}_D:4/${ac_cv_struct_boundary_float}:8/${ac_cv_struct_boundary_double}:"

# sizeof float/double are not tested since IEEE 754 is assumed.
# Check README.IEEE for rational.

# The numbers after the _ in the arch name are the maximal packing boundary.
# big32_2   means that all data are aligned on a 2 bytes boundary.              (ARM)
# big32_8_4 means that some or them are aligned on 8 bytes, some are on 4 bytes (AIX)
case $trace in
  l_C:1/1:_I:2/2:4/4:4/4:8/8:_P:4/4:4/4:_D:4/4:8/8:) gras_arch=0; gras_arch_name=little32;;
  l_C:1/1:_I:2/2:4/4:4/4:8/4:_P:4/4:4/4:_D:4/4:8/4:) gras_arch=1; gras_arch_name=little32_4;;
  
  l_C:1/1:_I:2/2:4/4:8/8:8/8:_P:8/8:8/8:_D:4/4:8/8:) gras_arch=2; gras_arch_name=little64;;
  
  B_C:1/1:_I:2/2:4/4:4/4:8/8:_P:4/4:4/4:_D:4/4:8/8:) gras_arch=3; gras_arch_name=big32;;
  B_C:1/1:_I:2/2:4/4:4/4:8/8:_P:4/4:4/4:_D:4/4:8/4:) gras_arch=4; gras_arch_name=big32_8_4;;
  B_C:1/1:_I:2/2:4/4:4/4:8/4:_P:4/4:4/4:_D:4/4:8/4:) gras_arch=5; gras_arch_name=big32_4;;
  B_C:1/1:_I:2/2:4/2:4/2:8/2:_P:4/2:4/2:_D:4/2:8/2:) gras_arch=6; gras_arch_name=big32_2;;
  
  B_C:1/1:_I:2/2:4/4:8/8:8/8:_P:8/8:8/8:_D:4/4:8/8:) gras_arch=7; gras_arch_name=big64;;
  B_C:1/1:_I:2/2:4/4:8/8:8/8:_P:8/8:8/8:_D:4/4:8/4:) gras_arch=8; gras_arch_name=big64_8_4;;
esac
if test x$gras_arch = xunknown ; then
  AC_MSG_RESULT([damnit ($trace)])
  AC_MSG_ERROR([Impossible to determine the GRAS architecture signature.
Please report this architecture trace ($trace) and what it corresponds to.])
fi
echo "$as_me:$LINENO: GRAS trace of this machine: $trace" >&AS_MESSAGE_LOG_FD
AC_DEFINE_UNQUOTED(GRAS_THISARCH,$gras_arch,[defines the GRAS architecture signature of this machine])
AC_MSG_RESULT($gras_arch ($gras_arch_name))
])
# END OF GRAS ARCH CHECK

AC_DEFUN([GRAS_CHECK_STRUCT_COMPACTION],
[  AC_MSG_CHECKING(whether the struct gets packed)
   AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include "confdefs.h" 
        #include <sys/types.h>
	#include <stddef.h> /* offsetof() */
        struct s {char c; double d;}; 
       ]],[[switch (0) 
        case 0: 
        case (sizeof(struct s) == sizeof(double)+sizeof(char)):;
       ]])],[gras_struct_packed=yes],[gras_struct_packed=no])
     
   AC_MSG_RESULT($gras_struct_packed)
   if test x$gras_struct_packed = "xyes" ; then
     AC_MSG_ERROR([GRAS does not support packed structures since it leads to nasty misalignments])
   fi
   
   AC_MSG_CHECKING(whether the struct gets compacted)
   AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include "confdefs.h" 
        #include <sys/types.h>
	#include <stddef.h> /* offsetof() */
        struct s {double d; int i; char c;}; 
       ]],[[switch (0) 
        case 0: 
        case (offsetof(struct s,c) == sizeof(double)+sizeof(int)):;
       ]])],[gras_struct_compact=yes],[gras_struct_compact=no])
     
   AC_MSG_RESULT($gras_struct_compact)
   
   if test x$gras_struct_compact != xyes ; then
     AC_MSG_ERROR([GRAS works only on structure compacting architectures (yet)])
   fi
   AC_DEFINE_UNQUOTED(GRAS_STRUCT_COMPACT, 1,
      [Defined if structures are compacted when possible.
	  
       Consider this structure: struct s {double d; int i; char c;}; 
	  
       If it is allowed, the char is placed just after the int. If not,
       it has to be on the 8 bytes boundary imposed by the double.
       
       For now, GRAS requires the structures to be compacted.
      ])
   AC_MSG_CHECKING(whether arrays can straddle struct alignment boundaries)

   AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include "confdefs.h" 
      #include <sys/types.h>
	#include <stddef.h> /* offsetof() */
	struct s { double d; int i; char c[6]; };
     ]],[[switch (0) 
      case 0: 
      case (offsetof(struct s,c) == sizeof(double)+sizeof(int)):;
     ]])],[gras_array_straddle_struct=yes],[gras_array_straddle_struct=no])
     
   AC_MSG_RESULT($gras_array_straddle_struct)

   if test x$gras_array_straddle_struct = xyes ; then
     AC_DEFINE_UNQUOTED(GRAS_ARRAY_STRADDLE_STRUCT, 1,
        [Defined if arrays in struct can straddle struct alignment boundaries.
	
	 This is like than the structure compaction above, but this time,
         the argument to be compacted is an array whom each element would be
         normally compacted. Exemple:
	 
	 struct s { double d; int i; char c[6]; };
	 
	 Arrays can straddle if c is allowed to come just after i.
	 
	 Note that GRAS only support architecture presenting this
         caracteristic so far.
	])
   else
     AC_MSG_ERROR([GRAS can only work on architectures allowing array fields to straddle alignment boundaries (yet)])
   fi
])
# END OF GRAS CHECK STRUCT COMPACTION

AC_DEFUN([GRAS_C_COMPACT_STRUCT],
[  AC_MSG_CHECKING(whether the struct gets compacted)
   AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include "confdefs.h" 
        #include <sys/types.h>
	#include <stddef.h> /* offsetof() */
        struct s {double d; int a; int b;}; 
       ]],[[switch (0) 
        case 0: 
        case (offsetof(struct s,b) == sizeof(double)+sizeof(int)):;
       ]])],[gras_compact_struct=yes],[gras_compact_struct=no])
     
   AC_MSG_RESULT($gras_compact_struct)
   
   if test x$gras_compact_struct = xyes ; then
     AC_DEFINE_UNQUOTED(GRAS_COMPACT_STRUCT, 1,
          [Defined if structures are compacted when possible])
   fi	  
])
# END OF GRAS COMPACT STRUCT

