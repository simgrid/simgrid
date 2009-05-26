/* $Id: ex_interface.h 3782 2007-07-14 09:11:06Z mquinson $ */

/* backtrace_dummy -- stubs of this module for non-supported archs          */

/* Copyright (c) 2003-2008, Da SimGrid team. All rights reserved.           */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "xbt_modinter.h"

/* Module creation/destruction */
void xbt_backtrace_init(void)
{
}

void xbt_backtrace_exit(void)
{
}

/* create a backtrace in the given exception */
void xbt_backtrace_current(xbt_ex_t * e)
{
}

/* prepare a backtrace for display */
void xbt_ex_setup_backtrace(xbt_ex_t * e)
{
}
