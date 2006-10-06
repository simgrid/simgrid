/* $Id$ */

/* portable -- header loading to write portable code                         */
/* loads much more stuff than sysdep.h since the latter is in public interface*/

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                   */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef GRAS_PORTABLE_H
#define GRAS_PORTABLE_H

/* 
 * win32 or win64 (__WIN32 is defined for win32 and win64 applications, __TOS_WIN__ is defined by xlC).	
*/ 
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(__TOS_WIN__)
# include "win32/config.h"
#  include <windows.h>
#else
#  include "gras_config.h"
#endif

#include <stdarg.h>

#ifdef HAVE_ERRNO_H
#  include <errno.h>
#endif

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

/****
 **** Networking 
 ****/


#ifdef HAVE_SYS_SOCKET_H
#  include <sys/socket.h>
#  include <netinet/in.h>   /* sometimes required for #include <arpa/inet.h> */
#  include <netinet/tcp.h>  /* TCP_NODELAY */
#  include <netdb.h>        /* getprotobyname() */
#  include <arpa/inet.h>    /* inet_ntoa() */
#  include <sys/types.h>    /* sometimes required for fd_set */
# endif


#ifndef HAVE_WINSOCK_H
#       define tcp_read( s, buf, len)   read( s, buf, len )
#       define tcp_write( s, buf, len)  write( s, buf, len )
#       define sock_errno        errno
#       define sock_errstr(err)  strerror(err)

#       ifdef SHUT_RDWR
#               define tcp_close( s )   (shutdown( s, SHUT_RDWR ), close( s ))
#       else
#               define tcp_close( s )   close( s )
#       endif
#endif /* windows or unix ? */

/****
 **** File handling
 ****/

#include <fcntl.h>

#ifdef HAVE_SYS_STAT_H
#  include <sys/stat.h>
#endif

#ifndef O_BINARY
#  define O_BINARY 0
#endif

/****
 **** Time handling
 ****/

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#ifdef _WIN32
#define sleep _sleep /* else defined in stdlib.h */
#endif

/****
 **** Contexts
 ****/

#ifdef USE_UCONTEXT
# include <ucontext.h>
#endif

#ifdef _WIN32
#  include "xbt/context_win32.h" /* Manual reimplementation for prehistoric platforms */
#endif

/****
 **** Signals
 ****/
#ifdef HAVE_SIGNAL_H
# include <signal.h>
#endif

/****
 **** string handling (parts from http://www.ijs.si/software/snprintf/)
 ****/

/* prototype of C99 functions */
#ifdef HAVE_SNPRINTF
#include <stdio.h>
#else
extern int snprintf(char *, size_t, const char *, /*args*/ ...);
extern int vsnprintf(char *, size_t, const char *, va_list);
#endif

/* use internal functions when OS provided ones are borken */
#if defined(HAVE_SNPRINTF) && defined(PREFER_PORTABLE_SNPRINTF)
extern int portable_snprintf(char *str, size_t str_m, const char *fmt, /*args*/ ...);
extern int portable_vsnprintf(char *str, size_t str_m, const char *fmt, va_list ap);
#define snprintf  portable_snprintf
#define vsnprintf portable_vsnprintf
#endif

/* prototype of GNU functions  */
extern int asprintf  (char **ptr, const char *fmt, /*args*/ ...);
extern int vasprintf (char **ptr, const char *fmt, va_list ap);
extern int asnprintf (char **ptr, size_t str_m, const char *fmt, /*args*/ ...);
extern int vasnprintf(char **ptr, size_t str_m, const char *fmt, va_list ap);

/*
 * That's needed to protect solaris's printf from ever seing NULL associated to a %s format
 * (without adding an extra check on working platforms :)
 */

#ifdef PRINTF_NULL_WORKING
#  define PRINTF_STR(a) (a)
#else
#  define PRINTF_STR(a) (a)?:"(null)"
#endif

/*
 * What we need to extract the backtrace in exception handling code
 */
#ifdef HAVE_EXECINFO_H
#  include <execinfo.h>
#endif

/****
 **** Some debugging functions. Can't we find a better place for this??
 ****/
void hexa_print(const char*name, unsigned char *data, int size);
const char *hexa_str(unsigned char *data, int size, int downside);

/* Windows __declspec(). */
#if defined (_XBT_USE_DECLSPEC) /* using export/import technique */

#    ifndef _XBT_EXPORT_DECLSPEC
#        define _XBT_EXPORT_DECLSPEC
#    endif

#    ifndef _XBT_IMPORT_DECLSPEC
#        define _XBT_IMPORT_DECLSPEC
#    endif

#    if defined (_XBT_DESIGNATED_DLL) /* this is a lib which will contain xbt exports */
#        define  _XBT_DECLSPEC        _XBT_EXPORT_DECLSPEC 
#    else
#        define  _XBT_DECLSPEC        _XBT_IMPORT_DECLSPEC   /* other modules, importing xbt exports */
#    endif

#else /* not using DLL export/import specifications */

#    define _XBT_DECLSPEC

#endif /* #if defined (_XBT_USE_DECLSPEC) */

#if !defined (_XBT_CALL)
#define _XBT_CALL
#endif

#endif /* GRAS_PORTABLE_H */
