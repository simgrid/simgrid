/* portable -- header loading to write portable code                         */
/* loads much more stuff than sysdep.h since the latter is in public interface*/

/* Copyright (c) 2004-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_PORTABLE_H
#define SIMGRID_PORTABLE_H

#include "simgrid_config.h"       /* what was compiled in? */
#include "src/internal_config.h"  /* some information about the environment */

#include "xbt/base.h"
#include "xbt/misc.h"
#ifdef _XBT_WIN32
# include <windows.h>
#endif

#include <stdarg.h>
#include <stdio.h>

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

/* What we need to extract the backtrace in exception handling code */
#ifdef HAVE_EXECINFO_H
#  include <execinfo.h>
#endif

#endif                          /* SIMGRID_PORTABLE_H */
