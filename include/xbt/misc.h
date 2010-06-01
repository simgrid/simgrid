/* xbt.h - Public interface to the xbt (gras's toolbox)                     */
/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras_config.h"

#ifndef XBT_MISC_H
#define XBT_MISC_H

/* Attributes are only in recent versions of GCC */
#if defined(__GNUC__) && (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4))
# define _XBT_GNUC_PRINTF( format_idx, arg_idx )    \
	   __attribute__((__format__ (__printf__, format_idx, arg_idx)))
# define _XBT_GNUC_SCANF( format_idx, arg_idx )     \
	       __attribute__((__format__ (__scanf__, format_idx, arg_idx)))
# define _XBT_GNUC_FORMAT( arg_idx )                \
		   __attribute__((__format_arg__ (arg_idx)))
# define _XBT_GNUC_NORETURN __attribute__((__noreturn__))
# define _XBT_GNUC_UNUSED  __attribute__((unused))
# define _XBT_GNUC_CONSTRUCTOR __attribute__((__constructor__))
# define _XBT_GNUC_DESTRUCTOR __attribute__((__destructor__))
# undef _XBT_NEED_INIT_PRAGMA

#else /* !__GNUC__ */
# define _XBT_GNUC_PRINTF( format_idx, arg_idx )
# define _XBT_GNUC_SCANF( format_idx, arg_idx )
# define _XBT_GNUC_FORMAT( arg_idx )
# define _XBT_GNUC_NORETURN
# define _XBT_GNUC_UNUSED
# define _XBT_GNUC_CONSTRUCTOR
# define _XBT_GNUC_DESTRUCTOR
# define  _XBT_NEED_INIT_PRAGMA 1

#endif /* !__GNUC__ */

/* inline and __FUNCTION__ are only in GCC when -ansi is off */

#if defined(__GNUC__) && ! defined(__STRICT_ANSI__)
# define _XBT_FUNCTION __FUNCTION__
#elif (defined(__STDC__) && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L)
# define _XBT_FUNC__ __func__   /* ISO-C99 compliant */
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

/* improvable on gcc (by evaluating arguments only once), but wouldn't be portable */
#ifdef MIN
# undef MIN
#endif
#define MIN(a,b) ((a)<(b)?(a):(b))

#ifdef MAX
# undef MAX
#endif
#define MAX(a,b) ((a)>(b)?(a):(b))


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
 *   * This is for library symbols visible from the application-land.
 *     Basically, any symbols defined in the include/directory must be 
 *     like this (plus some other globals). 
 *
 *     UNIX coders should just think of it as a special way to say "extern".
 *
 *   * If you build the DLL, define the DLL_EXPORT symbol so that all symbols
 *     actually get exported by this file.

 *   * If you do a static windows compilation, define DLL_STATIC, both when
 *     compiling the application files and when compiling the library.
 *
 *   * If you link your application against the DLL or if you do a UNIX build,
 *     don't do anything special. This file will do the right thing for you 
 *     by default.
 *
 * 
 * Rational of XBT_EXPORT_NO_IMPORT: (windows-only cruft)
 *   * Symbols which must be exported in the DLL, but not imported from it.
 * 
 *   * This is obviously useful for initialized globals (which cannot be 
 *     extern or similar).
 *   * This is also used in the log mecanism where a macro creates the 
 *     variable automatically. When the macro is called from within SimGrid,
 *     the symbol must be exported, but when called  from within the client
 *     code, it must not try to retrieve the symbol from the DLL since it's
 *      not in there.
 * 
 * Rational of XBT_IMPORT_NO_EXPORT: (windows-only cruft)
 *   * Symbols which must be imported from the DLL, but not explicitely 
 *     exported from it.
 * 
 *   * The root log category is already exported, but not imported explicitely 
 *     when creating a subcategory since we cannot import the parent category 
 *     to deal with the fact that the parent may be in application space, not 
 *     DLL space.
 */


/* Build the DLL */
#if defined(DLL_EXPORT)
#  define XBT_PUBLIC(type)            __declspec(dllexport) type
#  define XBT_EXPORT_NO_IMPORT(type)  __declspec(dllexport) type
#  define XBT_IMPORT_NO_EXPORT(type)  type
#  define XBT_PUBLIC_DATA(type)	      __declspec(dllexport) type

/* Pack everything up statically */
#elif defined(DLL_STATIC)
#  define XBT_PUBLIC(type)           extern type
#  define XBT_EXPORT_NO_IMPORT(type)  type
#  define XBT_IMPORT_NO_EXPORT(type)  type
#  define XBT_PUBLIC_DATA(type)       extern type


/* Link against the DLL */
#elif (defined(_WIN32) && !defined(DLL_EXPORT) && !defined(DLL_STATIC))
#  define XBT_PUBLIC(type)            	type
#  define XBT_EXPORT_NO_IMPORT(type)  	type
#  define XBT_IMPORT_NO_EXPORT(type)  	__declspec(dllimport) type
#  define XBT_PUBLIC_DATA(type)		__declspec(dllimport) type

/* UNIX build */
#else
#  define XBT_PUBLIC(type)            extern type
#  define XBT_EXPORT_NO_IMPORT(type)  type
#  define XBT_IMPORT_NO_EXPORT(type)  type
#  define XBT_PUBLIC_DATA(type)       extern type
#endif

#if !defined (max) && !defined(__cplusplus)
#  define max(a,b)	(((a) > (b)) ? (a) : (b))
#endif
#if !defined (min) && !defined(__cplusplus)
#  define min(a,b)	(((a) < (b)) ? (a) : (b))
#endif

#define TRUE  1
#define FALSE 0

#define XBT_MAX_CHANNEL 10      /* FIXME: killme */
/*! C++ users need love */
#ifndef SG_BEGIN_DECL
# ifdef __cplusplus
#  define SG_BEGIN_DECL() extern "C" {
# else
#  define SG_BEGIN_DECL()
# endif
#endif

#ifndef SG_END_DECL
# ifdef __cplusplus
#  define SG_END_DECL() }
# else
#  define SG_END_DECL()
# endif
#endif
/* End of cruft for C++ */

SG_BEGIN_DECL()

XBT_PUBLIC(const char *) xbt_procname(void);

#define XBT_BACKTRACE_SIZE 10   /* FIXME: better place? Do document */

SG_END_DECL()
#endif /* XBT_MISC_H */
