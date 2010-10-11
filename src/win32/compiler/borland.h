#ifndef __XBT_BORLAND_COMPILER_CONFIG_H__
#define __XBT_BORLAND_COMPILER_CONFIG_H__

/* borland.h - simgrid config for Borland C++ Builder   */

/* Copyright (c) 2006, 2007, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/*
 *  Borland C++ compiler configuration
 */

#include <win32/platform/select_platform_features.h>

/* 
 * include files. 
 */

/* No <dlfcn.h> header file. */
#if defined(HAVE_DLFCN_H)
#undef HAVE_DLFCN_H
#endif

/* Defined if the compiler has the <errno.h> header file. */
#if !defined(HAVE_ERRNO_H)
#define HAVE_ERRNO_H 		1
#endif

/* No <execinfo.h> header file. */
#if defined(HAVE_EXECINFO_H)
#undef HAVE_EXECINFO_H
#endif

/* No <inttypes.h> header file. */
#if defined(HAVE_INTTYPES_H)
#undef HAVE_INTTYPES_H
#endif

/* Defined if compiler has the <memory.h> header file. */
#if !defined(HAVE_MEMORY_H)
#define HAVE_MEMORY_H 		1
#endif

/* No <pthread.h> header file. */
#if defined(HAVE_PTHREAD_H)
#undef HAVE_PTHREAD_H
#endif


/* No <stdint.h> header file. */
#if defined(HAVE_STDINT_H)
#undef HAVE_STDINT_H
#endif

/* The compiler has the <stdlib.h> header file. */
#if !defined(HAVE_STDLIB_H)
#define HAVE_STDLIB_H		1
#endif

/* No <strings.h> header file. */
#if defined(HAVE_STRINGS_H)
#undef HAVE_STRINGS_H
#endif

/* The compiler has the <string.h> header file. */
#if !defined(HAVE_STRING_H)
#define HAVE_STRING_H		1
#endif

/* No <sys/socket.h> header file. */
#if defined(HAVE_SYS_SOCKET_H)
#undef HAVE_SYS_SOCKET_H
#endif

/* The compiler has <sys/stat.h> header file. */
#if !defined(HAVE_SYS_STAT_H)
#define HAVE_SYS_STAT_H		1
#endif

/* No <sys/time.h> header file. */
#if defined(HAVE_SYS_TIME_H)
#undef HAVE_SYS_TIME_H		1
#endif

/* The compiler has the <sys/types.h> header file. */
#if !defined(HAVE_SYS_TYPES_H)
#define HAVE_SYS_TYPES_H	1
#endif

/* No <unistd.h> header file. */
#if defined(HAVE_UNISTD_H)
#undef HAVE_UNISTD_H
#endif

/* 
 * The compiler has the <windows.h> header file. 
 * Process the case of afx.h
*/
#if !defined(HAVE_WINDOWS_H)
#define HAVE_WINDOWS_H		1
#endif

/* The compiler has the <winsock2.h> header file. */
#if !defined(HAVE_WINSOCK2_H)
#define HAVE_WINSOCK2_H
#endif

/*  
 * The compiler has the <winsock.h> header file.
 * Trouble if winsock2.h exists ?  
 */
#if !defined(HAVE_WINSOCK_H)
#define HAVE_WINSOCK_H 	1
#endif

/* The compiler has the <signal.h> header file */
#if !defined(HAVE_SIGNAL_H)
#define HAVE_SIGNAL_H	1
#endif

/* 
 * functions.
 */

/* No `getcontext' function. */
#if defined(HAVE_GETCONTEXT)
#undef HAVE_GETCONTEXT
#endif

/* No `getdtablesize' function. */
#if defined(HAVE_GETDTABLESIZE)
#undef HAVE_GETDTABLESIZE
#endif

/* No `gettimeofday' function. */
#if defined(HAVE_GETTIMEOFDAY)
#undef HAVE_GETTIMEOFDAY
#endif

/* No `makecontext' function. */
#if defined(HAVE_MAKECONTEXT)
#undef HAVE_MAKECONTEXT
#endif

/* No 'popen' function. */
#if defined(HAVE_POPEN)
#undef HAVE_POPEN
#endif

/* No `readv' function. */
#if defined(HAVE_READV)
#undef HAVE_READV
#endif

/* No `setcontext' function. */
#if defined(HAVE_SETCONTEXT)
#undef HAVE_SETCONTEXT
#endif

/* No 'signal' function */
#if defined(HAVE_SIGNAL)
#undef HAVE_SIGNAL
#endif

/* The compiler has `snprintf' function. */
#if !defined(HAVE_SNPRINTF)
#define HAVE_SNPRINTF	1
#endif

/* No `swapcontext' function. */
#if defined(HAVE_SWAPCONTEXT)
#undef HAVE_SWAPCONTEXT
#endif

/* No `sysconf' function. */
#if defined(HAVE_SYSCONF)
#undef HAVE_SYSCONF
#endif

/* No `usleep' function. */
#if defined(HAVE_USLEEP)
#undef HAVE_USLEEP
#endif

/* The compiler has the `vsnprintf' function. */
#if !defined(HAVE_VSNPRINTF)
#define HAVE_VSNPRINTF	1
#endif

/* enable the asprintf replacement */
#if !defined(NEED_ASPRINTF)
#define NEED_ASPRINTF	1
#endif

/*#ifdef NEED_ASPRINTF
#undef NEED_ASPRINTF
#endif*/


/* enable the vasprintf replacement */
#if  !defined(NEED_VASPRINTF)
#define NEED_VASPRINTF	1
#endif

/* "disable the snprintf replacement ( this function is broken on system v only" */

/* FIXME TO ANALYZE */
#if defined(PREFER_PORTABLE_SNPRINTF)
#undef PREFER_PORTABLE_SNPRINTF
#endif

#if !defined(PREFER_PORTABLE_SNPRINTF)
#define PREFER_PORTABLE_SNPRINTF
#endif

/* The maximal size of any scalar on this arch */
#if !defined(SIZEOF_MAX)
#define SIZEOF_MAX 8
#endif

/* Define to 1 if you have the ANSI C header files. */
#if !defined(STDC_HEADERS)
#define STDC_HEADERS 1
#endif

#if defined(TIME_WITH_SYS_TIME)
#undef TIME_WITH_SYS_TIME
#endif

/* 
 * libraries
 */

/* Define to 1 if you have the `pthread' library (-lpthread). */
#if defined(HAVE_LIBPTHREAD)
#undef HAVE_LIBPTHREAD
#endif

/* 
 * package informations ?
 */


/* Defined if arrays in struct can straddle struct alignment boundaries. This
is like than the structure compaction above, but this time, the argument to
be compacted is an array whom each element would be normally compacted.
Exemple: struct s { double d; int i; char c[6]; }; Arrays can straddle if c
is allowed to come just after i. Note that GRAS only support architecture
presenting this caracteristic so far. */

#if defined(GRAS_ARRAY_STRADDLE_STRUCT)
#undef GRAS_ARRAY_STRADDLE_STRUCT
#endif

/* Defined if structures are compacted when possible. Consider this structure:
struct s {double d; int i; char c;}; If it is allowed, the char is placed
just after the int. If not, it has to be on the 8 bytes boundary imposed by
the double. For now, GRAS requires the structures to be compacted. */
#if defined(GRAS_STRUCT_COMPACT)
#undef GRAS_STRUCT_COMPACT
#endif

/* Name of package */
#define PACKAGE "simgrid"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "simgrid-devel@lists.gforge.inria.fr"

/* Define to the full name of this package. */
#define PACKAGE_NAME "simgrid"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "simgrid 3.1.1-cvs"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "simgrid"

/* Define to the version of this package. */
#define PACKAGE_VERSION "3.1.1-cvs"

/* 
 * macro
 */

 /* define if big endian */
#if !defined(GRAS_BIGENDIAN)
#define GRAS_BIGENDIAN 0
#endif

/* Defined if structures are compacted when possible. Consider this structure:
struct s {double d; int i; char c;}; If it is allowed, the char is placed
just after the int. If not, it has to be on the 8 bytes boundary imposed by
the double. For now, GRAS requires the structures to be compacted. */
#if defined(GRAS_STRUCT_COMPACT)
#define GRAS_STRUCT_COMPACT 1
#endif

/* defines the GRAS architecture signature of this machine */
#if defined(GRAS_THISARCH)
#undef GRAS_THISARCH
#endif

#define GRAS_THISARCH 0


 /* Path to the addr2line tool */
#if defined(ADDR2LINE)
#undef ADDR2LINE
#endif

#if !defined(HAVE_VA_COPY)
#define HAVE_VA_COPY 1
#endif

/* This macro is not defined in borland stdarg.h include file, adds it. */
#define va_copy(dest,src)   ((dest)=(src))

/* Predefined possible va_copy() implementation (id: ASP) */
#define __VA_COPY_USE_ASP(d, s) do { *(d) = *(s); } while (0)

/* Predefined possible va_copy() implementation (id: ASS) */
#define __VA_COPY_USE_ASS(d, s) do { (d) = (s); } while (0)

/* Predefined possible va_copy() implementation (id: C99) */
#define __VA_COPY_USE_C99(d, s) va_copy((d), (s))

/* Predefined possible va_copy() implementation (id: CPP) */
#define __VA_COPY_USE_CPP(d, s) memcpy((void *)(d), (void *)(s), sizeof(*(s)))

/* Predefined possible va_copy() implementation (id: CPS) */
#define __VA_COPY_USE_CPS(d, s) memcpy((void *)&(d), (void *)&(s), sizeof(s))

/* Predefined possible va_copy() implementation (id: GCB) */
#define __VA_COPY_USE_GCB(d, s) __builtin_va_copy((d), (s))

/* Predefined possible va_copy() implementation (id: GCH) */
#define __VA_COPY_USE_GCH(d, s) __va_copy((d), (s))

/* Predefined possible va_copy() implementation (id: GCM) */
#define __VA_COPY_USE_GCM(d, s) VA_COPY((d), (s))


/* Optional va_copy() implementation activation */
#ifndef HAVE_VA_COPY
#define va_copy(d, s) __VA_COPY_USE(d, s)
#endif


/* Define to id of used va_copy() implementation */
#define __VA_COPY_USE __VA_COPY_USE_C99

#ifndef _XBT_CALL
#if defined(_XBT_DESIGNATED_DLL)
#define _XBT_CALL __cdecl __export
#elif defined(_RTLDLL)
#define  _XBT_CALL __cdecl __import
#else
#define  _XBT_CALL __cdecl
#endif
#endif

/* auto enable thread safety and exceptions: */
#ifndef _CPPUNWIND
#define _XBT_HAS_NO_EXCEPTIONS
#endif

#if defined ( __MT__ ) && !defined (_NOTHREADS) && !defined (_REENTRANT)
#define _REENTRANT 1
#endif

#if(__BORLANDC__>= 0x500)
#define _XBT_HAS_NAMESPACES
#endif


/* For open, read etc. file operations. */
#include <io.h>
#include <fcntl.h>

/* For getpid() function. */
#include <process.h>

/* no unistd.h header file. */
#define YY_NO_UNISTD_H
/*
 * Replace winsock2.h,ws2tcpip.h and winsock.h header files */
#include <windows.h>

/* types */
typedef unsigned int uint32_t;

/* Choose setjmp as exception implementation */
#ifndef __EX_MCTX_SJLJ__
#define __EX_MCTX_SJLJ__
#endif

/* this is used in context managment. */
#ifdef CONTEXT_UCONTEXT
#undef CONTEXT_UCONTEXT
#endif

#ifndef CONTEXT_THREADS
#define CONTEXT_THREADS 1
#endif




#endif                          /* #ifndef __XBT_BORLAND_COMPILER_CONFIG_H__ */
