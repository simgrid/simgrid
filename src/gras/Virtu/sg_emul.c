/* $Id$ */

/* sg_emul - Emulation support (simulation)                                 */

/* Copyright (c) 2003-5 Arnaud Legrand, Martin Quinson. All rights reserved.*/

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "gras/emul.h"
#include "gras/Virtu/virtu_sg.h"
#include "gras_modinter.h"

#include "xbt/xbt_portability.h" /* timers */
#include "xbt/dict.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(emul,gras,"Emulation support");

/*** Timing macros ***/
static xbt_os_timer_t timer;
static int benchmarking = 0;
static xbt_dict_t benchmark_set = NULL;
static double reference = .00523066250047108838; /* FIXME: we should benchmark host machine to set this */
static double duration = 0.0;

static char* locbuf = NULL;
static int locbufsize;

static void store_in_dict(xbt_dict_t dict, const char *key, double value)
{
  double *ir = NULL;

  xbt_dict_get(dict, key, (void *) &ir);
  if (!ir) {
    ir = xbt_new0(double,1);
    xbt_dict_set(dict, key, ir, free);
  }
  *ir = value;
}

static double get_from_dict(xbt_dict_t dict, const char *key)
{
  double *ir = NULL;

  xbt_dict_get(dict, key, (void *) &ir);

  return *ir;
}

int gras_bench_always_begin(const char *location,int line)
{
  xbt_assert0(!benchmarking,"Already benchmarking");
  benchmarking = 1;

  if (!timer)
  xbt_os_timer_start(timer);
  return 0;
}

int gras_bench_always_end(void)
{
  m_task_t task = NULL;
  
  xbt_assert0(benchmarking,"Not benchmarking yet");
  benchmarking = 0;
  xbt_os_timer_stop(timer);
  duration = xbt_os_timer_elapsed(timer);
  task = MSG_task_create("task", (duration)/reference, 0 , NULL);
  MSG_task_execute(task);
  MSG_task_destroy(task);
  return 0;
}

int gras_bench_once_begin(const char *location,int line)
{
  double *ir = NULL;
  xbt_assert0(!benchmarking,"Already benchmarking");
  benchmarking = 1;

  if (!locbuf || locbufsize < strlen(location) + 64) {
     locbufsize = strlen(location) + 64;
     locbuf = xbt_realloc(locbuf,locbufsize);
  }
  sprintf(locbuf,"%s:%d",location, line);
   
  xbt_dict_get(benchmark_set, locbuf, (void *) &ir);
  if(!ir) {
    DEBUG1("%s",locbuf); 
    duration = 1;
    xbt_os_timer_start(timer);
    return 1;
  } else {
    duration = -1.0;
    return 0;
  }
}

int gras_bench_once_end(void)
{
  m_task_t task = NULL;

  xbt_assert0(benchmarking,"Not benchmarking yet");
  benchmarking = 0;
  if(duration>0) {
    xbt_os_timer_stop(timer);
    duration = xbt_os_timer_elapsed(timer);
    store_in_dict(benchmark_set, locbuf, duration);
  } else {
    duration = get_from_dict(benchmark_set,locbuf);
  }
  DEBUG2("Simulate the run of a task of %f sec for %s",duration,locbuf);
  task = MSG_task_create("task", (duration)/reference, 0 , NULL);
  MSG_task_execute(task);
  MSG_task_destroy(task);
  return 0;
}

void gras_chrono_init(void)
{
  if(!benchmark_set) {
    benchmark_set = xbt_dict_new();
    timer = xbt_os_timer_new();
  }
}

void gras_chrono_exit(void) {
  if (locbuf) free(locbuf);
  xbt_dict_free(&benchmark_set);
  xbt_os_timer_free(timer);
}

/*** Conditional execution support ***/

int gras_if_RL(void) {
   return 0;
}

int gras_if_SG(void) {
   return 1;
}


