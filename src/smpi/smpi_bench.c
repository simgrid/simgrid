/* Copyright (c) 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include <math.h> // sqrt
#include "private.h"
#include "xbt/dict.h"
#include "xbt/sysdep.h"
#include "xbt/ex.h"
#include "surf/surf.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_bench, smpi,
                                "Logging specific to SMPI (benchmarking)");

xbt_dict_t allocs = NULL;       /* Allocated on first use */
xbt_dict_t samples = NULL;      /* Allocated on first use */
xbt_dict_t calls = NULL;        /* Allocated on first use */

typedef struct {
  int count;
  char data[];
} shared_data_t;

typedef struct {
  int count;
  double sum;
  double sum_pow2;
  double mean;
  double relstderr;
  int iters;
  double threshold;
  int started;
} local_data_t;

void smpi_bench_destroy(void)
{
  xbt_dict_free(&allocs);
  xbt_dict_free(&samples);
  xbt_dict_free(&calls);
}

static void smpi_execute_flops(double flops)
{
  smx_action_t action;
  smx_host_t host;
  host = SIMIX_host_self();

  XBT_DEBUG("Handle real computation time: %f flops", flops);
  action = SIMIX_req_host_execute("computation", host, flops, 1);
#ifdef HAVE_TRACING
  SIMIX_req_set_category (action, TRACE_internal_smpi_get_category());
#endif
  SIMIX_req_host_execution_wait(action);
}

static void smpi_execute(double duration)
{
  if (duration >= xbt_cfg_get_double(_surf_cfg_set, "smpi/cpu_threshold")) {
    XBT_DEBUG("Sleep for %f to handle real computation time", duration);
    smpi_execute_flops(duration *
                       xbt_cfg_get_double(_surf_cfg_set,
                                          "smpi/running_power"));
  }
}

void smpi_bench_begin(void)
{
  xbt_os_timer_start(smpi_process_timer());
}

void smpi_bench_end(void)
{
  xbt_os_timer_t timer = smpi_process_timer();

  xbt_os_timer_stop(timer);
  smpi_execute(xbt_os_timer_elapsed(timer));
}

unsigned int smpi_sleep(unsigned int secs)
{
  smpi_execute((double) secs);
  return secs;
}

int smpi_gettimeofday(struct timeval *tv, struct timezone *tz)
{
  double now = SIMIX_get_clock();

  if (tv) {
    tv->tv_sec = (time_t) now;
    tv->tv_usec = (suseconds_t) (now * 1e6);
  }
  return 0;
}

static char *sample_location(int global, const char *file, int line)
{
  if (global) {
    return bprintf("%s:%d", file, line);
  } else {
    return bprintf("%s:%d:%d", file, line, smpi_process_index());
  }
}

int smpi_sample_1(int global, const char *file, int line, int iters, double threshold)
{
  char *loc = sample_location(global, file, line);
  local_data_t *data;

  smpi_bench_end();     /* Take time from previous MPI call into account */
  if (!samples) {
    samples = xbt_dict_new_homogeneous(free);
  }
  data = xbt_dict_get_or_null(samples, loc);
  if (!data) {
    data = (local_data_t *) xbt_new(local_data_t, 1);
    data->count = 0;
    data->sum = 0.0;
    data->sum_pow2 = 0.0;
    data->iters = iters;
    data->threshold = threshold;
    data->started = 0;
    xbt_dict_set(samples, loc, data, NULL);
    return 0;
  }
  free(loc);
  return 1;
}

int smpi_sample_2(int global, const char *file, int line)
{
  char *loc = sample_location(global, file, line);
  local_data_t *data;

  xbt_assert(samples, "You did something very inconsistent, didn't you?");
  data = xbt_dict_get_or_null(samples, loc);
  if (!data) {
    xbt_assert(data, "Please, do thing in order");
  }
  if (!data->started) {
    if ((data->iters > 0 && data->count >= data->iters)
        || (data->count > 1 && data->threshold > 0.0 && data->relstderr <= data->threshold)) {
      XBT_DEBUG("Perform some wait of %f", data->mean);
      smpi_execute(data->mean);
    } else {
      data->started = 1;
      data->count++;
    }
  } else {
    data->started = 0;
  }
  free(loc);
  smpi_bench_begin();
  smpi_process_simulated_start();
  return data->started;
}

void smpi_sample_3(int global, const char *file, int line)
{
  char *loc = sample_location(global, file, line);
  local_data_t *data;
  double sample, n;

  xbt_assert(samples, "You did something very inconsistent, didn't you?");
  data = xbt_dict_get_or_null(samples, loc);
  smpi_bench_end();
  if(data && data->started && data->count < data->iters) {
    sample = smpi_process_simulated_elapsed();
    data->sum += sample;
    data->sum_pow2 += sample * sample;
    n = (double)data->count;
    data->mean = data->sum / n;
    data->relstderr = sqrt((data->sum_pow2 / n - data->mean * data->mean) / n) / data->mean;
    XBT_DEBUG("Average mean after %d steps is %f, relative standard error is %f (sample was %f)", data->count,
           data->mean, data->relstderr, sample);
  }
  free(loc);
}

void smpi_sample_flops(double flops)
{
  smpi_execute_flops(flops);
}

void *smpi_shared_malloc(size_t size, const char *file, int line)
{
  char *loc = bprintf("%s:%d:%zu", file, line, size);
  shared_data_t *data;

  if (!allocs) {
    allocs = xbt_dict_new_homogeneous(free);
  }
  data = xbt_dict_get_or_null(allocs, loc);
  if (!data) {
    data = (shared_data_t *) xbt_malloc0(sizeof(int) + size);
    data->count = 1;
    xbt_dict_set(allocs, loc, data, NULL);
  } else {
    data->count++;
  }
  free(loc);
  return data->data;
}

void smpi_shared_free(void *ptr)
{
  shared_data_t *data = (shared_data_t *) ((int *) ptr - 1);
  char *loc;

  if (!allocs) {
    XBT_WARN("Cannot free: nothing was allocated");
    return;
  }
  loc = xbt_dict_get_key(allocs, data);
  if (!loc) {
    XBT_WARN("Cannot free: %p was not shared-allocated by SMPI", ptr);
    return;
  }
  data->count--;
  if (data->count <= 0) {
    xbt_dict_remove(allocs, loc);
  }
}

int smpi_shared_known_call(const char* func, const char* input) {
   char* loc = bprintf("%s:%s", func, input);
   xbt_ex_t ex;
   int known;

   if(!calls) {
      calls = xbt_dict_new_homogeneous(NULL);
   }
   TRY {
      xbt_dict_get(calls, loc); /* Succeed or throw */
      known = 1;
   }
   CATCH(ex) {
      if(ex.category == not_found_error) {
         known = 0;
         xbt_ex_free(ex);
      } else {
         RETHROW;
      }
   }
   free(loc);
   return known;
}

void* smpi_shared_get_call(const char* func, const char* input) {
   char* loc = bprintf("%s:%s", func, input);
   void* data;

   if(!calls) {
      calls = xbt_dict_new_homogeneous(NULL);
   }
   data = xbt_dict_get(calls, loc);
   free(loc);
   return data;
}

void* smpi_shared_set_call(const char* func, const char* input, void* data) {
   char* loc = bprintf("%s:%s", func, input);

   if(!calls) {
      calls = xbt_dict_new_homogeneous(NULL);
   }
   xbt_dict_set(calls, loc, data, NULL);
   free(loc);
   return data;
}
