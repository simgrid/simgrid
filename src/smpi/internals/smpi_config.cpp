/* Copyright (c) 2008-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include "smpi_config.hpp"
#include "include/xbt/config.hpp"
#include "mc/mc.h"
#include "private.hpp"
#include "smpi_coll.hpp"
#include "src/simix/smx_private.hpp"
#include "xbt/parse_units.hpp"

#include <cfloat> /* DBL_MAX */
#include <boost/algorithm/string.hpp> /* trim */
#include <boost/tokenizer.hpp>

#if SIMGRID_HAVE_MC
#include "src/mc/mc_config.hpp"
#endif

#if defined(__APPLE__)
# include <AvailabilityMacros.h>
# ifndef MAC_OS_X_VERSION_10_12
#   define MAC_OS_X_VERSION_10_12 101200
# endif
constexpr bool HAVE_WORKING_MMAP = (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12);
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__sun) || defined(__HAIKU__)
constexpr bool HAVE_WORKING_MMAP = false;
#else
constexpr bool HAVE_WORKING_MMAP = true;
#endif

SharedMallocType _smpi_cfg_shared_malloc = SharedMallocType::GLOBAL;
SmpiPrivStrategies _smpi_cfg_privatization = SmpiPrivStrategies::NONE;
double _smpi_cfg_host_speed;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_config, smpi, "Logging specific to SMPI (config)");

simgrid::config::Flag<std::string> _smpi_cfg_host_speed_string{
    "smpi/host-speed", "Speed of the host running the simulation (in flop/s). Used to bench the operations.", "20000f",
    [](const std::string& str) {
      _smpi_cfg_host_speed = xbt_parse_get_speed("smpi/host-speed", 1, str, "option smpi/host-speed");
      xbt_assert(_smpi_cfg_host_speed > 0.0, "Invalid value (%s) for 'smpi/host-speed': it must be positive.",
                 _smpi_cfg_host_speed_string.get().c_str());
    }};

simgrid::config::Flag<bool> _smpi_cfg_simulate_computation{
  "smpi/simulate-computation", "Whether the computational part of the simulated application should be simulated.",
   true};
simgrid::config::Flag<std::string> _smpi_cfg_shared_malloc_string{
  "smpi/shared-malloc", "Whether SMPI_SHARED_MALLOC is enabled. Disable it for debugging purposes.", "global", 
  [](const std::string& val) {   
    if ((val == "yes") || (val == "1") || (val == "on") || (val == "global")) {
      _smpi_cfg_shared_malloc = SharedMallocType::GLOBAL;
    } else if (val == "local") {
      _smpi_cfg_shared_malloc = SharedMallocType::LOCAL;
    } else if ((val == "no") || (val == "0") || (val == "off")) {
      _smpi_cfg_shared_malloc = SharedMallocType::NONE;
    } else {
      xbt_die("Invalid value '%s' for option smpi/shared-malloc. Possible values: 'on' or 'global', 'local', 'off'",
      val.c_str());
    } 
  } };

simgrid::config::Flag<double> _smpi_cfg_cpu_threshold{
  "smpi/cpu-threshold", "Minimal computation time (in seconds) not discarded, or -1 for infinity.", 1e-6,
  [](const double& val){
    if (val < 0)
      _smpi_cfg_cpu_threshold = DBL_MAX;
  }};

simgrid::config::Flag<int> _smpi_cfg_async_small_thresh{"smpi/async-small-thresh",
                                                        "Maximal size of messages that are to be sent asynchronously, without waiting for the receiver",
                                                        0};
simgrid::config::Flag<int> _smpi_cfg_detached_send_thresh{"smpi/send-is-detached-thresh",
                                                          "Threshold of message size where MPI_Send stops behaving like MPI_Isend and becomes MPI_Ssend", 
                                                          65536};
simgrid::config::Flag<bool> _smpi_cfg_grow_injected_times{"smpi/grow-injected-times",
                                                          "Whether we want to make the injected time in MPI_Iprobe and MPI_Test grow, to "
                                                          "allow faster simulation. This can make simulation less precise, though.",
                                                          true};
simgrid::config::Flag<double> _smpi_cfg_iprobe_cpu_usage{"smpi/iprobe-cpu-usage",
                                                        "Maximum usage of CPUs by MPI_Iprobe() calls. We've observed that MPI_Iprobes "
                                                        "consume significantly less power than the maximum of a specific application. "
                                                        "This value is then (Iprobe_Usage/Max_Application_Usage).",
                                                        1.0};

simgrid::config::Flag<bool>  _smpi_cfg_trace_call_location{"smpi/trace-call-location",
                                                           "Should filename and linenumber of MPI calls be traced?", false};
simgrid::config::Flag<bool> _smpi_cfg_trace_call_use_absolute_path{"smpi/trace-call-use-absolute-path",
                                                                   "Should filenames for trace-call tracing be absolute or not?", false};
simgrid::config::Flag<std::string> _smpi_cfg_comp_adjustment_file{"smpi/comp-adjustment-file",
    "A file containing speedups or slowdowns for some parts of the code.", 
    "", [](const std::string& filename){
      if (not filename.empty()) {
        std::ifstream fstream(filename);
        xbt_assert(fstream.is_open(), "Could not open file %s. Does it exist?", filename.c_str());
        std::string line;
        using Tokenizer = boost::tokenizer<boost::escaped_list_separator<char>>;
        std::getline(fstream, line); // Skip the header line
        while (std::getline(fstream, line)) {
          Tokenizer tok(line);
          Tokenizer::iterator it  = tok.begin();
          Tokenizer::iterator end = std::next(tok.begin());
          std::string location = *it;
          boost::trim(location);
          location2speedup.insert(std::pair<std::string, double>(location, std::stod(*end)));
        }
      }
    }};

simgrid::config::Flag<bool> _smpi_cfg_default_errhandler_is_error{
  "smpi/errors-are-fatal", "Whether MPI errors are fatal or just return. Default is true", true };
#if HAVE_PAPI
  simgrid::config::Flag<std::string> _smpi_cfg_papi_events_file{"smpi/papi-events",
                                                                "This switch enables tracking the specified counters with PAPI", ""};
#endif

simgrid::config::Flag<double> _smpi_cfg_auto_shared_malloc_thresh("smpi/auto-shared-malloc-thresh",
                                                                  "Threshold size for the automatic sharing of memory",
                                                                  0);

simgrid::config::Flag<bool> _smpi_cfg_display_alloc("smpi/display-allocs",
                                                    "Whether we should display a memory allocations analysis after simulation.",
                                                     false);

simgrid::config::Flag<int> _smpi_cfg_list_leaks("smpi/list-leaks",
                                                "Whether we should display the n first MPI handle leaks (addresses and type only) after simulation",
                                                -1);

double smpi_cfg_host_speed(){
  return _smpi_cfg_host_speed;
}

bool smpi_cfg_simulate_computation(){
  return _smpi_cfg_simulate_computation;
}

SharedMallocType smpi_cfg_shared_malloc(){
  return _smpi_cfg_shared_malloc;
}

double smpi_cfg_cpu_thresh(){
  return _smpi_cfg_cpu_threshold;
}

SmpiPrivStrategies smpi_cfg_privatization(){
  return _smpi_cfg_privatization;
}

int smpi_cfg_async_small_thresh(){
  return _smpi_cfg_async_small_thresh;
}

int smpi_cfg_detached_send_thresh(){
  return _smpi_cfg_detached_send_thresh;
}

bool smpi_cfg_grow_injected_times(){
  return _smpi_cfg_grow_injected_times;
}

double smpi_cfg_iprobe_cpu_usage(){
  return _smpi_cfg_iprobe_cpu_usage;
}

bool smpi_cfg_trace_call_location(){
  return _smpi_cfg_trace_call_location;
}

bool smpi_cfg_trace_call_use_absolute_path(){
  return _smpi_cfg_trace_call_use_absolute_path;
}

bool smpi_cfg_display_alloc(){
  return _smpi_cfg_list_leaks != -1 ? true : _smpi_cfg_display_alloc;
}

std::string smpi_cfg_comp_adjustment_file(){
  return _smpi_cfg_comp_adjustment_file;
}
#if HAVE_PAPI
std::string smpi_cfg_papi_events_file(){
  return _smpi_cfg_papi_events_file;
}
#endif
double smpi_cfg_auto_shared_malloc_thresh(){
  return _smpi_cfg_auto_shared_malloc_thresh;
}

// public version declared in smpi.h (without parameter, and with C linkage)
void smpi_init_options()
{
  smpi_init_options_internal(false);
}

void smpi_init_options_internal(bool called_by_smpi_main)
{
  static bool smpi_options_initialized = false;
  static bool running_with_smpi_main   = false;

  if (called_by_smpi_main)
    running_with_smpi_main = true;

  // return if already called
  if (smpi_options_initialized)
    return;
  simgrid::config::declare_flag<bool>("smpi/display-timing", "Whether we should display the timing after simulation.", false);
  simgrid::config::declare_flag<bool>("smpi/keep-temps", "Whether we should keep the generated temporary files.", false);
  simgrid::config::declare_flag<std::string>("smpi/tmpdir", "tmp dir for dlopen files", "/tmp");

  simgrid::config::declare_flag<std::string>("smpi/coll-selector", "Which collective selector to use", "default");
  simgrid::config::declare_flag<std::string>("smpi/gather", "Which collective to use for gather", "");
  simgrid::config::declare_flag<std::string>("smpi/allgather", "Which collective to use for allgather", "");
  simgrid::config::declare_flag<std::string>("smpi/barrier", "Which collective to use for barrier", "");
  simgrid::config::declare_flag<std::string>("smpi/reduce_scatter", "Which collective to use for reduce_scatter", "");
  simgrid::config::declare_flag<std::string>("smpi/scatter", "Which collective to use for scatter", "");
  simgrid::config::declare_flag<std::string>("smpi/allgatherv", "Which collective to use for allgatherv", "");
  simgrid::config::declare_flag<std::string>("smpi/allreduce", "Which collective to use for allreduce", "");
  simgrid::config::declare_flag<std::string>("smpi/alltoall", "Which collective to use for alltoall", "");
  simgrid::config::declare_flag<std::string>("smpi/alltoallv", "Which collective to use for alltoallv", "");
  simgrid::config::declare_flag<std::string>("smpi/bcast", "Which collective to use for bcast", "");
  simgrid::config::declare_flag<std::string>("smpi/reduce", "Which collective to use for reduce", "");

  const char* default_privatization = std::getenv("SMPI_PRIVATIZATION");
  if (default_privatization == nullptr)
    default_privatization = "no";

  simgrid::config::declare_flag<std::string>(
      "smpi/privatization", "How we should privatize global variable at runtime (no, yes, mmap, dlopen).",
      default_privatization, [](const std::string& smpi_privatize_option) {
        if (smpi_privatize_option == "no" || smpi_privatize_option == "0")
          _smpi_cfg_privatization = SmpiPrivStrategies::NONE;
        else if (smpi_privatize_option == "yes" || smpi_privatize_option == "1")
          _smpi_cfg_privatization = SmpiPrivStrategies::DEFAULT;
        else if (smpi_privatize_option == "mmap")
          _smpi_cfg_privatization = SmpiPrivStrategies::MMAP;
        else if (smpi_privatize_option == "dlopen")
          _smpi_cfg_privatization = SmpiPrivStrategies::DLOPEN;
        else
          xbt_die("Invalid value for smpi/privatization: '%s'", smpi_privatize_option.c_str());

        if (not running_with_smpi_main) {
          XBT_DEBUG("Running without smpi_main(); disable smpi/privatization.");
          _smpi_cfg_privatization = SmpiPrivStrategies::NONE;
        }
        if (not HAVE_WORKING_MMAP && _smpi_cfg_privatization == SmpiPrivStrategies::MMAP) {
          XBT_INFO("mmap privatization is broken on this platform, switching to dlopen privatization instead.");
          _smpi_cfg_privatization = SmpiPrivStrategies::DLOPEN;
        }
      });

  simgrid::config::declare_flag<std::string>("smpi/privatize-libs", 
                                            "Add libraries (; separated) to privatize (libgfortran for example)."
                                            "You need to provide the full names of the files (libgfortran.so.4), or its full path", 
                                            "");
  simgrid::config::declare_flag<double>("smpi/shared-malloc-blocksize",
                                        "Size of the bogus file which will be created for global shared allocations", 
                                        1UL << 20);
  simgrid::config::declare_flag<std::string>("smpi/shared-malloc-hugepage",
                                             "Path to a mounted hugetlbfs, to use huge pages with shared malloc.", 
                                             "");

  simgrid::config::declare_flag<std::string>(
      "smpi/os", "Small messages timings (MPI_Send minimum time for small messages)", "0:0:0:0:0");
  simgrid::config::declare_flag<std::string>(
      "smpi/ois", "Small messages timings (MPI_Isend minimum time for small messages)", "0:0:0:0:0");
  simgrid::config::declare_flag<std::string>(
      "smpi/or", "Small messages timings (MPI_Recv minimum time for small messages)", "0:0:0:0:0");

  simgrid::config::declare_flag<bool>("smpi/finalization-barrier", "Do we add a barrier in MPI_Finalize or not", false);

  smpi_options_initialized = true;
}

void smpi_check_options()
{
#if SIMGRID_HAVE_MC
  if (MC_is_active()) {
    if (_sg_mc_buffering == "zero")
      simgrid::config::set_value<int>("smpi/send-is-detached-thresh", 0);
    else if (_sg_mc_buffering == "infty")
      simgrid::config::set_value<int>("smpi/send-is-detached-thresh", INT_MAX);
    else
      THROW_IMPOSSIBLE;
  }
#endif

  xbt_assert(smpi_cfg_async_small_thresh() <= smpi_cfg_detached_send_thresh(),
             "smpi/async-small-thresh (=%d) should be smaller or equal to smpi/send-is-detached-thresh (=%d)",
             smpi_cfg_async_small_thresh(),
             smpi_cfg_detached_send_thresh());

  if (simgrid::config::is_default("smpi/host-speed") && not MC_is_active()) {
    XBT_INFO("You did not set the power of the host running the simulation.  "
             "The timings will certainly not be accurate.  "
             "Use the option \"--cfg=smpi/host-speed:<flops>\" to set its value.  "
             "Check "
             "https://simgrid.org/doc/latest/Configuring_SimGrid.html#automatic-benchmarking-of-smpi-code for more "
             "information.");
  }

  simgrid::smpi::colls::set_collectives();
  simgrid::smpi::colls::smpi_coll_cleanup_callback = nullptr;
}

