/* Copyright (c) 2005-2010, 2012-2016. The SimGrid Team.
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
 * [...]
 *
 * Routines asprintf and vasprintf return a pointer (in the ptr argument)
 * to a buffer sufficiently large to hold the resulting string. This pointer
 * should be passed to free(3) to release the allocated storage when it is
 * no longer needed. If sufficient space cannot be allocated, these functions
 * will return -1 and set ptr to be a NULL pointer. These two routines are a
 * GNU C library extensions (glibc).
 *
 * AVAILABILITY
 *   http://www.ijs.si/software/snprintf/
 */

#include "xbt/sysdep.h"           /* xbt_abort() */
#include "src/internal_config.h"  /* Do we need vasprintf? */
#include <stdio.h>
#include <assert.h>

#if !HAVE_VASPRINTF
#include <stdarg.h> /* vsnprintf */
int vasprintf(char **ptr, const char *fmt, va_list ap);
int vasprintf(char **ptr, const char *fmt, va_list ap)
{
  size_t str_m;
  int str_l;

  *ptr = NULL;
  {
    va_list ap2;
    va_copy(ap2, ap);           /* don't consume the original ap, we'll need it again */
    str_l = vsnprintf(NULL, (size_t) 0, fmt, ap2);     /*get required size */
    va_end(ap2);
  }
  xbt_assert(str_l >= 0);           /* possible integer overflow if str_m > INT_MAX */
  *ptr = (char *) xbt_malloc(str_m = (size_t) str_l + 1);

  int str_l2 = vsnprintf(*ptr, str_m, fmt, ap);
  assert(str_l2 == str_l);

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

  va_start(ap, fmt);
  char *res = bvprintf(fmt, ap);
  va_end(ap);
  return res;
}
