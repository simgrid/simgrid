/* Copyright (c) 2007, 2009-2017. The SimGrid Team. All rights reserved.    */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/internal_config.h"
#include "private.h"
#include "private.hpp"
#include "simgrid/modelchecker.h"
#include "src/mc/mc_replay.h"
#include "smpi_process.hpp"
#include "smpi_comm.hpp"

#ifndef WIN32
#include <sys/mman.h>
#endif
#include <cmath>

#if HAVE_PAPI
#include <papi.h>
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_bench, smpi, "Logging specific to SMPI (benchmarking)");


xbt_dict_t samples = nullptr;         /* Allocated on first use */

double smpi_cpu_threshold = -1;
double smpi_host_speed;

shared_malloc_type smpi_cfg_shared_malloc = shmalloc_global;
double smpi_total_benched_time = 0;
smpi_privatization_region_t smpi_privatization_regions;

void smpi_bench_destroy()
{
  xbt_dict_free(&samples);
}

extern "C" XBT_PUBLIC(void) smpi_execute_flops_(double *flops);
void smpi_execute_flops_(double *flops)
{
  smpi_execute_flops(*flops);
}

extern "C" XBT_PUBLIC(void) smpi_execute_(double *duration);
void smpi_execute_(double *duration)
{
  smpi_execute(*duration);
}

void smpi_execute_flops(double flops) {
  XBT_DEBUG("Handle real computation time: %f flops", flops);
  smx_activity_t action = simcall_execution_start("computation", flops, 1, 0);
  simcall_set_category (action, TRACE_internal_smpi_get_category());
  simcall_execution_wait(action);
  smpi_switch_data_segment(smpi_process()->index());
}

void smpi_execute(double duration)
{
  if (duration >= smpi_cpu_threshold) {
    XBT_DEBUG("Sleep for %g to handle real computation time", duration);
    double flops = duration * smpi_host_speed;
    int rank = smpi_process()->index();
    instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
    extra->type=TRACING_COMPUTING;
    extra->comp_size=flops;
    TRACE_smpi_computing_in(rank, extra);

    smpi_execute_flops(flops);

    TRACE_smpi_computing_out(rank);

  } else {
    XBT_DEBUG("Real computation took %g while option smpi/cpu-threshold is set to %g => ignore it", duration,
              smpi_cpu_threshold);
  }
}

void smpi_execute_benched(double duration)
{
  smpi_bench_end();
  smpi_execute(duration);
  smpi_bench_begin();
}

void smpi_bench_begin()
{
  if (smpi_privatize_global_variables == SMPI_PRIVATIZE_MMAP) {
    smpi_switch_data_segment(smpi_process()->index());
  }

  if (MC_is_active() || MC_record_replay_is_active())
    return;

#if HAVE_PAPI
  if (xbt_cfg_get_string("smpi/papi-events")[0] != '\0') {
    int event_set = smpi_process()->papi_event_set();
    // PAPI_start sets everything to 0! See man(3) PAPI_start
    if (PAPI_LOW_LEVEL_INITED == PAPI_is_initialized()) {
      if (PAPI_start(event_set) != PAPI_OK) {
        // TODO This needs some proper handling.
        XBT_CRITICAL("Could not start PAPI counters.\n");
        xbt_die("Error.");
      }
    }
  }
#endif
  xbt_os_threadtimer_start(smpi_process()->timer());
}

void smpi_bench_end()
{
  if (MC_is_active() || MC_record_replay_is_active())
    return;

  double speedup = 1;
  xbt_os_timer_t timer = smpi_process()->timer();
  xbt_os_threadtimer_stop(timer);

#if HAVE_PAPI
  /**
   * An MPI function has been called and now is the right time to update
   * our PAPI counters for this process.
   */
  if (xbt_cfg_get_string("smpi/papi-events")[0] != '\0') {
    papi_counter_t& counter_data        = smpi_process()->papi_counters();
    int event_set                       = smpi_process()->papi_event_set();
    std::vector<long long> event_values = std::vector<long long>(counter_data.size());

    if (PAPI_stop(event_set, &event_values[0]) != PAPI_OK) { // Error
      XBT_CRITICAL("Could not stop PAPI counters.\n");
      xbt_die("Error.");
    } else {
      for (unsigned int i = 0; i < counter_data.size(); i++) {
        counter_data[i].second += event_values[i];
        // XBT_DEBUG("[%i] PAPI: Counter %s: Value is now %lli (got increment by %lli\n", smpi_process()->index(),
        // counter_data[i].first.c_str(), counter_data[i].second, event_values[i]);
      }
    }
  }
#endif

  if (smpi_process()->sampling()) {
    XBT_CRITICAL("Cannot do recursive benchmarks.");
    XBT_CRITICAL("Are you trying to make a call to MPI within a SMPI_SAMPLE_ block?");
    xbt_backtrace_display_current();
    xbt_die("Aborting.");
  }

  if (xbt_cfg_get_string("smpi/comp-adjustment-file")[0] != '\0') { // Maybe we need to artificially speed up or slow
    // down our computation based on our statistical analysis.

    smpi_trace_call_location_t* loc                            = smpi_process()->call_location();
    std::string key                                            = loc->get_composed_key();
    std::unordered_map<std::string, double>::const_iterator it = location2speedup.find(key);
    if (it != location2speedup.end()) {
      speedup = it->second;
    }
  }

  // Simulate the benchmarked computation unless disabled via command-line argument
  if (xbt_cfg_get_boolean("smpi/simulate-computation")) {
    smpi_execute(xbt_os_timer_elapsed(timer)/speedup);
  }

#if HAVE_PAPI
  if (xbt_cfg_get_string("smpi/papi-events")[0] != '\0' && TRACE_smpi_is_enabled()) {
    char container_name[INSTR_DEFAULT_STR_SIZE];
    smpi_container(smpi_process()->index(), container_name, INSTR_DEFAULT_STR_SIZE);
    container_t container        = PJ_container_get(container_name);
    papi_counter_t& counter_data = smpi_process()->papi_counters();

    for (auto& pair : counter_data) {
      new_pajeSetVariable(surf_get_clock(), container,
                          PJ_type_get(/* countername */ pair.first.c_str(), container->type), pair.second);
    }
  }
#endif

  smpi_total_benched_time += xbt_os_timer_elapsed(timer);
}

/* Private sleep function used by smpi_sleep() and smpi_usleep() */
static unsigned int private_sleep(double secs)
{
  smpi_bench_end();

  XBT_DEBUG("Sleep for: %lf secs", secs);
  int rank = MPI_COMM_WORLD->rank();
  instr_extra_data extra = xbt_new0(s_instr_extra_data_t,1);
  extra->type=TRACING_SLEEPING;
  extra->sleep_duration=secs;
  TRACE_smpi_sleeping_in(rank, extra);

  simcall_process_sleep(secs);

  TRACE_smpi_sleeping_out(rank);

  smpi_bench_begin();
  return 0;
}

unsigned int smpi_sleep(unsigned int secs)
{
  return private_sleep(static_cast<double>(secs));
}

int smpi_usleep(useconds_t usecs)
{
  return static_cast<int>(private_sleep(static_cast<double>(usecs) / 1000000.0));
}

#if _POSIX_TIMERS > 0
int smpi_nanosleep(const struct timespec *tp, struct timespec * t)
{
  return static_cast<int>(private_sleep(static_cast<double>(tp->tv_sec + tp->tv_nsec / 1000000000.0)));
}
#endif

int smpi_gettimeofday(struct timeval *tv, void* tz)
{
  smpi_bench_end();
  double now = SIMIX_get_clock();
  if (tv) {
    tv->tv_sec = static_cast<time_t>(now);
#ifdef WIN32
    tv->tv_usec = static_cast<useconds_t>((now - tv->tv_sec) * 1e6);
#else
    tv->tv_usec = static_cast<suseconds_t>((now - tv->tv_sec) * 1e6);
#endif
  }
  smpi_bench_begin();
  return 0;
}

#if _POSIX_TIMERS > 0
int smpi_clock_gettime(clockid_t clk_id, struct timespec *tp)
{
  //there is only one time in SMPI, so clk_id is ignored.
  smpi_bench_end();
  double now = SIMIX_get_clock();
  if (tp) {
    tp->tv_sec = static_cast<time_t>(now);
    tp->tv_nsec = static_cast<long int>((now - tp->tv_sec) * 1e9);
  }
  smpi_bench_begin();
  return 0;
}
#endif

extern double sg_surf_precision;
unsigned long long smpi_rastro_resolution ()
{
  smpi_bench_end();
  double resolution = (1/sg_surf_precision);
  smpi_bench_begin();
  return static_cast<unsigned long long>(resolution);
}

unsigned long long smpi_rastro_timestamp ()
{
  smpi_bench_end();
  double now = SIMIX_get_clock();

  unsigned long long sec = static_cast<unsigned long long>(now);
  unsigned long long pre = (now - sec) * smpi_rastro_resolution();
  smpi_bench_begin();
  return static_cast<unsigned long long>(sec) * smpi_rastro_resolution() + pre;
}

/* ****************************** Functions related to the SMPI_SAMPLE_ macros ************************************/
typedef struct {
  double threshold; /* maximal stderr requested (if positive) */
  double relstderr; /* observed stderr so far */
  double mean;      /* mean of benched times, to be used if the block is disabled */
  double sum;       /* sum of benched times (to compute the mean and stderr) */
  double sum_pow2;  /* sum of the square of the benched times (to compute the stderr) */
  int iters;        /* amount of requested iterations */
  int count;        /* amount of iterations done so far */
  int benching;     /* 1: we are benchmarking; 0: we have enough data, no bench anymore */
} local_data_t;

static char *sample_location(int global, const char *file, int line) {
  if (global) {
    return bprintf("%s:%d", file, line);
  } else {
    return bprintf("%s:%d:%d", file, line, smpi_process()->index());
  }
}

static int sample_enough_benchs(local_data_t *data) {
  int res = data->count >= data->iters;
  if (data->threshold>0.0) {
    if (data->count <2)
      res = 0; // not enough data
    if (data->relstderr > data->threshold)
      res = 0; // stderr too high yet
  }
  XBT_DEBUG("%s (count:%d iter:%d stderr:%f thres:%f mean:%fs)",
      (res?"enough benchs":"need more data"), data->count, data->iters, data->relstderr, data->threshold, data->mean);
  return res;
}

void smpi_sample_1(int global, const char *file, int line, int iters, double threshold)
{
  char *loc = sample_location(global, file, line);

  smpi_bench_end();     /* Take time from previous, unrelated computation into account */
  smpi_process()->set_sampling(1);

  if (samples==nullptr)
    samples = xbt_dict_new_homogeneous(free);

  local_data_t *data = static_cast<local_data_t *>(xbt_dict_get_or_null(samples, loc));
  if (data==nullptr) {
    xbt_assert(threshold>0 || iters>0,
        "You should provide either a positive amount of iterations to bench, or a positive maximal stderr (or both)");
    data = static_cast<local_data_t *>( xbt_new(local_data_t, 1));
    data->count = 0;
    data->sum = 0.0;
    data->sum_pow2 = 0.0;
    data->iters = iters;
    data->threshold = threshold;
    data->benching = 1; // If we have no data, we need at least one
    data->mean = 0;
    xbt_dict_set(samples, loc, data, nullptr);
    XBT_DEBUG("XXXXX First time ever on benched nest %s.",loc);
  } else {
    if (data->iters != iters || data->threshold != threshold) {
      XBT_ERROR("Asked to bench block %s with different settings %d, %f is not %d, %f. "
                "How did you manage to give two numbers at the same line??",
                loc, data->iters, data->threshold, iters, threshold);
      THROW_IMPOSSIBLE;
    }

    // if we already have some data, check whether sample_2 should get one more bench or whether it should emulate
    // the computation instead
    data->benching = (sample_enough_benchs(data) == 0);
    XBT_DEBUG("XXXX Re-entering the benched nest %s. %s", loc,
              (data->benching ? "more benching needed" : "we have enough data, skip computes"));
  }
  xbt_free(loc);
}

int smpi_sample_2(int global, const char *file, int line)
{
  char *loc = sample_location(global, file, line);
  int res;

  xbt_assert(samples, "Y U NO use SMPI_SAMPLE_* macros? Stop messing directly with smpi_sample_* functions!");
  local_data_t *data = static_cast<local_data_t *>(xbt_dict_get(samples, loc));
  XBT_DEBUG("sample2 %s",loc);
  xbt_free(loc);

  if (data->benching==1) {
    // we need to run a new bench
    XBT_DEBUG("benchmarking: count:%d iter:%d stderr:%f thres:%f; mean:%f",
        data->count, data->iters, data->relstderr, data->threshold, data->mean);
    res = 1;
  } else {
    // Enough data, no more bench (either we got enough data from previous visits to this benched nest, or we just
    //ran one bench and need to bail out now that our job is done). Just sleep instead
    XBT_DEBUG("No benchmark (either no need, or just ran one): count >= iter (%d >= %d) or stderr<thres (%f<=%f)."
              " apply the %fs delay instead",
              data->count, data->iters, data->relstderr, data->threshold, data->mean);
    smpi_execute(data->mean);
    smpi_process()->set_sampling(0);
    res = 0; // prepare to capture future, unrelated computations
  }
  smpi_bench_begin();
  return res;
}

void smpi_sample_3(int global, const char *file, int line)
{
  char *loc = sample_location(global, file, line);

  xbt_assert(samples, "Y U NO use SMPI_SAMPLE_* macros? Stop messing directly with smpi_sample_* functions!");
  local_data_t *data = static_cast<local_data_t *>(xbt_dict_get(samples, loc));
  XBT_DEBUG("sample3 %s",loc);
  xbt_free(loc);

  if (data->benching==0)
    THROW_IMPOSSIBLE;

  // ok, benchmarking this loop is over
  xbt_os_threadtimer_stop(smpi_process()->timer());

  // update the stats
  data->count++;
  double sample = xbt_os_timer_elapsed(smpi_process()->timer());
  data->sum += sample;
  data->sum_pow2 += sample * sample;
  double n = static_cast<double>(data->count);
  data->mean = data->sum / n;
  data->relstderr = sqrt((data->sum_pow2 / n - data->mean * data->mean) / n) / data->mean;
  if (sample_enough_benchs(data)==0) {
    data->mean = sample; // Still in benching process; We want sample_2 to simulate the exact time of this loop
    // occurrence before leaving, not the mean over the history
  }
  XBT_DEBUG("Average mean after %d steps is %f, relative standard error is %f (sample was %f)", data->count,
      data->mean, data->relstderr, sample);

  // That's enough for now, prevent sample_2 to run the same code over and over
  data->benching = 0;
}

extern "C" { /** These functions will be called from the user code **/
  smpi_trace_call_location_t* smpi_trace_get_call_location() {
    return smpi_process()->call_location();
  }

  void smpi_trace_set_call_location(const char* file, const int line) {
    smpi_trace_call_location_t* loc = smpi_process()->call_location();

    loc->previous_filename   = loc->filename;
    loc->previous_linenumber = loc->linenumber;
    loc->filename            = file;
    loc->linenumber          = line;
  }

  /**
   * Required for Fortran bindings
   */
  void smpi_trace_set_call_location_(const char* file, int* line) {
    smpi_trace_set_call_location(file, *line);
  }

  /**
   * Required for Fortran if -fsecond-underscore is activated
   */
  void smpi_trace_set_call_location__(const char* file, int* line) {
    smpi_trace_set_call_location(file, *line);
  }
}
