/* 	$Id$	 */

/* sg_chrono.c - code benchmarking for emulation                            */

/* Copyright (c) 2005 Martin Quinson, Arnaud Legrand. All rights reserved.  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "xbt/dict.h"
#include "gras/chrono.h"
#include "msg/msg.h"
#include "portable.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(chrono,gras,"Benchmarking used code");

static double timer = 0.0;
static int benchmarking = 0;
static xbt_dict_t benchmark_set = NULL;
static double reference = .00523066250047108838;
static double duration = 0.0;
static const char* __location__ = NULL;

static void store_in_dict(xbt_dict_t dict, const char *key, double value)
{
  double *ir = NULL;

  xbt_dict_get(dict, key, (void *) &ir);
  if (!ir) {
    ir = calloc(1, sizeof(double));
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

int gras_bench_always_begin(const char *location, int line)
{
  xbt_assert0(!benchmarking,"Already benchmarking");
  benchmarking = 1;

  timer = xbt_os_time();
  return 0;
}

int gras_bench_always_end(void)
{
  m_task_t task = NULL;
  
  xbt_assert0(benchmarking,"Not benchmarking yet");
  benchmarking = 0;
  duration = xbt_os_time()-timer;
  task = MSG_task_create("task", (duration)/reference, 0 , NULL);
  MSG_task_execute(task);
  /*   printf("---> %lg <--- \n", xbt_os_time()-timer); */
  MSG_task_destroy(task);
  return 0;
}

int gras_bench_once_begin(const char *location, int line)
{
  double *ir = NULL;
  xbt_assert0(!benchmarking,"Already benchmarking");
  benchmarking = 1;

  __location__=location;
  xbt_dict_get(benchmark_set, __location__, (void *) &ir);
  if(!ir) {
/*     printf("%s:%d\n",location,line); */
    duration = xbt_os_time();
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
    duration = xbt_os_time()-duration;
    store_in_dict(benchmark_set, __location__, duration);
  } else {
    duration = get_from_dict(benchmark_set,__location__);
  }
  task = MSG_task_create("task", (duration)/reference, 0 , NULL);
  MSG_task_execute(task);
  MSG_task_destroy(task);
  return 0;
}

void gras_chrono_init(void)
{
  if(!benchmark_set)
    benchmark_set = xbt_dict_new();
}
