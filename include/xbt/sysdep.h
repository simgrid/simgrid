/* $Id$ */

/* gras/sysdep.h -- all system dependency                                   */
/*  no system header should be loaded out of this file so that we have only */
/*  one file to check when porting to another OS                            */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */


#ifndef _GRAS_SYSDEP_H
#define _GRAS_SYSDEP_H

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
  
#define gras_strdup(s)  ((s)?(strdup(s)?:(gras_die("memory allocation error"),NULL))\
			    :(NULL))
#define gras_malloc(n)   (malloc(n) ?: (gras_die("memory allocation error"),NULL))
#define gras_malloc0(n)  (calloc( (n),1 ) ?: (gras_die("memory allocation error"),NULL))
#define gras_realloc(p,s) (s? (p? (realloc(p,s)?:gras_die("memory allocation error"),NULL) \
				: gras_malloc(s)) \
			    : (p? (free(p),NULL) \
			        : NULL))
#define gras_free free /*nothing specific to do here. A poor valgrind replacement?*/
#define gras_free_fct free /* replacement with the guareenty of being a function */
   
#define gras_new(type, count)  ((type*)gras_malloc (sizeof (type) * (count)))
#define gras_new0(type, count) ((type*)gras_malloc0 (sizeof (type) * (count)))

/* Attributes are only in recent versions of GCC */

#if     __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
# define _GRAS_GNUC_PRINTF( format_idx, arg_idx )    \
	   __attribute__((__format__ (__printf__, format_idx, arg_idx)))
# define _GRAS_GNUC_SCANF( format_idx, arg_idx )     \
	       __attribute__((__format__ (__scanf__, format_idx, arg_idx)))
# define _GRAS_GNUC_FORMAT( arg_idx )                \
		   __attribute__((__format_arg__ (arg_idx)))
# define _GRAS_GNUC_NORETURN __attribute__((__noreturn__))

#else   /* !__GNUC__ */
# define _GRAS_GNUC_PRINTF( format_idx, arg_idx )
# define _GRAS_GNUC_SCANF( format_idx, arg_idx )
# define _GRAS_GNUC_FORMAT( arg_idx )
# define _GRAS_GNUC_NORETURN

#endif  /* !__GNUC__ */

/* inline and __FUNCTION__ are only in GCC when -ansi is of */

#if defined(__GNUC__) && ! defined(__STRICT_ANSI__)

# define _GRAS_GNUC_FUNCTION __FUNCTION__
# define _GRAS_INLINE inline
#else
# define _GRAS_GNUC_FUNCTION "function"
# define _GRAS_INLINE 
#endif


void gras_abort(void) _GRAS_GNUC_NORETURN;

/* FIXME: This is a very good candidate to rewrite (along with a proper string stuff) 
   but I'm too lazy right now, so copy the definition */
long int strtol(const char *nptr, char **endptr, int base);
double strtod(const char *nptr, char **endptr);
int atoi(const char *nptr);

END_DECL   

#endif /* _GRAS_SYSDEP_H */
