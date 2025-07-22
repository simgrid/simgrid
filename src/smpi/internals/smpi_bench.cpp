/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.hpp"
#include "simgrid/host.h"
#include "simgrid/modelchecker.h"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Exec.hpp"
#include "smpi_comm.hpp"
#include "smpi_utils.hpp"
#include "src/internal_config.h"
#include "src/kernel/lmm/System.hpp" // sg_precision_timing
#include "src/mc/mc_replay.hpp"
#include "src/sthread/sthread.h"
#include "xbt/config.hpp"
#include "xbt/file.hpp"
#include <getopt.h>

#include "src/smpi/include/smpi_actor.hpp"
#include "xbt/log.h"
#include "xbt/xbt_os_time.h"

#include <cerrno>
#include <cmath>
#include <sys/mman.h>
#include <unordered_map>

#if HAVE_PAPI
#include <papi.h>
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_bench, smpi, "Logging specific to SMPI (benchmarking)");

static simgrid::config::Flag<double>
    smpi_wtime_sleep("smpi/wtime",
                     "Minimum time to inject inside a call to MPI_Wtime(), gettimeofday() and clock_gettime()",
                     1e-8 /* Documented to be 10 ns */);

// Private execute_flops used by smpi_execute and smpi_execute_benched
void private_execute_flops(double flops) {
  xbt_assert(flops >= 0, "You're trying to execute a negative amount of flops (%f)!", flops);
  XBT_DEBUG("Handle real computation time: %f flops", flops);
  simgrid::s4u::this_actor::exec_init(flops)
      ->set_name("computation")
      ->set_tracing_category(smpi_process()->get_tracing_category())
      ->start()
      ->wait();
  smpi_switch_data_segment(simgrid::s4u::Actor::self());
}

void smpi_execute_flops(double flops)
{
  private_execute_flops(flops);
}

void smpi_execute(double duration)
{
  if (duration >= smpi_cfg_cpu_thresh()) {
    XBT_DEBUG("Sleep for %gs (host time) to handle real computation time", duration);
    private_execute_flops(duration * smpi_cfg_host_speed());
  } else {
    XBT_DEBUG("Real computation took %g while option smpi/cpu-threshold is set to %g => ignore it", duration,
              smpi_cfg_cpu_thresh());
  }
}

void smpi_execute_benched(double duration)
{
  const SmpiBenchGuard suspend_bench;
  double speed = sg_host_get_speed(sg_host_self());
  smpi_execute_flops(duration*speed);
}

void smpi_execute_flops_benched(double flops) {
  const SmpiBenchGuard suspend_bench;
  smpi_execute_flops(flops);
}

void smpi_bench_begin()
{
  smpi_switch_data_segment(simgrid::s4u::Actor::self());

  if (MC_is_active() || MC_record_replay_is_active())
    return;

#if HAVE_PAPI
  if (not smpi_cfg_papi_events_file().empty()) {
      simgrid_papi_start();
  }
#endif
  xbt_os_threadtimer_start(smpi_process()->timer());

  sthread_enable();
}

double smpi_adjust_comp_speed(){
  double speedup=1;
  if (smpi_cfg_comp_adjustment_file()[0] != '\0') {
    const smpi_trace_call_location_t* loc                      = smpi_process()->call_location();
    std::string key                                            = loc->get_composed_key();
    std::unordered_map<std::string, double>::const_iterator it = location2speedup.find(key);
    if (it != location2speedup.end()) {
      speedup = it->second;
    }
  }
  return speedup;
}

void smpi_bench_end()
{
  sthread_disable();

  if (MC_is_active() || MC_record_replay_is_active())
    return;

  xbt_os_timer_t timer = smpi_process()->timer();
  xbt_os_threadtimer_stop(timer);

#if HAVE_PAPI
  /**
   * An MPI function has been called and now is the right time to update
   * our PAPI counters for this process.
   */
  if (not smpi_cfg_papi_events_file().empty()) {
      simgrid_papi_stop();
  }
#endif

  if (smpi_process()->sampling()) {
    XBT_CRITICAL("Cannot do recursive benchmarks.");
    XBT_CRITICAL("Are you trying to make a call to MPI within an SMPI_SAMPLE_ block?");
    xbt_backtrace_display_current();
    xbt_die("Aborting.");
  }

  // Maybe we need to artificially speed up or slow down our computation based on our statistical analysis.
  // Simulate the benchmarked computation unless disabled via command-line argument
  if (smpi_cfg_simulate_computation()) {
    smpi_execute(xbt_os_timer_elapsed(timer)/smpi_adjust_comp_speed());
  }

#if HAVE_PAPI
  if (not smpi_cfg_papi_events_file().empty() && TRACE_smpi_is_enabled()) {
    simgrid::instr::Container* container =
        simgrid::instr::Container::by_name(std::string("rank-") + std::to_string(simgrid::s4u::this_actor::get_pid()));
    const papi_counter_t& counter_data = smpi_process()->papi_counters();

    for (auto const& [counter, value] : counter_data) {
      container->get_variable(counter)->set_event(simgrid::s4u::Engine::get_clock(), static_cast<double>(value));
    }
  }
#endif

  simgrid::smpi::utils::add_benched_time(xbt_os_timer_elapsed(timer));
}

/* Private sleep function used by smpi_sleep(), smpi_usleep() and friends */
static void private_sleep(double secs)
{
  const SmpiBenchGuard suspend_bench;

  XBT_DEBUG("Sleep for: %lf secs", secs);
  aid_t pid = simgrid::s4u::this_actor::get_pid();
  TRACE_smpi_sleeping_in(pid, secs);
  simgrid::s4u::this_actor::sleep_for(secs);
  TRACE_smpi_sleeping_out(pid);
}

unsigned int smpi_sleep(unsigned int secs)
{
  if (not smpi_process())
    return sleep(secs);
  private_sleep(secs);
  return 0;
}

int smpi_usleep(useconds_t usecs)
{
  if (not smpi_process())
    return usleep(usecs);
  private_sleep(static_cast<double>(usecs) / 1e6);
  return 0;
}

#if _POSIX_TIMERS > 0
int smpi_nanosleep(const struct timespec* tp, struct timespec* t)
{
  if (not smpi_process())
    return nanosleep(tp,t);
  private_sleep(static_cast<double>(tp->tv_sec) + static_cast<double>(tp->tv_nsec) / 1e9);
  return 0;
}
#endif

int smpi_gettimeofday(struct timeval* tv, struct timezone* tz)
{
  if (not smpi_process()->initialized() || smpi_process()->finalized() || smpi_process()->sampling())
    return gettimeofday(tv, tz);

  const SmpiBenchGuard suspend_bench;
  if (tv) {
    double now   = simgrid::s4u::Engine::get_clock();
    double secs  = trunc(now);
    double usecs = (now - secs) * 1e6;
    tv->tv_sec   = static_cast<time_t>(secs);
    tv->tv_usec  = static_cast<decltype(tv->tv_usec)>(usecs); // suseconds_t
  }
  if (smpi_wtime_sleep > 0)
    simgrid::s4u::this_actor::sleep_for(smpi_wtime_sleep);
  return 0;
}

#if _POSIX_TIMERS > 0
int smpi_clock_gettime(clockid_t clk_id, struct timespec* tp)
{
  if (not tp) {
    errno = EFAULT;
    return -1;
  }
  if (not smpi_process()->initialized() || smpi_process()->finalized() || smpi_process()->sampling())
    return clock_gettime(clk_id, tp);
  //there is only one time in SMPI, so clk_id is ignored.
  const SmpiBenchGuard suspend_bench;
  double now   = simgrid::s4u::Engine::get_clock();
  double secs  = trunc(now);
  double nsecs = (now - secs) * 1e9;
  tp->tv_sec   = static_cast<time_t>(secs);
  tp->tv_nsec  = static_cast<long int>(nsecs);
  if (smpi_wtime_sleep > 0)
    simgrid::s4u::this_actor::sleep_for(smpi_wtime_sleep);
  return 0;
}
#endif

double smpi_mpi_wtime()
{
  double time;
  if (smpi_process()->initialized() && not smpi_process()->finalized() && not smpi_process()->sampling()) {
    const SmpiBenchGuard suspend_bench;
    time = simgrid::s4u::Engine::get_clock();
    if (smpi_wtime_sleep > 0)
      simgrid::s4u::this_actor::sleep_for(smpi_wtime_sleep);
  } else {
    time = simgrid::s4u::Engine::get_clock();
  }
  return time;
}

// Used by Akypuera (https://github.com/schnorr/akypuera)
unsigned long long smpi_rastro_resolution ()
{
  const SmpiBenchGuard suspend_bench;
  return static_cast<unsigned long long>(1.0 / sg_precision_timing);
}

unsigned long long smpi_rastro_timestamp ()
{
  const SmpiBenchGuard suspend_bench;
  return static_cast<unsigned long long>(simgrid::s4u::Engine::get_clock() / sg_precision_timing);
}

/* ****************************** Functions related to the SMPI_SAMPLE_ macros ************************************/
namespace {
class SampleLocation : public std::string {
public:
  SampleLocation(bool global, const char* file, const char* tag) : std::string(std::string(file) + ":" + std::string(tag))
  {
    if (not global)
      this->append(":" + std::to_string(simgrid::s4u::this_actor::get_pid()));
  }
};

class LocalData {
public:
  double threshold; /* maximal stderr requested (if positive) */
  double relstderr; /* observed stderr so far */
  double mean;      /* mean of benched times, to be used if the block is disabled */
  double sum;       /* sum of benched times (to compute the mean and stderr) */
  double sum_pow2;  /* sum of the square of the benched times (to compute the stderr) */
  int iters;        /* amount of requested iterations */
  int count;        /* amount of iterations done so far */
  bool benching;    /* true: we are benchmarking; false: we have enough data, no bench anymore */

  bool need_more_benchs() const;
};

bool LocalData::need_more_benchs() const
{
  bool res = (count < iters) && (threshold < 0.0 || count < 2 ||          // not enough data
                                                  relstderr >= threshold); // stderr too high yet
  XBT_DEBUG("%s (count:%d iter:%d stderr:%f thres:%f mean:%fs)",
            (res ? "need more data" : "enough benchs"), count, iters, relstderr, threshold, mean);
  return res;
}

std::unordered_map<SampleLocation, LocalData, std::hash<std::string>> samples;
}

int smpi_sample_cond(int global, const char* file, const char* tag, int iters, double threshold, int iter_count)
{
  SampleLocation loc(global, file, tag);
  if (not smpi_process()->sampling()) { /* Only at first call when benchmarking, skip for next ones */
    smpi_bench_end();     /* Take time from previous, unrelated computation into account */
    smpi_process()->set_sampling(1);
  }

  auto [sample, inserted] = samples.try_emplace(loc,
                                                LocalData{
                                                    threshold, // threshold
                                                    0.0,       // relstderr
                                                    0.0,       // mean
                                                    0.0,       // sum
                                                    0.0,       // sum_pow2
                                                    iters,     // iters
                                                    0,         // count
                                                    true       // benching (if we have no data, we need at least one)
                                                });
  LocalData& data         = sample->second;

  if (inserted) {
    XBT_DEBUG("XXXXX First time ever on benched nest %s.", loc.c_str());
    xbt_assert(threshold > 0 || iters > 0,
        "You should provide either a positive amount of iterations to bench, or a positive maximal stderr (or both)");
  } else {
    if (data.iters != iters || data.threshold != threshold) {
      XBT_ERROR("Asked to bench block %s with different settings %d, %f is not %d, %f. "
                "How did you manage to give two numbers at the same line??",
                loc.c_str(), data.iters, data.threshold, iters, threshold);
      THROW_IMPOSSIBLE;
    }

    // if we already have some data, check whether we should get one more bench or whether we should emulate
    // the computation instead
    data.benching = data.need_more_benchs();
    XBT_DEBUG("XXXX Re-entering the benched nest %s. %s", loc.c_str(),
              (data.benching ? "more benching needed" : "we have enough data, skip computes"));
  }

  XBT_DEBUG("sample cond %s %d", loc.c_str(), iter_count);
  if (data.benching) {
    // we need to run a new bench
    XBT_DEBUG("benchmarking: count:%d iter:%d stderr:%f thres:%f; mean:%f; total:%f",
              data.count, data.iters, data.relstderr, data.threshold, data.mean, data.sum);
    smpi_bench_begin();
  } else {
    // Enough data, no more bench (either we got enough data from previous visits to this benched nest, or we just
    //ran one bench and need to bail out now that our job is done). Just sleep instead
    if (not data.need_more_benchs()){
      XBT_DEBUG("No benchmark (either no need, or just ran one): count (%d) >= iter (%d) (or <2) or stderr (%f) < thres (%f), or thresh is negative and ignored. "
              "Mean is %f, will be injected %d times",
              data.count, data.iters, data.relstderr, data.threshold, data.mean, iter_count);

      //we ended benchmarking, let's inject all the time, now, and fast forward out of the loop.
      smpi_process()->set_sampling(0);
      smpi_execute(data.mean*iter_count);
      smpi_bench_begin();
      return 0;
    } else {
      XBT_DEBUG("Skipping - Benchmark already performed - accumulating time");
      xbt_os_threadtimer_start(smpi_process()->timer());
    }
  }
  return 1;
}

void smpi_sample_iter(int global, const char* file, const char* tag)
{
  SampleLocation loc(global, file, tag);

  XBT_DEBUG("sample iter %s", loc.c_str());
  auto sample = samples.find(loc);
  xbt_assert(sample != samples.end(),
             "Y U NO use SMPI_SAMPLE_* macros? Stop messing directly with smpi_sample_* functions!");
  LocalData& data = sample->second;
  xbt_assert(data.benching);

  // ok, benchmarking this loop is over
  xbt_os_threadtimer_stop(smpi_process()->timer());

  // update the stats
  data.count++;
  double period  = xbt_os_timer_elapsed(smpi_process()->timer());
  data.sum      += period;
  data.sum_pow2 += period * period;
  double n       = data.count;
  data.mean      = data.sum / n;
  data.relstderr = sqrt((data.sum_pow2 / n) - (data.mean * data.mean)) / data.mean;

  XBT_DEBUG("Average mean after %d steps is %f, relative standard error is %f (sample was %f)",
            data.count, data.mean, data.relstderr, period);
}

int smpi_sample_exit(int global, const char *file, const char* tag, int iter_count){
  if (smpi_process()->sampling()){
    SampleLocation loc(global, file, tag);

    XBT_DEBUG("sample exit %s", loc.c_str());
    auto sample = samples.find(loc);
    xbt_assert(sample != samples.end(),
               "Y U NO use SMPI_SAMPLE_* macros? Stop messing directly with smpi_sample_* functions!");

    if (smpi_process()->sampling()){//end of loop, but still sampling needed
      const LocalData& data = sample->second;
      smpi_process()->set_sampling(0);
      smpi_execute(data.mean * iter_count);
      smpi_bench_begin();
    }
  }
  return 0;
}

smpi_trace_call_location_t* smpi_trace_get_call_location()
{
  return smpi_process()->call_location();
}

void smpi_trace_set_call_location(const char* file, const int line, const char* call_name)
{
  smpi_trace_call_location_t* loc = smpi_process()->call_location();

  loc->previous_filename   = loc->filename;
  loc->previous_linenumber = loc->linenumber;
  if(not smpi_cfg_trace_call_use_absolute_path())
    loc->filename = simgrid::xbt::Path(file).get_base_name();
  else
    loc->filename = file;
  std::replace(loc->filename.begin(), loc->filename.end(), ' ', '_');
  loc->linenumber = line;
  loc->func_call  = call_name;
}

/** Required for Fortran bindings */
void smpi_trace_set_call_location_(const char* file, const int* line, const char* call_name)
{
  smpi_trace_set_call_location(file, *line, call_name);
}

/** Required for Fortran if -fsecond-underscore is activated */
void smpi_trace_set_call_location__(const char* file, const int* line, const char* call_name)
{
  smpi_trace_set_call_location(file, *line, call_name);
}

void smpi_bench_destroy()
{
  samples.clear();
}

int smpi_getopt_long_only (int argc,  char *const *argv,  const char *options,
                      const struct option * long_options, int *opt_index)
{
  if (smpi_process())
    optind = smpi_process()->get_optind();
  int ret = getopt_long_only (argc,  argv,  options, long_options, opt_index);
  if (smpi_process())
    smpi_process()->set_optind(optind);
  return ret;
}

int smpi_getopt_long (int argc,  char *const *argv,  const char *options,
                      const struct option * long_options, int *opt_index)
{
  if (smpi_process())
    optind = smpi_process()->get_optind();
  int ret = getopt_long (argc,  argv,  options, long_options, opt_index);
  if (smpi_process())
    smpi_process()->set_optind(optind);
  return ret;
}

int smpi_getopt (int argc,  char *const *argv,  const char *options)
{
  if (smpi_process())
    optind = smpi_process()->get_optind();
  int ret = getopt (argc,  argv,  options);
  if (smpi_process())
    smpi_process()->set_optind(optind);
  return ret;
}

pid_t smpi_getpid(){
  return static_cast<pid_t>(simgrid::s4u::this_actor::get_pid());
}

#define SIZE 2000

double smpi_autobench()
{
  XBT_INFO("You requested an automatic benchmark of this machine. This may take some time.");

  // Allocate matrices
  double* A = (double*)malloc(SIZE * SIZE * sizeof(double));
  double* B = (double*)malloc(SIZE * SIZE * sizeof(double));
  double* C = (double*)malloc(SIZE * SIZE * sizeof(double));

  // Fill in matrix values
  for (int i = 0; i < SIZE * SIZE; i++) {
    A[i] = 2.0 * i / (SIZE * SIZE);
    B[i] = -1.0 * i / (SIZE * SIZE);
    C[i] = 0.0;
  }

  xbt_os_timer_t timer = xbt_os_timer_new();

  // Multiply the matrices
  xbt_os_threadtimer_start(timer);
  for (int i = 0; i < SIZE; i++)
    for (int k = 0; k < SIZE; k++)
      for (int j = 0; j < SIZE; j++)
        C[i * SIZE + j] += A[i * SIZE + k] * B[k * SIZE + j];
  xbt_os_threadtimer_stop(timer);

  // Compute the sum (to avoid compiler optimization)
  double sum = 0;
  for (int i = 0; i < SIZE * SIZE; i++)
    sum += C[i];

  double elapsed     = xbt_os_timer_elapsed(timer);
  // flop count is one add and one mul per element, the final reduction is left out of the bench
  double number_flop = (2.0 * SIZE * SIZE * SIZE);

  free(A);
  free(B);
  free(C);

  // Print the wall-clock time and the sum (to avoid compiler optimization)
  XBT_INFO("Automatic calibration ended after %lf seconds (for about 24Gflop). Use --cfg=smpi/host-speed:%.3lfGf next "
           "time on this machine. (ignorable sum: %d)",
           elapsed, number_flop / elapsed / (1000 * 1000 * 1000), (int)sum);

  return number_flop / elapsed;
}
