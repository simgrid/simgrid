/* $Id$ */

/* xbt.h - Public interface to the xbt (gras's toolbox)                     */

/* Copyright (c) 2004 Martin Quinson.                                       */
/* Copyright (c) 2004 Arnaud Legrand.                                       */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_MISC_H
#define XBT_MISC_H

/* Attributes are only in recent versions of GCC */
#if     __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
# define _XBT_GNUC_PRINTF( format_idx, arg_idx )    \
	   __attribute__((__format__ (__printf__, format_idx, arg_idx)))
# define _XBT_GNUC_SCANF( format_idx, arg_idx )     \
	       __attribute__((__format__ (__scanf__, format_idx, arg_idx)))
# define _XBT_GNUC_FORMAT( arg_idx )                \
		   __attribute__((__format_arg__ (arg_idx)))
# define _XBT_GNUC_NORETURN __attribute__((__noreturn__))
# define _XBT_GNUC_UNUSED  __attribute__((unused))

#else   /* !__GNUC__ */
# define _XBT_GNUC_PRINTF( format_idx, arg_idx )
# define _XBT_GNUC_SCANF( format_idx, arg_idx )
# define _XBT_GNUC_FORMAT( arg_idx )
# define _XBT_GNUC_NORETURN
# define _XBT_GNUC_UNUSED

#endif  /* !__GNUC__ */

/* inline and __FUNCTION__ are only in GCC when -ansi is off */

#if defined(__GNUC__) && ! defined(__STRICT_ANSI__)
# define _XBT_FUNCTION __FUNCTION__
#elif (defined(__STDC__) && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L)
# define _XBT_FUNC__ __func__      /* ISO-C99 compliant */
#else
# define _XBT_FUNCTION "function"
#endif

#ifndef __cplusplus
#    if defined(__GNUC__) && ! defined(__STRICT_ANSI__)
#        define XBT_INLINE inline
#    elif (defined(__STDC__) && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L)
#        define XBT_INLINE inline
#    elif defined(__BORLANDC__) && !defined(__STRICT_ANSI__)
#        define XBT_INLINE __inline
#    else
#        define XBT_INLINE
#    endif
#endif


#ifndef max
#  define max(a, b) (((a) > (b))?(a):(b))
#endif
#ifndef min
#  define min(a, b) (((a) < (b))?(a):(b))
#endif

#define TRUE  1
#define FALSE 0

#define XBT_MAX_CHANNEL 10 /* FIXME: killme */
/*! C++ users need love */
#ifndef SG_BEGIN_DECL
# ifdef __cplusplus
#  define SG_BEGIN_DECL() extern "C" {
# else
#  define SG_BEGIN_DECL() 
# endif
#endif

/*! C++ users need love */
#ifndef SG_END_DECL
# ifdef __cplusplus
#  define SG_END_DECL() }
# else
#  define SG_END_DECL() 
# endif
#endif
/* End of cruft for C++ */

SG_BEGIN_DECL()

const char *xbt_procname(void);

#define XBT_BACKTRACE_SIZE 10 /* FIXME: better place? Do document */
   
SG_END_DECL()

#endif /* XBT_MISC_H */
