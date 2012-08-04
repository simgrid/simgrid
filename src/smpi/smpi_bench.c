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

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_bench, smpi,
                                "Logging specific to SMPI (benchmarking)");

/* Shared allocations are handled through shared memory segments.
 * Associated data and metadata are used as follows:
 *
 *                                                                    mmap #1
 *    `allocs' dict                                                     ---- -.
 *    ----------      shared_data_t               shared_metadata_t   / |  |  |
 * .->| <name> | ---> -------------------- <--.   -----------------   | |  |  |
 * |  ----------      | fd of <name>     |    |   | size of mmap  | --| |  |  |
 * |                  | count (2)        |    |-- | data          |   \ |  |  |
 * `----------------- | <name>           |    |   -----------------     ----  |
 *                    --------------------    |   ^                           |
 *                                            |   |                           |
 *                                            |   |   `allocs_metadata' dict  |
 *                                            |   |   ----------------------  |
 *                                            |   `-- | <addr of mmap #1>  |<-'
 *                                            |   .-- | <addr of mmap #2>  |<-.
 *                                            |   |   ----------------------  |
 *                                            |   |                           |
 *                                            |   |                           |
 *                                            |   |                           |
 *                                            |   |                   mmap #2 |
 *                                            |   v                     ---- -'
 *                                            |   shared_metadata_t   / |  |
 *                                            |   -----------------   | |  |
 *                                            |   | size of mmap  | --| |  |
 *                                            `-- | data          |   | |  |
 *                                                -----------------   | |  |
 *                                                                    \ |  |
 *                                                                      ----
 */

#define PTR_STRLEN (2 + 2 * sizeof(void*) + 1)

xbt_dict_t allocs = NULL;          /* Allocated on first use */
xbt_dict_t allocs_metadata = NULL; /* Allocated on first use */
xbt_dict_t samples = NULL;         /* Allocated on first use */
xbt_dict_t calls = NULL;           /* Allocated on first use */
__thread int smpi_current_rank = 0;      /* Updated after each MPI call */

typedef struct {
  int fd;
  int count;
  char* loc;
} shared_data_t;

typedef struct  {
  size_t size;
  shared_data_t* data;
} shared_metadata_t;

static size_t shm_size(int fd) {
  struct stat st;

  if(fstat(fd, &st) < 0) {
    xbt_die("Could not stat fd %d: %s", fd, strerror(errno));
  }
  return (size_t)st.st_size;
}

static void* shm_map(int fd, size_t size, shared_data_t* data) {
  void* mem;
  char loc[PTR_STRLEN];
  shared_metadata_t* meta;

  if(size > shm_size(fd)) {
    if(ftruncate(fd, (off_t)size) < 0) {
      xbt_die("Could not truncate fd %d to %zu: %s", fd, size, strerror(errno));
    }
  }
  mem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if(mem == MAP_FAILED) {
    xbt_die("Could not map fd %d: %s", fd, strerror(errno));
  }
  if(!allocs_metadata) {
    allocs_metadata = xbt_dict_new();
  }
  snprintf(loc, PTR_STRLEN, "%p", mem);
  meta = xbt_new(shared_metadata_t, 1);
  meta->size = size;
  meta->data = data;
  xbt_dict_set(allocs_metadata, loc, meta, &free);
  XBT_DEBUG("MMAP %zu to %p", size, mem);
  return mem;
}

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
  action = simcall_host_execute("computation", host, flops, 1);
#ifdef HAVE_TRACING
  simcall_set_category (action, TRACE_internal_smpi_get_category());
#endif
  simcall_host_execution_wait(action);
}

static void smpi_execute(double duration)
{
  /* FIXME: a global variable would be less expensive to consult than a call to xbt_cfg_get_double() right on the critical path */
  if (duration >= xbt_cfg_get_double(_surf_cfg_set, "smpi/cpu_threshold")) {
    XBT_DEBUG("Sleep for %f to handle real computation time", duration);
    smpi_execute_flops(duration *
                       xbt_cfg_get_double(_surf_cfg_set,
                                          "smpi/running_power"));
  } else {
    XBT_DEBUG("Real computation took %f while threshold is set to %f; ignore it",
        duration, xbt_cfg_get_double(_surf_cfg_set, "smpi/cpu_threshold"));
  }
}

void smpi_bench_begin(void)
{
  xbt_os_timer_start(smpi_process_timer());
  smpi_current_rank = smpi_process_index();
}

void smpi_bench_end(void)
{
  xbt_os_timer_t timer = smpi_process_timer();

  xbt_os_timer_stop(timer);
  smpi_execute(xbt_os_timer_elapsed(timer));
}

unsigned int smpi_sleep(unsigned int secs)
{
  smpi_bench_end();
  smpi_execute((double) secs);
  smpi_bench_begin();
  return secs;
}

int smpi_gettimeofday(struct timeval *tv, struct timezone *tz)
{
  double now;
  smpi_bench_end();
  now = SIMIX_get_clock();
  if (tv) {
    tv->tv_sec = (time_t)now;
    tv->tv_usec = (suseconds_t)((now - tv->tv_sec) * 1e6);
  }
  smpi_bench_begin();
  return 0;
}

extern double sg_maxmin_precision;
unsigned long long smpi_rastro_resolution (void)
{
  smpi_bench_end();
  double resolution = (1/sg_maxmin_precision);
  smpi_bench_begin();
  return (unsigned long long)resolution;
}

unsigned long long smpi_rastro_timestamp (void)
{
  smpi_bench_end();
  double now = SIMIX_get_clock();

  unsigned long long sec = (unsigned long long)now;
  unsigned long long pre = (now - sec) * smpi_rastro_resolution();
  smpi_bench_begin();
  return (unsigned long long)sec * smpi_rastro_resolution() + pre;
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

  xbt_assert(samples, "Y U NO use SMPI_SAMPLE_* macros? Stop messing directly with smpi_sample_* functions!");
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

  xbt_assert(samples, "Y U NO use SMPI_SAMPLE_* macros? Stop messing directly with smpi_sample_* functions!");
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
  char *loc = bprintf("%zu_%s_%d", (size_t)getpid(), file, line);
  size_t len = strlen(loc);
  size_t i;
  int fd;
  void* mem;
  shared_data_t *data;

  for(i = 0; i < len; i++) {
    /* Make the 'loc' ID be a flat filename */
    if(loc[i] == '/') {
      loc[i] = '_';
    }
  }
  if (!allocs) {
    allocs = xbt_dict_new_homogeneous(free);
  }
  data = xbt_dict_get_or_null(allocs, loc);
  if(!data) {
    fd = shm_open(loc, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if(fd < 0) {
      switch(errno) {
        case EEXIST:
          xbt_die("Please cleanup /dev/shm/%s", loc);
        default:
          xbt_die("An unhandled error occured while opening %s: %s", loc, strerror(errno));
      }
    }
    data = xbt_new(shared_data_t, 1);
    data->fd = fd;
    data->count = 1;
    data->loc = loc;
    mem = shm_map(fd, size, data);
    if(shm_unlink(loc) < 0) {
      XBT_WARN("Could not early unlink %s: %s", loc, strerror(errno));
    }
    xbt_dict_set(allocs, loc, data, NULL);
    XBT_DEBUG("Mapping %s at %p through %d", loc, mem, fd);
  } else {
    mem = shm_map(data->fd, size, data);
    data->count++;
  }
  XBT_DEBUG("Malloc %zu in %p (metadata at %p)", size, mem, data);
  return mem;
}

void smpi_shared_free(void *ptr)
{
  char loc[PTR_STRLEN];
  shared_metadata_t* meta;
  shared_data_t* data;

  if (!allocs) {
    XBT_WARN("Cannot free: nothing was allocated");
    return;
  }
  if(!allocs_metadata) {
    XBT_WARN("Cannot free: no metadata was allocated");
  }
  snprintf(loc, PTR_STRLEN, "%p", ptr);
  meta = (shared_metadata_t*)xbt_dict_get_or_null(allocs_metadata, loc);
  if (!meta) {
    XBT_WARN("Cannot free: %p was not shared-allocated by SMPI", ptr);
    return;
  }
  data = meta->data;
  if(!data) {
    XBT_WARN("Cannot free: something is broken in the metadata link");
    return;
  }
  if(munmap(ptr, meta->size) < 0) {
    XBT_WARN("Unmapping of fd %d failed: %s", data->fd, strerror(errno));
  }
  data->count--;
  if (data->count <= 0) {
    close(data->fd);
    xbt_dict_remove(allocs, data->loc);
    free(data->loc);
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
