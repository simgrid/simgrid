/* $Id$ */

/* time - time related syscal wrappers                                      */

/* Copyright (c) 2003-2007 Martin Quinson.  All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras/Virtu/virtu_sg.h"

/*
 * Time elapsed since the begining of the simulation.
 */
double xbt_time()
{
  return SIMIX_get_clock();
}

/*
 * Freeze the process for the specified amount of time
 */
void xbt_sleep(double sec)
{
  smx_action_t act_sleep;
  smx_process_t proc = SIMIX_process_self();
  smx_mutex_t mutex;
  smx_cond_t cond;
  /* create action to sleep */
  act_sleep = SIMIX_action_sleep(SIMIX_process_get_host(proc), sec);

  mutex = SIMIX_mutex_init();
  SIMIX_mutex_lock(mutex);
  /* create conditional and register action to it */
  cond = SIMIX_cond_init();

  SIMIX_register_action_to_condition(act_sleep, cond);
  SIMIX_cond_wait(cond, mutex);
  SIMIX_mutex_unlock(mutex);

  /* remove variables */
  SIMIX_cond_destroy(cond);
  SIMIX_mutex_destroy(mutex);
  SIMIX_action_destroy(act_sleep);

}
