#ifndef __XBT_WIN32_CONFIG_H__
#define __XBT_WIN32_CONFIG_H__


/* config.h - simgrid config selection for windows platforms. */

/* Copyright (c) 2006, 2007, 2008, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* 
 * config selection. 
*/
#if defined(__GNUC__)
        /* data comes from autoconf when using gnuc (cross-compiling?) */
  # include "internal_config.h"
  #ifndef _XBT_WIN32
    typedef unsigned int uint32_t;
  #endif
# else
  # error "Unknown compiler - please report the problems to the main simgrid mailing list (http://gforge.inria.fr/mail/?group_id=12)"
#endif

#ifndef _XBT_VISUALC_COMPILER
  #ifndef EWOULDBLOCK
  #define EWOULDBLOCK WSAEWOULDBLOCK
  #endif

  #ifndef EINPROGRESS
  #define EINPROGRESS WSAEINPROGRESS
  #endif

  #ifndef ETIMEDOUT
  #define ETIMEDOUT   WSAETIMEDOUT
  #endif
#endif

#ifdef S_IRGRP
  #undef S_IRGRP
#endif

#define S_IRGRP 0

#ifdef S_IWGRP
  #undef S_IWGRP
#endif

#define S_IWGRP 0

#endif                          /* #ifndef __XBT_WIN32_CONFIG_H__ */
