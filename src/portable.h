/* portable -- header loading to write portable code                         */
/* loads much more stuff than sysdep.h since the latter is in public interface*/

/* Copyright (c) 2004-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_PORTABLE_H
#define SIMGRID_PORTABLE_H

#include "internal_config.h"
#include "xbt/base.h"
#include "xbt/misc.h"
#ifdef _XBT_WIN32
# include <windows.h>
#endif

#include <stdarg.h>
#include <stdio.h>

#ifdef HAVE_ERRNO_H
#  include <errno.h>
#endif

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_SYS_SYSCTL_H
# include <sys/sysctl.h>
#endif

/****
 **** File handling
 ****/

#include <fcntl.h>

#ifdef _XBT_WIN32
  #ifndef EWOULDBLOCK
  #define EWOULDBLOCK WSAEWOULDBLOCK
  #endif

  #ifndef EINPROGRESS
  #define EINPROGRESS WSAEINPROGRESS
  #endif

  #ifndef ETIMEDOUT
  #define ETIMEDOUT   WSAETIMEDOUT
  #endif

  #ifdef S_IRGRP
    #undef S_IRGRP
  #endif
  #define S_IRGRP 0

  #ifdef S_IWGRP
    #undef S_IWGRP
  #endif
  #define S_IWGRP 0
#endif

#ifdef HAVE_SYS_STAT_H
#  include <sys/stat.h>
#endif

#ifndef O_BINARY
#  define O_BINARY 0
#endif

/****
 **** Time handling
 ****/

#if HAVE_SYS_TIME_H
#  include <sys/time.h>
#endif
#include <time.h>

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
#if defined(HAVE_SNPRINTF)
#include <stdio.h>
#else
XBT_PUBLIC(int) snprintf(char *, size_t, const char *, /*args */ ...);
XBT_PUBLIC(int) vsnprintf(char *, size_t, const char *, va_list);
#endif


/* use internal functions when OS provided ones are borken */
#if defined(HAVE_SNPRINTF) && defined(PREFER_PORTABLE_SNPRINTF)
XBT_PRIVATE int portable_snprintf(char *str, size_t str_m, const char *fmt,
                             /*args */ ...);
XBT_PRIVATE int portable_vsnprintf(char *str, size_t str_m, const char *fmt,
                              va_list ap);
#define snprintf  portable_snprintf
#define vsnprintf portable_vsnprintf
#endif

/* prototype of GNU functions  */
#if (defined(__GNUC__) && !defined(__cplusplus))
XBT_PUBLIC(int) asprintf(char **ptr, const char *fmt, /*args */ ...);
XBT_PUBLIC(int) vasprintf(char **ptr, const char *fmt, va_list ap);
#endif

extern int asnprintf(char **ptr, size_t str_m, const char *fmt, /*args */
                     ...);
extern int vasnprintf(char **ptr, size_t str_m, const char *fmt,
                      va_list ap);

/*
 * What we need to extract the backtrace in exception handling code
 */
#ifdef HAVE_EXECINFO_H
#  include <execinfo.h>
#endif

#endif                          /* SIMGRID_PORTABLE_H */
