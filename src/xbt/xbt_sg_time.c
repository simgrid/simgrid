/* time - time related syscal wrappers                                      */

/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras/Virtu/virtu_sg.h"
#include "simgrid/simix.h"

/*
 * Time elapsed since the begining of the simulation.
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

