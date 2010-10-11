/* Copyright (c) 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/dict.h"
#include "xbt/sysdep.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_bench, smpi,
                                "Logging specific to SMPI (benchmarking)");

xbt_dict_t allocs = NULL;       /* Allocated on first use */
xbt_dict_t samples = NULL;      /* Allocated on first use */

typedef struct {
  int count;
  char data[];
} shared_data_t;

typedef struct {
  double time;
  int count;
  int max;
  int started;
} local_data_t;

void smpi_bench_destroy(void)
{
  if (allocs) {
    xbt_dict_free(&allocs);
  }
  if (samples) {
    xbt_dict_free(&samples);
  }
}

static void smpi_execute_flops(double flops)
{
  smx_host_t host;
  smx_action_t action;
  smx_mutex_t mutex;
  smx_cond_t cond;
  e_surf_action_state_t state;

  host = SIMIX_host_self();
  mutex = SIMIX_mutex_init();
  cond = SIMIX_cond_init();
  DEBUG1("Handle real computation time: %f flops", flops);
  action = SIMIX_action_execute(host, "computation", flops);
  SIMIX_mutex_lock(mutex);
  SIMIX_register_action_to_condition(action, cond);
  for (state = SIMIX_action_get_state(action);
       state == SURF_ACTION_READY ||
       state == SURF_ACTION_RUNNING;
       state = SIMIX_action_get_state(action)) {
    SIMIX_cond_wait(cond, mutex);
  }
  SIMIX_unregister_action_to_condition(action, cond);
  SIMIX_mutex_unlock(mutex);
  SIMIX_action_destroy(action);
  SIMIX_cond_destroy(cond);
  SIMIX_mutex_destroy(mutex);
}

static void smpi_execute(double duration)
{
  if (duration >= xbt_cfg_get_double(_surf_cfg_set, "smpi/cpu_threshold")) {
    DEBUG1("Sleep for %f to handle real computation time", duration);
    smpi_execute_flops(duration *
                       xbt_cfg_get_double(_surf_cfg_set,
                                          "smpi/running_power"));
  }
}

void smpi_bench_begin(int rank, const char *mpi_call)
{
  if (mpi_call && rank >= 0
      && xbt_cfg_get_int(_surf_cfg_set, "smpi/log_events")) {
    INFO3("SMPE: ts=%f rank=%d type=end et=%s", SIMIX_get_clock(), rank,
          mpi_call);
  }
  xbt_os_timer_start(smpi_process_timer());
}

void smpi_bench_end(int rank, const char *mpi_call)
{
  xbt_os_timer_t timer = smpi_process_timer();

  xbt_os_timer_stop(timer);
  smpi_execute(xbt_os_timer_elapsed(timer));
  if (mpi_call && rank >= 0
      && xbt_cfg_get_int(_surf_cfg_set, "smpi/log_events")) {
    INFO3("SMPE: ts=%f rank=%d type=begin et=%s", SIMIX_get_clock(), rank,
          mpi_call);
  }
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

void smpi_sample_1(int global, const char *file, int line, int max)
{
  char *loc = sample_location(global, file, line);
  local_data_t *data;

  smpi_bench_end(-1, NULL);     /* Take time from previous MPI call into account */
  if (!samples) {
    samples = xbt_dict_new();
  }
  data = xbt_dict_get_or_null(samples, loc);
  if (!data) {
    data = (local_data_t *) xbt_new(local_data_t, 1);
    data->time = 0.0;
    data->count = 0;
    data->max = max;
    data->started = 0;
    xbt_dict_set(samples, loc, data, &free);
  }
  free(loc);
}

int smpi_sample_2(int global, const char *file, int line)
{
  char *loc = sample_location(global, file, line);
  local_data_t *data;

  xbt_assert0(samples, "You did something very inconsistent, didn't you?");
  data = xbt_dict_get_or_null(samples, loc);
  if (!data) {
    xbt_assert0(data, "Please, do thing in order");
  }
  if (!data->started) {
    if (data->count < data->max) {
      data->started = 1;
      data->count++;
    } else {
      DEBUG1("Perform some wait of %f", data->time / (double) data->count);
      smpi_execute(data->time / (double) data->count);
    }
  } else {
    data->started = 0;
  }
  free(loc);
  smpi_bench_begin(-1, NULL);
  smpi_process_simulated_start();
  return data->started;
}

void smpi_sample_3(int global, const char *file, int line)
{
  char *loc = sample_location(global, file, line);
  local_data_t *data;

  xbt_assert0(samples, "You did something very inconsistent, didn't you?");
  data = xbt_dict_get_or_null(samples, loc);
  if (!data || !data->started || data->count >= data->max) {
    xbt_assert0(data, "Please, do thing in order");
  }
  smpi_bench_end(-1, NULL);
  data->time += smpi_process_simulated_elapsed();
  DEBUG2("Average mean after %d steps is %f", data->count,
         data->time / (double) data->count);
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
    allocs = xbt_dict_new();
  }
  data = xbt_dict_get_or_null(allocs, loc);
  if (!data) {
    data = (shared_data_t *) xbt_malloc0(sizeof(int) + size);
    data->count = 1;
    xbt_dict_set(allocs, loc, data, &free);
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
    WARN0("Cannot free: nothing was allocated");
    return;
  }
  loc = xbt_dict_get_key(allocs, data);
  if (!loc) {
    WARN1("Cannot free: %p was not shared-allocated by SMPI", ptr);
    return;
  }
  data->count--;
  if (data->count <= 0) {
    xbt_dict_remove(allocs, loc);
  }
}
