/* xbt.h - Public interface to the xbt (simgrid's toolbox)                     */

/* Copyright (c) 2004-2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_MISC_H
#define XBT_MISC_H

/* Define _GNU_SOURCE for getline, isfinite, etc. */
#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
#endif

/* Attributes are only in recent versions of GCC */
#if defined(__GNUC__) && (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4))
# define _XBT_GNUC_PRINTF( format_idx, arg_idx )    \
     __attribute__((__format__ (__printf__, format_idx, arg_idx)))
# define _XBT_GNUC_SCANF( format_idx, arg_idx )     \
         __attribute__((__format__ (__scanf__, format_idx, arg_idx)))
# define _XBT_GNUC_NORETURN __attribute__((__noreturn__))
# define _XBT_GNUC_UNUSED  __attribute__((__unused__))
/* Constructor priorities exist since gcc 4.3.  Apparently, they are however not
 * supported on Macs. */
# if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)) && !defined(__APPLE__)
#  define _XBT_GNUC_CONSTRUCTOR(prio) __attribute__((__constructor__ (prio)))
#  define _XBT_GNUC_DESTRUCTOR(prio) __attribute__((__destructor__ (prio)))
# else
#  define _XBT_GNUC_CONSTRUCTOR(prio) __attribute__((__constructor__))
#  define _XBT_GNUC_DESTRUCTOR(prio) __attribute__((__destructor__))
# endif
# undef _XBT_NEED_INIT_PRAGMA

#else                           /* !__GNUC__ */
# define _XBT_GNUC_PRINTF( format_idx, arg_idx )
# define _XBT_GNUC_SCANF( format_idx, arg_idx )
# define _XBT_GNUC_NORETURN
# define _XBT_GNUC_UNUSED
# define _XBT_GNUC_CONSTRUCTOR(prio)
# define _XBT_GNUC_DESTRUCTOR(prio)
# define  _XBT_NEED_INIT_PRAGMA 1

#endif                          /* !__GNUC__ */

/* inline and __FUNCTION__ are only in GCC when -ansi is off */

#if defined(__GNUC__) && ! defined(__STRICT_ANSI__)
# define _XBT_FUNCTION __FUNCTION__
#elif (defined(__STDC__) && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L)
# define _XBT_FUNCTION __func__ /* ISO-C99 compliant */
#else
# define _XBT_FUNCTION "function"
#endif

#ifdef DOXYGEN
#  define XBT_INLINE
#else
#  ifndef __cplusplus
#    if defined(__GNUC__) && ! defined(__STRICT_ANSI__)
#        define XBT_INLINE inline
#    elif (defined(__STDC__) && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L)
#        define XBT_INLINE inline
#    elif defined(__BORLANDC__) && !defined(__STRICT_ANSI__)
#        define XBT_INLINE __inline
#    else
#        define XBT_INLINE
#    endif
#  else
#     if defined (__VISUALC__)
#       define XBT_INLINE __inline
#     else
#       define XBT_INLINE  inline
#     endif
#  endif /* __cplusplus */
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
 * Expands to `one' if there is only one argument for the variadic part.
 * Otherwise, expands to `more'.
 * Works with up to 63 arguments, which is the maximum mandated by the C99
 * standard.
 */
#define _XBT_IF_ONE_ARG(one, more, ...)                                 \
    _XBT_IF_ONE_ARG_(__VA_ARGS__,                                       \
                     more, more, more, more, more, more, more, more,    \
                     more, more, more, more, more, more, more, more,    \
                     more, more, more, more, more, more, more, more,    \
                     more, more, more, more, more, more, more, more,    \
                     more, more, more, more, more, more, more, more,    \
                     more, more, more, more, more, more, more, more,    \
                     more, more, more, more, more, more, more, more,    \
                     more, more, more, more, more, more, more, one)
#define _XBT_IF_ONE_ARG_(a64, a63, a62, a61, a60, a59, a58, a57,        \
                         a56, a55, a54, a53, a52, a51, a50, a49,        \
                         a48, a47, a46, a45, a44, a43, a42, a41,        \
                         a40, a39, a38, a37, a36, a35, a34, a33,        \
                         a32, a31, a30, a29, a28, a27, a26, a25,        \
                         a24, a23, a22, a21, a20, a19, a18, a17,        \
                         a16, a15, a14, a13, a12, a11, a10, a9,         \
                         a8, a7, a6, a5, a4, a3, a2, a1, N, ...) N

/* 
 * Function calling convention (not used for now) 
 */

#ifdef _XBT_WIN32
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
#  define XBT_PUBLIC(type)            extern __declspec(dllexport) type
#  define XBT_EXPORT_NO_IMPORT(type)  __declspec(dllexport) type
#  define XBT_IMPORT_NO_EXPORT(type)  type
#  define XBT_PUBLIC_DATA(type)       extern __declspec(dllexport) type

/* Pack everything up statically */
#elif defined(DLL_STATIC)
#  define XBT_PUBLIC(type)            extern type
#  define XBT_EXPORT_NO_IMPORT(type)  type
#  define XBT_IMPORT_NO_EXPORT(type)  type
#  define XBT_PUBLIC_DATA(type)       extern type

/* Link against the DLL */
#elif (defined(_XBT_WIN32) && !defined(DLL_EXPORT) && !defined(DLL_STATIC))
#  define XBT_PUBLIC(type)            extern __declspec(dllimport) type
#  define XBT_EXPORT_NO_IMPORT(type)  type
#  define XBT_IMPORT_NO_EXPORT(type)  __declspec(dllimport) type
#  define XBT_PUBLIC_DATA(type)       extern __declspec(dllimport) type

/* UNIX build */
#else
#  define XBT_PUBLIC(type)            extern type
#  define XBT_EXPORT_NO_IMPORT(type)  type
#  define XBT_IMPORT_NO_EXPORT(type)  type
#  define XBT_PUBLIC_DATA(type)       extern type
#endif

#if !defined (max) && !defined(__cplusplus)
#  define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif
#if !defined (min) && !defined(__cplusplus)
#  define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif

#define TRUE  1
#define FALSE 0

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
#endif                          /* XBT_MISC_H */
