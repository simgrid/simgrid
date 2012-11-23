/* virtu - virtualization layer for XBT to choose between GRAS and MSG implementation */

/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h"
#include "xbt/virtu.h"
#include "xbt/function_types.h"
#include "simgrid/simix.h"

char *xbt_os_procname_data = NULL;

static int xbt_fake_pid(void)
{
  return 0;
}

int_f_void_t xbt_getpid = xbt_fake_pid;

/*
 * Time elapsed since the beginning of the simulation.
 */
double xbt_time()
{
  /* FIXME: check if we should use the request mechanism or not */
  return SIMIX_get_clock();
}

/*
 * Freeze the process for the specified amount of time
 */
void xbt_sleep(double sec)
{
  simcall_process_sleep(sec);
}

const char *xbt_procname(void)
{
  return SIMIX_process_self_get_name();
}

const char *xbt_os_procname(void)
{
  return xbt_os_procname_data;
}
