/* $Id$ */

/* gras/sysdep.h -- all system dependency                                   */
/*  no system header should be loaded out of this file so that we have only */
/*  one file to check when porting to another OS                            */

/* Authors: Martin Quinson                                                  */
/* Copyright (C) 2004 the OURAGAN project.                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/error.h"

#include <stdlib.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(sysdep, xbt, "System dependency");

/****
 **** Misc
 ****/

void xbt_abort(void) {
   abort();
}
