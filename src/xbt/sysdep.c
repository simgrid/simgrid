/* $Id$ */

/* sysdep.h -- all system dependency                                        */
/*  no system header should be loaded out of this file so that we have only */
/*  one file to check when porting to another OS                            */

/* Copyright (c) 2004 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/error.h"

#include <stdlib.h>

/* \defgroup XBT_sysdep All system dependency
 * \brief This section describes many macros/functions that can serve as
 *  an OS abstraction.
 */

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(sysdep, xbt, "System dependency");

