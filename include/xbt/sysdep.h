/* $Id$ */

/*  xbt/sysdep.h -- all system dependency                                   */
/*  no system header should be loaded out of this file so that we have only */
/*  one file to check when porting to another OS                            */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_SYSDEP_H
#define _XBT_SYSDEP_H

#include <string.h> /* Included directly for speed */

#include <time.h> /* FIXME: remove */
#include <unistd.h> /* FIXME: remove */
#include <stdlib.h> 

#include "xbt/misc.h"
BEGIN_DECL
#if 0
#define __CALLOC_OP(n, s)  calloc((n), (s))
  #define __REALLOC_OP(n, s)  realloc((n), (s))
#define CALLOC(n, s)  ((__CALLOC_OP  ((n)?:(FAILURE("attempt to alloc 0 bytes"), 0), (s)?:(FAILURE("attempt to alloc 0 bytes"), 0)))?:(FAILURE("memory allocation error"), NULL))
  /* #define REALLOC(p, s) ((__REALLOC_OP ((p), (s)?:(FAILURE("attempt to alloc 0 bytes"), 0)))?:(FAILURE("memory reallocation error"), NULL)) */
  #define REALLOC(p, s) (__REALLOC_OP ((p), (s)))
#endif
  
#define xbt_strdup(s)  ((s)?(strdup(s)?:(xbt_die("memory allocation error"),NULL))\
			    :(NULL))
#define xbt_malloc(n)   (malloc(n) ?: (xbt_die("memory allocation error"),NULL))
#define xbt_malloc0(n)  (calloc( (n),1 ) ?: (xbt_die("memory allocation error"),NULL))
#define xbt_realloc(p,s) (s? (p? (realloc(p,s)?:(xbt_die("memory allocation error"),NULL)) \
				: xbt_malloc(s)) \
			    : (p? (free(p),NULL) \
			        : NULL))
#define xbt_free free /*nothing specific to do here. A poor valgrind replacement?*/
#define xbt_free_fct free /* replacement with the guareenty of being a function */
   
#define xbt_new(type, count)  ((type*)xbt_malloc (sizeof (type) * (count)))
#define xbt_new0(type, count) ((type*)xbt_malloc0 (sizeof (type) * (count)))

/* Attributes are only in recent versions of GCC */

#if     __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
# define _XBT_GNUC_PRINTF( format_idx, arg_idx )    \
	   __attribute__((__format__ (__printf__, format_idx, arg_idx)))
# define _XBT_GNUC_SCANF( format_idx, arg_idx )     \
	       __attribute__((__format__ (__scanf__, format_idx, arg_idx)))
# define _XBT_GNUC_FORMAT( arg_idx )                \
		   __attribute__((__format_arg__ (arg_idx)))
# define _XBT_GNUC_NORETURN __attribute__((__noreturn__))

#else   /* !__GNUC__ */
# define _XBT_GNUC_PRINTF( format_idx, arg_idx )
# define _XBT_GNUC_SCANF( format_idx, arg_idx )
# define _XBT_GNUC_FORMAT( arg_idx )
# define _XBT_GNUC_NORETURN

#endif  /* !__GNUC__ */

/* inline and __FUNCTION__ are only in GCC when -ansi is of */

#if defined(__GNUC__) && ! defined(__STRICT_ANSI__)

# define _XBT_GNUC_FUNCTION __FUNCTION__
# define _XBT_INLINE inline
#else
# define _XBT_GNUC_FUNCTION "function"
# define _XBT_INLINE 
#endif

END_DECL
   
#include "xbt/error.h" /* needed for xbt_die */

#endif /* _XBT_SYSDEP_H */
