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
# else
#    define XBT_INLINE  inline
#endif

/* Windows __declspec(). */
#if defined(_WIN32) && defined(__BORLANDC__)
#  if(__BORLANDC__ < 0x540)
#    if (defined (__DLL) || defined (_DLL) || defined (_WINDLL) || defined (_RTLDLL) || defined (_XBT_USE_DYNAMIC_LIB) ) && ! defined (_XBT_USE_STAT
#      undef  _XBT_USE_DECLSPEC
#      define _XBT_USE_DECLSPEC
#    endif
#  else
#  if ( defined (__DLL) || defined (_DLL) || defined (_WINDLL) || defined (_RTLDLL) || defined(_AFXDLL) || defined (_XBT_USE_DYNAMIC_LIB) )
#    undef  _XBT_USE_DECLSPEC
#    define _XBT_USE_DECLSPEC 1
#  endif
#  endif
#endif

#if defined (_XBT_USE_DECLSPEC) /* using export/import technique */

#    ifndef _XBT_EXPORT_DECLSPEC
#        define _XBT_EXPORT_DECLSPEC
#    endif

#    ifndef _XBT_IMPORT_DECLSPEC
#        define _XBT_IMPORT_DECLSPEC
#    endif

#    if defined (_XBT_DESIGNATED_DLL) /* this is a lib which will contain xbt exports */
#        define  XBT_PUBLIC        _XBT_EXPORT_DECLSPEC
#    else
#        define  XBT_PUBLIC        _XBT_IMPORT_DECLSPEC   /* other modules, importing xbt exports */
#    endif

#else /* not using DLL export/import specifications */

#    define XBT_PUBLIC

#endif /* #if defined (_XBT_USE_DECLSPEC) */

/* Function calling convention (not used for now) */
#if !defined (_XBT_CALL)
#define _XBT_CALL
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

XBT_PUBLIC const char *xbt_procname(void);

#define XBT_BACKTRACE_SIZE 10 /* FIXME: better place? Do document */
   
SG_END_DECL()

#endif /* XBT_MISC_H */
