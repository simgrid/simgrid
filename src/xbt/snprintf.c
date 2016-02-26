/* Copyright (c) 2005-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/*
 * snprintf.c - a portable implementation of snprintf
 *
 * AUTHOR
 *   Mark Martinec <mark.martinec@ijs.si>, April 1999.
 *
 *   Copyright 1999, Mark Martinec. All rights reserved.
 *
 * TERMS AND CONDITIONS
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the "Frontier Artistic License" which comes
 *   with this Kit.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *   See the Frontier Artistic License for more details.
 *
 *   You should have received a copy of the Frontier Artistic License
 *   with this Kit in the file named LICENSE.txt .
 *   If not, I'll be glad to provide one.
 *
 * FEATURES
 * - careful adherence to specs regarding flags, field width and precision;
 * - good performance for large string handling (large format, large
 *   argument or large paddings). Performance is similar to system's sprintf
 *   and in several cases significantly better (make sure you compile with
 *   optimizations turned on, tell the compiler the code is strict ANSI
 *   if necessary to give it more freedom for optimizations);
 * - return value semantics per ISO/IEC 9899:1999 ("ISO C99");
 * - written in standard ISO/ANSI C - requires an ANSI C compiler.
 *
 * SUPPORTED CONVERSION SPECIFIERS AND DATA TYPES
 *
 * This snprintf only supports the following conversion specifiers:
 * s, c, d, u, o, x, X, p  (and synonyms: i, D, U, O - see below)
 * with flags: '-', '+', ' ', '0' and '#'.
 * An asterisk is supported for field width as well as precision.
 *
 * Length modifiers 'h' (short int), 'l' (long int),
 * and 'll' (long long int) are supported.
 * NOTE:
 *   If macro SNPRINTF_LONGLONG_SUPPORT is not defined (default) the
 *   length modifier 'll' is recognized but treated the same as 'l',
 *   which may cause argument value truncation! Defining
 *   SNPRINTF_LONGLONG_SUPPORT requires that your system's sprintf also
 *   handles length modifier 'll'.  long long int is a language extension
 *   which may not be portable.
 *
 * Conversion of numeric data (conversion specifiers d, u, o, x, X, p)
 * with length modifiers (none or h, l, ll) is left to the system routine
 * sprintf, but all handling of flags, field width and precision as well as
 * c and s conversions is done very carefully by this portable routine.
 * If a string precision (truncation) is specified (e.g. %.8s) it is
 * guaranteed the string beyond the specified precision will not be referenced.
 *
 * Length modifiers h, l and ll are ignored for c and s conversions (data
 * types wint_t and wchar_t are not supported).
 *
 * The following common synonyms for conversion characters are supported:
 *   - i is a synonym for d
 *   - D is a synonym for ld, explicit length modifiers are ignored
 *   - U is a synonym for lu, explicit length modifiers are ignored
 *   - O is a synonym for lo, explicit length modifiers are ignored
 * The D, O and U conversion characters are nonstandard, they are supported
 * for backward compatibility only, and should not be used for new code.
 *
 * The following is specifically NOT supported:
 *   - flag ' (thousands' grouping character) is recognized but ignored
 *   - numeric conversion specifiers: f, e, E, g, G and synonym F,
 *     as well as the new a and A conversion specifiers
 *   - length modifier 'L' (long double) and 'q' (quad - use 'll' instead)
 *   - wide character/string conversions: lc, ls, and nonstandard
 *     synonyms C and S
 *   - writeback of converted string length: conversion character n
 *   - the n$ specification for direct reference to n-th argument
 *   - locales
 *
 * It is permitted for str_m to be zero, and it is permitted to specify NULL
 * pointer for resulting string argument if str_m is zero (as per ISO C99).
 *
 * The return value is the number of characters which would be generated
 * for the given input, excluding the trailing null. If this value
 * is greater or equal to str_m, not all characters from the result
 * have been stored in str, output bytes beyond the (str_m-1) -th character
 * are discarded. If str_m is greater than zero it is guaranteed
 * the resulting string will be null-terminated.
 *
 * NOTE that this matches the ISO C99, OpenBSD, and GNU C library 2.1,
 * but is different from some older and vendor implementations,
 * and is also different from XPG, XSH5, SUSv2 specifications.
 * For historical discussion on changes in the semantics and standards
 * of snprintf see printf(3) man page in the Linux programmers manual.
 *
 * Routines asprintf and vasprintf return a pointer (in the ptr argument)
 * to a buffer sufficiently large to hold the resulting string. This pointer
 * should be passed to free(3) to release the allocated storage when it is
 * no longer needed. If sufficient space cannot be allocated, these functions
 * will return -1 and set ptr to be a NULL pointer. These two routines are a
 * GNU C library extensions (glibc).
 *
 * Routines asnprintf and vasnprintf are similar to asprintf and vasprintf,
 * yet, like snprintf and vsnprintf counterparts, will write at most str_m-1
 * characters into the allocated output string, the last character in the
 * allocated buffer then gets the terminating null. If the formatted string
 * length (the return value) is greater than or equal to the str_m argument,
 * the resulting string was truncated and some of the formatted characters
 * were discarded. These routines present a handy way to limit the amount
 * of allocated memory to some sane value.
 *
 * AVAILABILITY
 *   http://www.ijs.si/software/snprintf/
 *
 * REVISION HISTORY
 * 1999-04  V0.9  Mark Martinec
 *    - initial version, some modifications after comparing printf
 *      man pages for Digital Unix 4.0, Solaris 2.6 and HPUX 10,
 *      and checking how Perl handles sprintf (differently!);
 * 1999-04-09  V1.0  Mark Martinec <mark.martinec@ijs.si>
 *    - added main test program, fixed remaining inconsistencies,
 *      added optional (long long int) support;
 * 1999-04-12  V1.1  Mark Martinec <mark.martinec@ijs.si>
 *    - support the 'p' conversion (pointer to void);
 *    - if a string precision is specified
 *      make sure the string beyond the specified precision
 *      will not be referenced (e.g. by strlen);
 * 1999-04-13  V1.2  Mark Martinec <mark.martinec@ijs.si>
 *    - support synonyms %D=%ld, %U=%lu, %O=%lo;
 *    - speed up the case of long format string with few conversions;
 * 1999-06-30  V1.3  Mark Martinec <mark.martinec@ijs.si>
 *    - fixed runaway loop (eventually crashing when str_l wraps
 *      beyond 2^31) while copying format string without
 *      conversion specifiers to a buffer that is too short
 *      (thanks to Edwin Young <edwiny@autonomy.com> for
 *      spotting the problem);
 *    - added macros PORTABLE_SNPRINTF_VERSION_(MAJOR|MINOR)
 *      to snprintf.h
 * 2000-02-14  V2.0 (never released) Mark Martinec <mark.martinec@ijs.si>
 *    - relaxed license terms: The Artistic License now applies.
 *      You may still apply the GNU GENERAL PUBLIC LICENSE
 *      as was distributed with previous versions, if you prefer;
 *    - changed REVISION HISTORY dates to use ISO 8601 date format;
 *    - added vsnprintf (patch also independently proposed by
 *      Caolan McNamara 2000-05-04, and Keith M Willenson 2000-06-01)
 * 2000-06-27  V2.1  Mark Martinec <mark.martinec@ijs.si>
 *    - removed POSIX check for str_m<1; value 0 for str_m is
 *      allowed by ISO C99 (and GNU C library 2.1) - (pointed out
 *      on 2000-05-04 by Caolan McNamara, caolan@ csn dot ul dot ie).
 *      Besides relaxed license this change in standards adherence
 *      is the main reason to bump up the major version number;
 *    - added nonstandard routines asnprintf, vasnprintf, asprintf,
 *      vasprintf that dynamically allocate storage for the
 *      resulting string; these routines are not compiled by default,
 *      see comments where NEED_V?ASN?PRINTF macros are defined;
 *    - autoconf contributed by Caolan McNamara
 * 2000-10-06  V2.2  Mark Martinec <mark.martinec@ijs.si>
 *    - BUG FIX: the %c conversion used a temporary variable
 *      that was no longer in scope when referenced,
 *      possibly causing incorrect resulting character;
 *    - BUG FIX: make precision and minimal field width unsigned
 *      to handle huge values (2^31 <= n < 2^32) correctly;
 *      also be more careful in the use of signed/unsigned/size_t
 *      internal variables - probably more careful than many
 *      vendor implementations, but there may still be a case
 *      where huge values of str_m, precision or minimal field
 *      could cause incorrect behaviour;
 *    - use separate variables for signed/unsigned arguments,
 *      and for short/int, long, and long long argument lengths
 *      to avoid possible incompatibilities on certain
 *      computer architectures. Also use separate variable
 *      arg_sign to hold sign of a numeric argument,
 *      to make code more transparent;
 *    - some fiddling with zero padding and "0x" to make it
 *      Linux compatible;
 *    - systematically use macros fast_memcpy and fast_memset
 *      instead of case-by-case hand optimization; determine some
 *      breakeven string lengths for different architectures;
 *    - terminology change: 'format' -> 'conversion specifier',
 *      'C9x' -> 'ISO/IEC 9899:1999 ("ISO C99")',
 *      'alternative form' -> 'alternate form',
 *      'data type modifier' -> 'length modifier';
 *    - several comments rephrased and new ones added;
 *    - make compiler not complain about 'credits' defined but
 *      not used;
 */

/* Define NEED_V?ASN?PRINTF macros if you need library extension
 * routines asprintf, vasprintf, asnprintf, vasnprintf respectively,
 * and your system library does not provide them. They are all small
 * wrapper routines around portable_vsnprintf. Defining any of the four
 * NEED_V?ASN?PRINTF macros automatically turns off NEED_SNPRINTF_ONLY
 * and turns on PREFER_PORTABLE_SNPRINTF.
 *
 * Watch for name conflicts with the system library if these routines
 * are already present there.
 *
 * NOTE: vasprintf and vasnprintf routines need va_copy() from stdarg.h, as
 * specified by C99, to be able to traverse the same list of arguments twice.
 * I don't know of any other standard and portable way of achieving the same.
 * With some versions of gcc you may use __va_copy(). You might even get away
 * with "ap2 = ap", in this case you must not call va_end(ap2) !
 *   #define va_copy(ap2,ap) ap2 = ap
 */
/* #define NEED_ASPRINTF   */
/* #define NEED_ASNPRINTF  */
/* #define NEED_VASPRINTF  */
/* #define NEED_VASNPRINTF */


/* ============================================= */
/* NO USER SERVICABLE PARTS FOLLOWING THIS POINT */
/* ============================================= */

#include "src/portable.h"           /* to get a working stdarg.h */

#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <assert.h>
#include <errno.h>
#include "xbt/str.h"

#if defined(NEED_ASPRINTF)
int asprintf(char **ptr, const char *fmt, /*args */ ...);
#endif
#if defined(NEED_VASPRINTF)
int vasprintf(char **ptr, const char *fmt, va_list ap);
#endif


  /* FIXME: better place */
#include "xbt/sysdep.h"

/* declarations */

/*
    Old copyright
    snprintf.c, v2.2: Mark Martinec, <mark.martinec@ijs.si>
    snprintf.c, v2.2: Copyright 1999, Mark Martinec. Frontier Artistic License applies.
    snprintf.c, v2.2: http://www.ijs.si/software/snprintf
*/

#if defined(NEED_ASPRINTF)
int asprintf(char **ptr, const char *fmt, /*args */ ...)
{
  va_list ap;
  size_t str_m;
  int str_l;

  *ptr = NULL;
  va_start(ap, fmt);            /* measure the required size */
  str_l = portable_vsnprintf(NULL, (size_t) 0, fmt, ap);
  va_end(ap);
  assert(str_l >= 0);           /* possible integer overflow if str_m > INT_MAX */
  *ptr = (char *) xbt_malloc(str_m = (size_t) str_l + 1);
  if (*ptr == NULL) {
    errno = ENOMEM;
    str_l = -1;
  } else {
    int str_l2;
    va_start(ap, fmt);
    str_l2 = portable_vsnprintf(*ptr, str_m, fmt, ap);
    va_end(ap);
    assert(str_l2 == str_l);
  }
  return str_l;
}
#endif

#if defined(NEED_VASPRINTF)
int vasprintf(char **ptr, const char *fmt, va_list ap)
{
  size_t str_m;
  int str_l;

  *ptr = NULL;
  {
    va_list ap2;
    va_copy(ap2, ap);           /* don't consume the original ap, we'll need it again */
    str_l = portable_vsnprintf(NULL, (size_t) 0, fmt, ap2);     /*get required size */
    va_end(ap2);
  }
  assert(str_l >= 0);           /* possible integer overflow if str_m > INT_MAX */
  *ptr = (char *) xbt_malloc(str_m = (size_t) str_l + 1);
  if (*ptr == NULL) {
    errno = ENOMEM;
    str_l = -1;
  } else {
    int str_l2 = portable_vsnprintf(*ptr, str_m, fmt, ap);
    assert(str_l2 == str_l);
  }
  return str_l;
}
#endif

#if defined(NEED_ASNPRINTF)
int asnprintf(char **ptr, size_t str_m, const char *fmt, /*args */ ...)
{
  va_list ap;
  int str_l;

  *ptr = NULL;
  va_start(ap, fmt);            /* measure the required size */
  str_l = portable_vsnprintf(NULL, (size_t) 0, fmt, ap);
  va_end(ap);
  assert(str_l >= 0);           /* possible integer overflow if str_m > INT_MAX */
  if ((size_t) str_l + 1 < str_m)
    str_m = (size_t) str_l + 1; /* truncate */
  /* if str_m is 0, no buffer is allocated, just set *ptr to NULL */
  if (str_m == 0) {             /* not interested in resulting string, just return size */
  } else {
    *ptr = (char *) xbt_malloc(str_m);
    if (*ptr == NULL) {
      errno = ENOMEM;
      str_l = -1;
    } else {
      int str_l2;
      va_start(ap, fmt);
      str_l2 = portable_vsnprintf(*ptr, str_m, fmt, ap);
      va_end(ap);
      assert(str_l2 == str_l);
    }
  }
  return str_l;
}
#endif

#if defined(NEED_VASNPRINTF)
int vasnprintf(char **ptr, size_t str_m, const char *fmt, va_list ap)
{
  int str_l;

  *ptr = NULL;
  {
    va_list ap2;
    va_copy(ap2, ap);           /* don't consume the original ap, we'll need it again */
    str_l = portable_vsnprintf(NULL, (size_t) 0, fmt, ap2);     /*get required size */
    va_end(ap2);
  }
  assert(str_l >= 0);           /* possible integer overflow if str_m > INT_MAX */
  if ((size_t) str_l + 1 < str_m)
    str_m = (size_t) str_l + 1; /* truncate */
  /* if str_m is 0, no buffer is allocated, just set *ptr to NULL */
  if (str_m == 0) {             /* not interested in resulting string, just return size */
  } else {
    *ptr = (char *) xbt_malloc(str_m);
    if (*ptr == NULL) {
      errno = ENOMEM;
      str_l = -1;
    } else {
      int str_l2 = portable_vsnprintf(*ptr, str_m, fmt, ap);
      assert(str_l2 == str_l);
    }
  }
  return str_l;
}
#endif



char *bvprintf(const char *fmt, va_list ap)
{
  char *res;

  if (vasprintf(&res, fmt, ap) < 0) {
    /* Do not want to use xbt_die() here, as it uses the logging
     * infrastucture and may fail to allocate memory too. */
    fprintf(stderr, "bprintf: vasprintf failed. Aborting.\n");
    xbt_abort();
  }
  return res;
}

char *bprintf(const char *fmt, ...)
{
  va_list ap;
  char *res;

  va_start(ap, fmt);
  res = bvprintf(fmt, ap);
  va_end(ap);
  return res;
}
