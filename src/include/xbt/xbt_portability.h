/* $Id$ */
/*  xbt/xbt_portability.h -- all system dependency                          */
/* Private portability layer                                                */

/* Copyright (c) 2004,2005 Martin Quinson. All rights reserved.             */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_PORTABILITY_H
#define _XBT_PORTABILITY_H

/** @brief get time in seconds 

  * gives  the  number  of  seconds since the Epoch (00:00:00 UTC, January 1, 1970).
  * Most users should use gras_os_time and should not use this function unless 
    they really know what they are doing. */
double xbt_os_time(void);

#endif
