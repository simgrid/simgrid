/* Copyright (c) 2009-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* sg_config: configuration infrastructure for the simulation world         */

#include "simgrid/sg_config.h"
#include "instr/instr_interface.h"
#include "mc/mc.h"
#include "simgrid/instr.h"
#include "simgrid/simix.h"
#include "simgrid_config.h" /* what was compiled in? */
#include "src/kernel/lmm/maxmin.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/surf/surf_interface.hpp"
#include "surf/surf.hpp"
#include "xbt/config.h"
#include "xbt/config.hpp"
#include "xbt/log.h"
#include "xbt/mallocator.h"
#include "xbt/misc.h"
#include "xbt/sysdep.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_config, surf, "About the configuration of SimGrid");


/* 0: beginning of time (config cannot be changed yet);
 * 1: initialized: cfg_set created (config can now be changed);
 * 2: configured: command line parsed and config part of platform file was
 *    integrated also, platform construction ongoing or done.
 *    (Config cannot be changed anymore!)
 */
int _sg_cfg_init_status = 0;

/* instruct the upper layer (simix or simdag) to exit as soon as possible */
int _sg_cfg_exit_asap = 0;

#define sg_cfg_exit_early() do { _sg_cfg_exit_asap = 1; return; } while (0)

/* Parse the command line, looking for options */
static void sg_config_cmd_line(int *argc, char **argv)
{
  int shall_exit = 0;
  int i;
  int j;

  for (j = i = 1; i < *argc; i++) {
    if (not strncmp(argv[i], "--cfg=", strlen("--cfg="))) {
      char *opt = strchr(argv[i], '=');
      opt++;

      xbt_cfg_set_parse(opt);
      XBT_DEBUG("Did apply '%s' as config setting", opt);
    } else if (not strcmp(argv[i], "--version")) {
      printf("%s\n", SIMGRID_VERSION_STRING);
      shall_exit = 1;
    } else if (not strcmp(argv[i], "--cfg-help") || not strcmp(argv[i], "--help")) {
      printf("Description of the configuration accepted by this simulator:\n");
      xbt_cfg_help();
      printf(
          "\n"
          "Each of these configurations can be used by adding\n"
          "    --cfg=<option name>:<option value>\n"
          "to the command line.\n"
          "\n"
          "For more information, please refer to:\n"
          "   --help-aliases for the list of all option aliases.\n"
          "   --help-logs and --help-log-categories for the details of logging output.\n"
          "   --help-models for a list of all models known by this simulator.\n"
          "   --help-tracing for the details of all tracing options known by this simulator.\n"
          "   --version to get SimGrid version information.\n"
          "\n"
        );
      shall_exit = 1;
    } else if (not strcmp(argv[i], "--help-aliases")) {
      printf("Here is a list of all deprecated option names, with their replacement.\n");
      xbt_cfg_aliases();
      printf("Please consider using the recent names\n");
      shall_exit = 1;
    } else if (not strcmp(argv[i], "--help-models")) {
      model_help("host", surf_host_model_description);
      printf("\n");
      model_help("CPU", surf_cpu_model_description);
      printf("\n");
      model_help("network", surf_network_model_description);
      printf("\nLong description of all optimization levels accepted by the models of this simulator:\n");
      for (int k = 0; surf_optimization_mode_description[k].name; k++)
        printf("  %s: %s\n",
               surf_optimization_mode_description[k].name,
               surf_optimization_mode_description[k].description);
      printf("Both network and CPU models have 'Lazy' as default optimization level\n\n");
      shall_exit = 1;
    } else if (not strcmp(argv[i], "--help-tracing")) {
      TRACE_help (1);
      shall_exit = 1;
    } else {
      argv[j++] = argv[i];
    }
  }
  if (j < *argc) {
    argv[j] = nullptr;
    *argc = j;
  }
  if (shall_exit)
    sg_cfg_exit_early();
}

/* callback of the plugin variable */
static void _sg_cfg_cb__plugin(const char *name)
{
  xbt_assert(_sg_cfg_init_status < 2, "Cannot load a plugin after the initialization");

  std::string val = xbt_cfg_get_string(name);
  if (val.empty())
    return;

  if (val == "help") {
    model_help("plugin", surf_plugin_description);
    sg_cfg_exit_early();
  }

  int plugin_id = find_model_description(surf_plugin_description, val);
  surf_plugin_description[plugin_id].model_init_preparse();
}

/* callback of the host/model variable */
static void _sg_cfg_cb__host_model(const char *name)
{
  xbt_assert(_sg_cfg_init_status < 2, "Cannot change the model after the initialization");

  std::string val = xbt_cfg_get_string(name);
  if (val == "help") {
    model_help("host", surf_host_model_description);
    sg_cfg_exit_early();
  }

  /* Make sure that the model exists */
  find_model_description(surf_host_model_description, val);
}

/* callback of the cpu/model variable */
static void _sg_cfg_cb__cpu_model(const char *name)
{
  xbt_assert(_sg_cfg_init_status < 2, "Cannot change the model after the initialization");

  std::string val = xbt_cfg_get_string(name);
  if (val == "help") {
    model_help("CPU", surf_cpu_model_description);
    sg_cfg_exit_early();
  }

  /* New Module missing */
  find_model_description(surf_cpu_model_description, val);
}

/* callback of the cpu/model variable */
static void _sg_cfg_cb__optimization_mode(const char *name)
{
  xbt_assert(_sg_cfg_init_status < 2, "Cannot change the model after the initialization");

  std::string val = xbt_cfg_get_string(name);
  if (val == "help") {
    model_help("optimization", surf_optimization_mode_description);
    sg_cfg_exit_early();
  }

  /* New Module missing */
  find_model_description(surf_optimization_mode_description, val);
}

/* callback of the cpu/model variable */
static void _sg_cfg_cb__storage_mode(const char *name)
{
  xbt_assert(_sg_cfg_init_status < 2, "Cannot change the model after the initialization");

  std::string val = xbt_cfg_get_string(name);
  if (val == "help") {
    model_help("storage", surf_storage_model_description);
    sg_cfg_exit_early();
  }

  find_model_description(surf_storage_model_description, val);
}

/* callback of the network_model variable */
static void _sg_cfg_cb__network_model(const char *name)
{
  xbt_assert(_sg_cfg_init_status < 2, "Cannot change the model after the initialization");

  std::string val = xbt_cfg_get_string(name);
  if (val == "help") {
    model_help("network", surf_network_model_description);
    sg_cfg_exit_early();
  }

  /* New Module missing */
  find_model_description(surf_network_model_description, val);
}
/* callback to decide if we want to use the model-checking */
#include "src/xbt_modinter.h"

static void _sg_cfg_cb_model_check_replay(const char *name) {
  MC_record_path = xbt_cfg_get_string(name);
}

#if SIMGRID_HAVE_MC
extern int _sg_do_model_check_record;
static void _sg_cfg_cb_model_check_record(const char *name) {
  _sg_do_model_check_record = xbt_cfg_get_boolean(name);
}
#endif

extern int _sg_do_verbose_exit;
static void _sg_cfg_cb_verbose_exit(const char *name)
{
  _sg_do_verbose_exit = xbt_cfg_get_boolean(name);
}

extern int _sg_do_clean_atexit;
static void _sg_cfg_cb_clean_atexit(const char *name)
{
  _sg_do_clean_atexit = xbt_cfg_get_boolean(name);
}

static void _sg_cfg_cb_context_stack_size(const char *name)
{
  smx_context_stack_size_was_set = 1;
  smx_context_stack_size = xbt_cfg_get_int(name) * 1024;
}

static void _sg_cfg_cb_context_guard_size(const char *name)
{
  smx_context_guard_size_was_set = 1;
  smx_context_guard_size = xbt_cfg_get_int(name) * xbt_pagesize;
}

static void _sg_cfg_cb_contexts_nthreads(const char *name)
{
  SIMIX_context_set_nthreads(xbt_cfg_get_int(name));
}

static void _sg_cfg_cb_contexts_parallel_threshold(const char *name)
{
  SIMIX_context_set_parallel_threshold(xbt_cfg_get_int(name));
}

static void _sg_cfg_cb_contexts_parallel_mode(const char *name)
{
  std::string mode_name = xbt_cfg_get_string(name);
  if (mode_name == "posix") {
    SIMIX_context_set_parallel_mode(XBT_PARMAP_POSIX);
  } else if (mode_name == "futex") {
    SIMIX_context_set_parallel_mode(XBT_PARMAP_FUTEX);
  } else if (mode_name == "busy_wait") {
    SIMIX_context_set_parallel_mode(XBT_PARMAP_BUSY_WAIT);
  } else {
    xbt_die("Command line setting of the parallel synchronization mode should "
            "be one of \"posix\", \"futex\" or \"busy_wait\"");
  }
}

static void _sg_cfg_cb__surf_network_crosstraffic(const char *name)
{
  sg_network_crosstraffic = xbt_cfg_get_boolean(name);
}

/* build description line with possible values */
static void describe_model(char *result,int resultsize,
                           const s_surf_model_description_t model_description[],
                           const char *name,
                           const char *description)
{
  result[0] = '\0';
  char *p = result;
  p += snprintf(result,resultsize-1, "%s. Possible values: %s", description,
            model_description[0].name ? model_description[0].name : "n/a");
  for (int i = 1; model_description[i].name; i++)
    p += snprintf(p,resultsize-(p-result)-1, ", %s", model_description[i].name);
  p += snprintf(p,resultsize-(p-result)-1, ".\n       (use 'help' as a value to see the long description of each %s)", name);

  xbt_assert(p<result+resultsize-1,"Buffer too small to display the model description of %s",name);
}

/* create the config set, register what should be and parse the command line*/
void sg_config_init(int *argc, char **argv)
{
  const int descsize = 1024;
  char description[descsize];

  /* Create the configuration support */
  if (_sg_cfg_init_status != 0) { /* Only create stuff if not already inited */
    XBT_WARN("Call to sg_config_init() after initialization ignored");
    return;
  }

  /* Plugins configuration */
  describe_model(description, descsize, surf_plugin_description, "plugin", "The plugins");
  xbt_cfg_register_string("plugin", "", &_sg_cfg_cb__plugin, description);

  describe_model(description, descsize, surf_cpu_model_description, "model", "The model to use for the CPU");
  xbt_cfg_register_string("cpu/model", "Cas01", &_sg_cfg_cb__cpu_model, description);

  describe_model(description, descsize, surf_optimization_mode_description, "optimization mode",
                 "The optimization modes to use for the CPU");
  xbt_cfg_register_string("cpu/optim", "Lazy", &_sg_cfg_cb__optimization_mode, description);

  describe_model(description, descsize, surf_storage_model_description, "model", "The model to use for the storage");
  xbt_cfg_register_string("storage/model", "default", &_sg_cfg_cb__storage_mode, description);

  describe_model(description, descsize, surf_network_model_description, "model", "The model to use for the network");
  xbt_cfg_register_string("network/model", "LV08", &_sg_cfg_cb__network_model, description);

  describe_model(description, descsize, surf_optimization_mode_description, "optimization mode",
                 "The optimization modes to use for the network");
  xbt_cfg_register_string("network/optim", "Lazy", &_sg_cfg_cb__optimization_mode, description);

  describe_model(description, descsize, surf_host_model_description, "model", "The model to use for the host");
  xbt_cfg_register_string("host/model", "default", &_sg_cfg_cb__host_model, description);

  sg_tcp_gamma = 4194304.0;
  simgrid::config::bindFlag(sg_tcp_gamma, {"network/TCP-gamma", "network/TCP_gamma"},
                            "Size of the biggest TCP window (cat /proc/sys/net/ipv4/tcp_[rw]mem for recv/send window; "
                            "Use the last given value, which is the max window size)");

  simgrid::config::bindFlag(sg_surf_precision, "surf/precision",
                            "Numerical precision used when updating simulation times (in seconds)");

  simgrid::config::bindFlag(sg_maxmin_precision, "maxmin/precision",
                            "Numerical precision used when computing resource sharing (in flops/sec or bytes/sec)");

  simgrid::config::bindFlag(sg_concurrency_limit, "maxmin/concurrency-limit",
                            "Maximum number of concurrent variables in the maxmim system. Also limits the number of "
                            "processes on each host, at higher level. (default: -1 means no such limitation)");
  xbt_cfg_register_alias("maxmin/concurrency-limit", "maxmin/concurrency_limit");

  /* The parameters of network models */

  sg_latency_factor = 13.01; // comes from the default LV08 network model
  simgrid::config::bindFlag(sg_latency_factor, {"network/latency-factor", "network/latency_factor"},
                            "Correction factor to apply to the provided latency (default value set by network model)");

  sg_bandwidth_factor = 0.97; // comes from the default LV08 network model
  simgrid::config::bindFlag(
      sg_bandwidth_factor, {"network/bandwidth-factor", "network/bandwidth_factor"},
      "Correction factor to apply to the provided bandwidth (default value set by network model)");

  sg_weight_S_parameter = 20537; // comes from the default LV08 network model
  simgrid::config::bindFlag(
      sg_weight_S_parameter, {"network/weight-S", "network/weight_S"},
      "Correction factor to apply to the weight of competing streams (default value set by network model)");

  /* Inclusion path */
  simgrid::config::declareFlag<std::string>("path", "Lookup path for inclusions in platform and deployment XML files",
                                            "", [](std::string const& path) {
                                              if (path[0] != '\0') {
                                                surf_path.push_back(path);
                                              }
                                            });

  xbt_cfg_register_boolean("cpu/maxmin-selective-update", "no", nullptr, "Update the constraint set propagating "
                                                                         "recursively to others constraints (off by "
                                                                         "default when optim is set to lazy)");
  xbt_cfg_register_alias("cpu/maxmin-selective-update", "cpu/maxmin_selective_update");
  xbt_cfg_register_boolean("network/maxmin-selective-update", "no", nullptr, "Update the constraint set propagating "
                                                                             "recursively to others constraints (off "
                                                                             "by default when optim is set to lazy)");
  xbt_cfg_register_alias("network/maxmin-selective-update", "network/maxmin_selective_update");
  /* Replay (this part is enabled even if MC it disabled) */
  xbt_cfg_register_string("model-check/replay", nullptr, _sg_cfg_cb_model_check_replay,
                          "Model-check path to replay (as reported by SimGrid when a violation is reported)");

#if SIMGRID_HAVE_MC
    /* do model-checking-record */
    xbt_cfg_register_boolean("model-check/record", "no", _sg_cfg_cb_model_check_record, "Record the model-checking paths");

    xbt_cfg_register_int("model-check/checkpoint", 0, _mc_cfg_cb_checkpoint,
        "Specify the amount of steps between checkpoints during stateful model-checking (default: 0 => stateless verification). "
        "If value=1, one checkpoint is saved for each step => faster verification, but huge memory consumption; higher values are good compromises between speed and memory consumption.");

    xbt_cfg_register_boolean("model-check/sparse-checkpoint", "no", _mc_cfg_cb_sparse_checkpoint, "Use sparse per-page snapshots.");
    xbt_cfg_register_boolean("model-check/ksm", "no", _mc_cfg_cb_ksm, "Kernel same-page merging");

    xbt_cfg_register_string("model-check/property", "", _mc_cfg_cb_property,
                            "Name of the file containing the property, as formatted by the ltl2ba program.");
    xbt_cfg_register_boolean("model-check/communications-determinism", "no", _mc_cfg_cb_comms_determinism,
        "Whether to enable the detection of communication determinism");
    xbt_cfg_register_alias("model-check/communications-determinism","model-check/communications_determinism");

    xbt_cfg_register_boolean("model-check/send-determinism", "no", _mc_cfg_cb_send_determinism,
        "Enable/disable the detection of send-determinism in the communications schemes");
    xbt_cfg_register_alias("model-check/send-determinism","model-check/send_determinism");

    /* Specify the kind of model-checking reduction */
    xbt_cfg_register_string("model-check/reduction", "dpor", _mc_cfg_cb_reduce,
        "Specify the kind of exploration reduction (either none or DPOR)");
    xbt_cfg_register_boolean("model-check/timeout", "no",  _mc_cfg_cb_timeout,
        "Whether to enable timeouts for wait requests");

    xbt_cfg_register_boolean("model-check/hash", "no", _mc_cfg_cb_hash, "Whether to enable state hash for state comparison (experimental)");
    xbt_cfg_register_boolean("model-check/snapshot-fds", "no",  _mc_cfg_cb_snapshot_fds,
        "Whether file descriptors must be snapshoted (currently unusable)");
    xbt_cfg_register_alias("model-check/snapshot-fds","model-check/snapshot_fds");
    xbt_cfg_register_int("model-check/max-depth", 1000, _mc_cfg_cb_max_depth, "Maximal exploration depth (default: 1000)");
    xbt_cfg_register_alias("model-check/max-depth","model-check/max_depth");
    xbt_cfg_register_int("model-check/visited", 0, _mc_cfg_cb_visited,
        "Specify the number of visited state stored for state comparison reduction. If value=5, the last 5 visited states are stored. If value=0 (the default), all states are stored.");

    xbt_cfg_register_string("model-check/dot-output", "", _mc_cfg_cb_dot_output, "Name of dot output file corresponding to graph state");
    xbt_cfg_register_alias("model-check/dot-output","model-check/dot_output");
    xbt_cfg_register_boolean("model-check/termination", "no", _mc_cfg_cb_termination, "Whether to enable non progressive cycle detection");
#endif

    xbt_cfg_register_boolean("verbose-exit", "yes", _sg_cfg_cb_verbose_exit, "Activate the \"do nothing\" mode in Ctrl-C");

    xbt_cfg_register_int("contexts/stack-size", 8*1024, _sg_cfg_cb_context_stack_size, "Stack size of contexts in KiB");
    /* (FIXME: this is unpleasant) Reset this static variable that was altered when setting the default value. */
    smx_context_stack_size_was_set = 0;
    xbt_cfg_register_alias("contexts/stack-size","contexts/stack_size");

    /* guard size for contexts stacks in memory pages */
    xbt_cfg_register_int("contexts/guard-size",
#if defined(_WIN32) || (PTH_STACKGROWTH != -1)
        0,
#else
        1,
#endif
    _sg_cfg_cb_context_guard_size, "Guard size for contexts stacks in memory pages");
    /* No, it was not set yet (the above setdefault() changed this to 1). */
    smx_context_guard_size_was_set = 0;
    xbt_cfg_register_alias("contexts/guard-size","contexts/guard_size");
    xbt_cfg_register_int("contexts/nthreads", 1, _sg_cfg_cb_contexts_nthreads, "Number of parallel threads used to execute user contexts");

    xbt_cfg_register_int("contexts/parallel-threshold", 2, _sg_cfg_cb_contexts_parallel_threshold,
        "Minimal number of user contexts to be run in parallel (raw contexts only)");
    xbt_cfg_register_alias("contexts/parallel-threshold","contexts/parallel_threshold");

    /* synchronization mode for parallel user contexts */
#if HAVE_FUTEX_H
    xbt_cfg_register_string("contexts/synchro", "futex",     _sg_cfg_cb_contexts_parallel_mode,
        "Synchronization mode to use when running contexts in parallel (either futex, posix or busy_wait)");
#else //No futex on mac and posix is unimplememted yet
    xbt_cfg_register_string("contexts/synchro", "busy_wait", _sg_cfg_cb_contexts_parallel_mode,
        "Synchronization mode to use when running contexts in parallel (either futex, posix or busy_wait)");
#endif

    xbt_cfg_register_boolean("network/crosstraffic", "yes", _sg_cfg_cb__surf_network_crosstraffic,
        "Activate the interferences between uploads and downloads for fluid max-min models (LV08, CM02)");

    // For smpi/bw-factor and smpi/lat-factor
    // SMPI model can be used without enable_smpi, so keep this out of the ifdef.
    xbt_cfg_register_string("smpi/bw-factor",
        "65472:0.940694;15424:0.697866;9376:0.58729;5776:1.08739;3484:0.77493;1426:0.608902;732:0.341987;257:0.338112;0:0.812084", nullptr,
        "Bandwidth factors for smpi. Format: 'threshold0:value0;threshold1:value1;...;thresholdN:valueN', meaning if(size >=thresholdN ) return valueN.");
    xbt_cfg_register_alias("smpi/bw-factor","smpi/bw_factor");

    xbt_cfg_register_string("smpi/lat-factor",
        "65472:11.6436;15424:3.48845;9376:2.59299;5776:2.18796;3484:1.88101;1426:1.61075;732:1.9503;257:1.95341;0:2.01467", nullptr, "Latency factors for smpi.");
    xbt_cfg_register_alias("smpi/lat-factor","smpi/lat_factor");

    xbt_cfg_register_string("smpi/IB-penalty-factors", "0.965;0.925;1.35", nullptr,
        "Correction factor to communications using Infiniband model with contention (default value based on Stampede cluster profiling)");
    xbt_cfg_register_alias("smpi/IB-penalty-factors","smpi/IB_penalty_factors");

#if HAVE_SMPI
    xbt_cfg_register_double("smpi/host-speed", 20000.0, nullptr, "Speed of the host running the simulation (in flop/s). Used to bench the operations.");
    xbt_cfg_register_alias("smpi/host-speed","smpi/running_power");
    xbt_cfg_register_alias("smpi/host-speed","smpi/running-power");

    xbt_cfg_register_boolean("smpi/keep-temps", "no", nullptr, "Whether we should keep the generated temporary files.");

    xbt_cfg_register_boolean("smpi/display-timing", "no", nullptr, "Whether we should display the timing after simulation.");
    xbt_cfg_register_alias("smpi/display-timing", "smpi/display_timing");

    xbt_cfg_register_boolean("smpi/simulate-computation", "yes", nullptr, "Whether the computational part of the simulated application should be simulated.");
    xbt_cfg_register_alias("smpi/simulate-computation","smpi/simulate_computation");

    xbt_cfg_register_string("smpi/shared-malloc", "global", nullptr,
                            "Whether SMPI_SHARED_MALLOC is enabled. Disable it for debugging purposes.");
    xbt_cfg_register_alias("smpi/shared-malloc", "smpi/use-shared-malloc");
    xbt_cfg_register_alias("smpi/shared-malloc", "smpi/use_shared_malloc");
    xbt_cfg_register_double("smpi/shared-malloc-blocksize", 1UL << 20, nullptr, "Size of the bogus file which will be created for global shared allocations");
    xbt_cfg_register_string("smpi/shared-malloc-hugepage", "", nullptr,
                            "Path to a mounted hugetlbfs, to use huge pages with shared malloc.");

    xbt_cfg_register_double("smpi/cpu-threshold", 1e-6, nullptr, "Minimal computation time (in seconds) not discarded, or -1 for infinity.");
    xbt_cfg_register_alias("smpi/cpu-threshold", "smpi/cpu_threshold");

    xbt_cfg_register_int("smpi/async-small-thresh", 0, nullptr,
        "Maximal size of messages that are to be sent asynchronously, without waiting for the receiver");
    xbt_cfg_register_alias("smpi/async-small-thresh","smpi/async_small_thresh");
    xbt_cfg_register_alias("smpi/async-small-thresh","smpi/async_small_thres");

    xbt_cfg_register_boolean("smpi/trace-call-location", "no", nullptr, "Should filename and linenumber of MPI calls be traced?");

    xbt_cfg_register_int("smpi/send-is-detached-thresh", 65536, nullptr,
        "Threshold of message size where MPI_Send stops behaving like MPI_Isend and becomes MPI_Ssend");
    xbt_cfg_register_alias("smpi/send-is-detached-thresh","smpi/send_is_detached_thresh");
    xbt_cfg_register_alias("smpi/send-is-detached-thresh","smpi/send_is_detached_thres");

    const char* default_privatization = std::getenv("SMPI_PRIVATIZATION");
    if (default_privatization == nullptr)
      default_privatization = "no";

    xbt_cfg_register_string("smpi/privatization", default_privatization, nullptr,
                            "How we should privatize global variable at runtime (no, yes, mmap, dlopen).");

    xbt_cfg_register_alias("smpi/privatization", "smpi/privatize-global-variables");
    xbt_cfg_register_alias("smpi/privatization", "smpi/privatize_global_variables");

    xbt_cfg_register_boolean("smpi/grow-injected-times", "yes", nullptr, "Whether we want to make the injected time in MPI_Iprobe and MPI_Test grow, to allow faster simulation. This can make simulation less precise, though.");

#if HAVE_PAPI
    xbt_cfg_register_string("smpi/papi-events", nullptr, nullptr, "This switch enables tracking the specified counters with PAPI");
#endif
    xbt_cfg_register_string("smpi/comp-adjustment-file", nullptr, nullptr, "A file containing speedups or slowdowns for some parts of the code.");
    xbt_cfg_register_string("smpi/os", "0:0:0:0:0", nullptr,  "Small messages timings (MPI_Send minimum time for small messages)");
    xbt_cfg_register_string("smpi/ois", "0:0:0:0:0", nullptr, "Small messages timings (MPI_Isend minimum time for small messages)");
    xbt_cfg_register_string("smpi/or", "0:0:0:0:0", nullptr,  "Small messages timings (MPI_Recv minimum time for small messages)");

    xbt_cfg_register_double("smpi/iprobe-cpu-usage", 1, nullptr, "Maximum usage of CPUs by MPI_Iprobe() calls. We've observed that MPI_Iprobes consume significantly less power than the maximum of a specific application. This value is then (Iprobe_Usage/Max_Application_Usage).");

    xbt_cfg_register_string("smpi/coll-selector", "default", nullptr, "Which collective selector to use");
    xbt_cfg_register_alias("smpi/coll-selector","smpi/coll_selector");
    xbt_cfg_register_string("smpi/gather",        nullptr, nullptr, "Which collective to use for gather");
    xbt_cfg_register_string("smpi/allgather",     nullptr, nullptr, "Which collective to use for allgather");
    xbt_cfg_register_string("smpi/barrier",       nullptr, nullptr, "Which collective to use for barrier");
    xbt_cfg_register_string("smpi/reduce_scatter",nullptr, nullptr, "Which collective to use for reduce_scatter");
    xbt_cfg_register_alias("smpi/reduce_scatter","smpi/reduce-scatter");
    xbt_cfg_register_string("smpi/scatter",       nullptr, nullptr, "Which collective to use for scatter");
    xbt_cfg_register_string("smpi/allgatherv",    nullptr, nullptr, "Which collective to use for allgatherv");
    xbt_cfg_register_string("smpi/allreduce",     nullptr, nullptr, "Which collective to use for allreduce");
    xbt_cfg_register_string("smpi/alltoall",      nullptr, nullptr, "Which collective to use for alltoall");
    xbt_cfg_register_string("smpi/alltoallv",     nullptr, nullptr,"Which collective to use for alltoallv");
    xbt_cfg_register_string("smpi/bcast",         nullptr, nullptr, "Which collective to use for bcast");
    xbt_cfg_register_string("smpi/reduce",        nullptr, nullptr, "Which collective to use for reduce");
#endif // HAVE_SMPI

    /* Storage */

    sg_storage_max_file_descriptors = 1024;
    simgrid::config::bindFlag(sg_storage_max_file_descriptors, "storage/max_file_descriptors",
      "Maximum number of concurrently opened files per host. Default is 1024");

    /* Others */

    xbt_cfg_register_boolean("exception/cutpath", "no", nullptr,
        "Whether to cut all path information from call traces, used e.g. in exceptions.");

    xbt_cfg_register_boolean("clean-atexit", "yes", _sg_cfg_cb_clean_atexit,
        "Whether to cleanup SimGrid at exit. Disable it if your code segfaults after its end.");
    xbt_cfg_register_alias("clean-atexit","clean_atexit");

    if (surf_path.empty())
      xbt_cfg_setdefault_string("path", "./");

    _sg_cfg_init_status = 1;

    sg_config_cmd_line(argc, argv);

    xbt_mallocator_initialization_is_done(SIMIX_context_is_parallel());
}

void sg_config_finalize()
{
  if (not _sg_cfg_init_status)
    return;                     /* Not initialized yet. Nothing to do */

  xbt_cfg_free(&simgrid_config);
  _sg_cfg_init_status = 0;
}
