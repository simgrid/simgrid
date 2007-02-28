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

/* 
 * Function calling convention (not used for now) 
 */
 
#ifdef _WIN32
#  ifndef _XBT_CALL
#    define _XBT_CALL __cdecl
#   endif
#else 
#  define _XBT_CALL
#endif

/* Handle import/export stuff
 * 
 * Rational of XBT_PUBLIC: 
 *   * If you build the DLL you must pass the right value of XBT_PUBLIC in the project : to do this you must define the DLL_EXPORT macro
 *   * If you do a static compilation, you must define the macro DLL_STATIC
 *   * If you link your code against the DLL, this file defines the macro to '__declspec(dllimport)' for you
 *   * If you compile under unix, this file defines the macro to 'extern', even if it's not mandatory with modern compilers
 * 
 * Rational of XBT_PUBLIC_NO_IMPORT:
 *   * This is for symbols which must be exported in the DLL, but not imported from it. 
 *     This is obviously useful for initialized globals (which cannot be extern or similar).
 *     This is also used in the log mecanism where a macro creates the variable automatically.
 *      When the macro is called from within SimGrid, the symbol must be exported, but when called 
 *      from within the client code, it must not try to retrieve the symbol from the DLL since it's not in there.
 */


#ifdef DLL_EXPORT
#  define XBT_PUBLIC(type)			  __declspec(dllexport) type
#  define XBT_PUBLIC_NO_IMPORT(type)  __declspec(dllexport) type
#  define XBT_IMPORT_NO_PUBLIC(type)  type 
#else
#  ifdef DLL_STATIC
#    define XBT_PUBLIC(type)		   type
#    define XBT_PUBLIC_NO_IMPORT(type) type
#   define XBT_IMPORT_NO_PUBLIC(type)  type 
#  else
#    ifdef _WIN32
#      define XBT_PUBLIC(type)		     __declspec(dllimport) type
#      define XBT_PUBLIC_NO_IMPORT(type) type
#      define XBT_IMPORT_NO_PUBLIC(type) __declspec(dllimport) type
#  	 else
#      define XBT_PUBLIC(type)		     extern type
#      define XBT_PUBLIC_NO_IMPORT(type) type
#      define XBT_IMPORT_NO_PUBLIC(type) type
#    endif
#  endif
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

XBT_PUBLIC(const char *)xbt_procname(void);

#define XBT_BACKTRACE_SIZE 10 /* FIXME: better place? Do document */
   
SG_END_DECL()

#endif /* XBT_MISC_H */




