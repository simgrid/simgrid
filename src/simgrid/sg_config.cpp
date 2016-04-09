/* Copyright (c) 2009-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* sg_config: configuration infrastructure for the simulation world       */

#include "xbt/misc.h"
#include "xbt/config.h"
#include "xbt/log.h"
#include "xbt/mallocator.h"
#include "xbt/str.h"
#include "xbt/lib.h"
#include "xbt/sysdep.h"
#include "surf/surf.h"
#include "surf/maxmin.h"
#include "instr/instr_interface.h"
#include "simgrid/simix.h"
#include "simgrid/sg_config.h"
#include "simgrid_config.h" /* what was compiled in? */
#if HAVE_SMPI
#include "smpi/smpi_interface.h"
#endif
#include "mc/mc.h"
#include "simgrid/instr.h"
#include "src/mc/mc_replay.h"

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
  int i, j;

  for (j = i = 1; i < *argc; i++) {
    if (!strncmp(argv[i], "--cfg=", strlen("--cfg="))) {
      char *opt = strchr(argv[i], '=');
      opt++;

      xbt_cfg_set_parse(opt);
      XBT_DEBUG("Did apply '%s' as config setting", opt);
    } else if (!strcmp(argv[i], "--version")) {
      printf("%s\n", SIMGRID_VERSION_STRING);
      shall_exit = 1;
    } else if (!strcmp(argv[i], "--cfg-help") || !strcmp(argv[i], "--help")) {
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
    } else if (!strcmp(argv[i], "--help-aliases")) {
      printf("Here is a list of all deprecated option names, with their replacement.\n");
      xbt_cfg_aliases();
      printf("Please consider using the recent names\n");
      shall_exit = 1;
    } else if (!strcmp(argv[i], "--help-models")) {
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
    } else if (!strcmp(argv[i], "--help-tracing")) {
      TRACE_help (1);
      shall_exit = 1;
    } else {
      argv[j++] = argv[i];
    }
  }
  if (j < *argc) {
    argv[j] = NULL;
    *argc = j;
  }
  if (shall_exit)
    sg_cfg_exit_early();
}

/* callback of the plugin variable */
static void _sg_cfg_cb__plugin(const char *name)
{
  xbt_assert(_sg_cfg_init_status < 2, "Cannot load a plugin after the initialization");

  char *val = xbt_cfg_get_string(name);
  if (val==nullptr)
    return;

  if (!strcmp(val, "help")) {
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

  char *val = xbt_cfg_get_string(name);
  if (!strcmp(val, "help")) {
    model_help("host", surf_host_model_description);
    sg_cfg_exit_early();
  }

  /* Make sure that the model exists */
  find_model_description(surf_host_model_description, val);
}

/* callback of the vm/model variable */
static void _sg_cfg_cb__vm_model(const char *name)
{
  xbt_assert(_sg_cfg_init_status < 2, "Cannot change the model after the initialization");

  char *val = xbt_cfg_get_string(name);
  if (!strcmp(val, "help")) {
    model_help("vm", surf_vm_model_description);
    sg_cfg_exit_early();
  }

  /* Make sure that the model exists */
  find_model_description(surf_vm_model_description, val);
}

/* callback of the cpu/model variable */
static void _sg_cfg_cb__cpu_model(const char *name)
{
  xbt_assert(_sg_cfg_init_status < 2, "Cannot change the model after the initialization");

  char *val = xbt_cfg_get_string(name);
  if (!strcmp(val, "help")) {
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

  char *val = xbt_cfg_get_string(name);
  if (!strcmp(val, "help")) {
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

  char *val = xbt_cfg_get_string(name);
  if (!strcmp(val, "help")) {
    model_help("storage", surf_storage_model_description);
    sg_cfg_exit_early();
  }

  find_model_description(surf_storage_model_description, val);
}

/* callback of the network_model variable */
static void _sg_cfg_cb__network_model(const char *name)
{
  xbt_assert(_sg_cfg_init_status < 2, "Cannot change the model after the initialization");

  char *val = xbt_cfg_get_string(name);
  if (!strcmp(val, "help")) {
    model_help("network", surf_network_model_description);
    sg_cfg_exit_early();
  }

  /* New Module missing */
  find_model_description(surf_network_model_description, val);
}

/* callbacks of the network models values */
static void _sg_cfg_cb__tcp_gamma(const char *name)
{
  sg_tcp_gamma = xbt_cfg_get_double(name);
}

static void _sg_cfg_cb__maxmin_precision(const char* name)
{
  sg_maxmin_precision = xbt_cfg_get_double(name);
}

static void _sg_cfg_cb__surf_precision(const char* name)
{
  sg_surf_precision = xbt_cfg_get_double(name);
}

static void _sg_cfg_cb__sender_gap(const char* name)
{
  sg_sender_gap = xbt_cfg_get_double(name);
}

static void _sg_cfg_cb__latency_factor(const char *name)
{
  sg_latency_factor = xbt_cfg_get_double(name);
}

static void _sg_cfg_cb__bandwidth_factor(const char *name)
{
  sg_bandwidth_factor = xbt_cfg_get_double(name);
}

static void _sg_cfg_cb__weight_S(const char *name)
{
  sg_weight_S_parameter = xbt_cfg_get_double(name);
}

#if HAVE_SMPI
/* callback of the mpi collectives: simply check that this is a valid name. It will be picked up in smpi_global.cpp */
static void _check_coll(const char *category,
                             s_mpi_coll_description_t * table,
                             const char *name)
{
  xbt_assert(_sg_cfg_init_status < 2, "Cannot change the collective algorithm after the initialization");

  char *val = xbt_cfg_get_string(name);
  if (val && !strcmp(val, "help")) {
    coll_help(category, table);
    sg_cfg_exit_early();
  }

  find_coll_description(table, val, category);
}
static void _check_coll_gather(const char *name){
  _check_coll("gather", mpi_coll_gather_description, name);
}
static void _check_coll_allgather(const char *name){
  _check_coll("allgather", mpi_coll_allgather_description, name);
}
static void _check_coll_allgatherv(const char *name){
  _check_coll("allgatherv", mpi_coll_allgatherv_description, name);
}
static void _check_coll_allreduce(const char *name)
{
  _check_coll("allreduce", mpi_coll_allreduce_description, name);
}
static void _check_coll_alltoall(const char *name)
{
  _check_coll("alltoall", mpi_coll_alltoall_description, name);
}
static void _check_coll_alltoallv(const char *name)
{
  _check_coll("alltoallv", mpi_coll_alltoallv_description, name);
}
static void _check_coll_bcast(const char *name)
{
  _check_coll("bcast", mpi_coll_bcast_description, name);
}
static void _check_coll_reduce(const char *name)
{
  _check_coll("reduce", mpi_coll_reduce_description, name);
}
static void _check_coll_reduce_scatter(const char *name){
  _check_coll("reduce_scatter", mpi_coll_reduce_scatter_description, name);
}
static void _check_coll_scatter(const char *name){
  _check_coll("scatter", mpi_coll_scatter_description, name);
}
static void _check_coll_barrier(const char *name){
  _check_coll("barrier", mpi_coll_barrier_description, name);
}

static void _sg_cfg_cb__wtime_sleep(const char *name){
  smpi_wtime_sleep = xbt_cfg_get_double(name);
}

static void _sg_cfg_cb__iprobe_sleep(const char *name){
  smpi_iprobe_sleep = xbt_cfg_get_double(name);
}

static void _sg_cfg_cb__test_sleep(const char *name){
  smpi_test_sleep = xbt_cfg_get_double(name);
}
#endif

/* callback of the inclusion path */
static void _sg_cfg_cb__surf_path(const char *name)
{
  char *path = xbt_strdup(xbt_cfg_get_string(name));
  if (path[0]) // ignore ""
    xbt_dynar_push(surf_path, &path);
}

/* callback to decide if we want to use the model-checking */
#include "src/xbt_modinter.h"

static void _sg_cfg_cb_model_check_replay(const char *name) {
  MC_record_path = xbt_cfg_get_string(name);
}

#if HAVE_MC
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

static void _sg_cfg_cb_context_factory(const char *name)
{
  smx_context_factory_name = xbt_cfg_get_string(name);
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
  const char* mode_name = xbt_cfg_get_string(name);
  if (!strcmp(mode_name, "posix")) {
    SIMIX_context_set_parallel_mode(XBT_PARMAP_POSIX);
  }
  else if (!strcmp(mode_name, "futex")) {
    SIMIX_context_set_parallel_mode(XBT_PARMAP_FUTEX);
  }
  else if (!strcmp(mode_name, "busy_wait")) {
    SIMIX_context_set_parallel_mode(XBT_PARMAP_BUSY_WAIT);
  }
  else {
    xbt_die("Command line setting of the parallel synchronization mode should "
            "be one of \"posix\", \"futex\" or \"busy_wait\"");
  }
}

static void _sg_cfg_cb__surf_network_coordinates(const char *name)
{
  static int already_set = 0;
  int val = xbt_cfg_get_boolean(name);
  if (val) {
    if (!already_set) {
      COORD_HOST_LEVEL = sg_host_extension_create(xbt_dynar_free_voidp);
      COORD_ASR_LEVEL  = xbt_lib_add_level(as_router_lib,xbt_dynar_free_voidp);
    }
    already_set = 1;
  } else
    if (already_set)
      xbt_die("Setting of whether to use coordinate cannot be disabled once set.");
}

static void _sg_cfg_cb__surf_network_crosstraffic(const char *name)
{
  sg_network_crosstraffic = xbt_cfg_get_boolean(name);
}

/* build description line with possible values */
static void describe_model(char *result,
                           const s_surf_model_description_t model_description[],
                           const char *name,
                           const char *description)
{
  char *p = result +
    sprintf(result, "%s. Possible values: %s", description,
            model_description[0].name ? model_description[0].name : "n/a");
  for (int i = 1; model_description[i].name; i++)
    p += sprintf(p, ", %s", model_description[i].name);
  sprintf(p,
      ".\n       (use 'help' as a value to see the long description of each %s)",
          name);
}

/* create the config set, register what should be and parse the command line*/
void sg_config_init(int *argc, char **argv)
{
  char description[1024];

  /* Create the configuration support */
  if (_sg_cfg_init_status == 0) { /* Only create stuff if not already inited */

    /* Plugins configuration */
    describe_model(description, surf_plugin_description, "plugin", "The plugins");
    xbt_cfg_register_string("plugin", nullptr, &_sg_cfg_cb__plugin, description);

    describe_model(description, surf_cpu_model_description, "model", "The model to use for the CPU");
    xbt_cfg_register_string("cpu/model", "Cas01", &_sg_cfg_cb__cpu_model, description);

    describe_model(description, surf_optimization_mode_description, "optimization mode", "The optimization modes to use for the CPU");
    xbt_cfg_register_string("cpu/optim", "Lazy", &_sg_cfg_cb__optimization_mode, description);

    describe_model(description, surf_storage_model_description, "model", "The model to use for the storage");
    xbt_cfg_register_string("storage/model", "default", &_sg_cfg_cb__storage_mode, description);

    describe_model(description, surf_network_model_description, "model", "The model to use for the network");
    xbt_cfg_register_string("network/model", "LV08", &_sg_cfg_cb__network_model, description);

    describe_model(description, surf_optimization_mode_description, "optimization mode", "The optimization modes to use for the network");
    xbt_cfg_register_string("network/optim", "Lazy", &_sg_cfg_cb__optimization_mode, description);

    describe_model(description, surf_host_model_description, "model", "The model to use for the host");
    xbt_cfg_register_string("host/model", "default", &_sg_cfg_cb__host_model, description);

    describe_model(description, surf_vm_model_description, "model", "The model to use for the vm");
    xbt_cfg_register_string("vm/model", "default", &_sg_cfg_cb__vm_model, description);

    xbt_cfg_register_double("network/TCP_gamma",  4194304.0, _sg_cfg_cb__tcp_gamma,
        "Size of the biggest TCP window (cat /proc/sys/net/ipv4/tcp_[rw]mem for recv/send window; Use the last given value, which is the max window size)");
    xbt_cfg_register_double("surf/precision", 0.00001, _sg_cfg_cb__surf_precision,
        "Numerical precision used when updating simulation times (in seconds)");
    xbt_cfg_register_double("maxmin/precision", 0.00001, _sg_cfg_cb__maxmin_precision,
        "Numerical precision used when computing resource sharing (in ops/sec or bytes/sec)");

    /* The parameters of network models */

    xbt_cfg_register_double("network/sender_gap", NAN, _sg_cfg_cb__sender_gap,
        "Minimum gap between two overlapping sends"); /* real default for "network/sender_gap" is set in network_smpi.cpp */
    xbt_cfg_register_double("network/latency_factor", 1.0, _sg_cfg_cb__latency_factor,
        "Correction factor to apply to the provided latency (default value set by network model)");
    xbt_cfg_register_double("network/bandwidth_factor", 1.0, _sg_cfg_cb__bandwidth_factor, "Correction factor to apply to the provided bandwidth (default value set by network model)");
    xbt_cfg_register_double("network/weight_S", NAN, _sg_cfg_cb__weight_S, /* real default for "network/weight_S" is set in network_*.cpp */
        "Correction factor to apply to the weight of competing streams (default value set by network model)");

    /* Inclusion path */
    xbt_cfg_register_string("path", "", _sg_cfg_cb__surf_path, "Lookup path for inclusions in platform and deployment XML files");

    xbt_cfg_register_boolean("cpu/maxmin_selective_update", "no", NULL,
        "Update the constraint set propagating recursively to others constraints (off by default when optim is set to lazy)");
    xbt_cfg_register_boolean("network/maxmin_selective_update", "no", NULL,
        "Update the constraint set propagating recursively to others constraints (off by default when optim is set to lazy)");
    /* Replay (this part is enabled even if MC it disabled) */
    xbt_cfg_register_string("model-check/replay", nullptr, _sg_cfg_cb_model_check_replay,
        "Model-check path to replay (as reported by SimGrid when a violation is reported)");

#if HAVE_MC
    /* do model-checking-record */
    xbt_cfg_register_boolean("model-check/record", "no", _sg_cfg_cb_model_check_record, "Record the model-checking paths");

    xbt_cfg_register_int("model-check/checkpoint", 0, _mc_cfg_cb_checkpoint,
        "Specify the amount of steps between checkpoints during stateful model-checking (default: 0 => stateless verification). "
        "If value=1, one checkpoint is saved for each step => faster verification, but huge memory consumption; higher values are good compromises between speed and memory consumption.");

    xbt_cfg_register_boolean("model-check/sparse_checkpoint", "no", _mc_cfg_cb_sparse_checkpoint, "Use sparse per-page snapshots.");
    xbt_cfg_register_boolean("model-check/soft-dirty", "no", _mc_cfg_cb_soft_dirty, "Use sparse per-page snapshots.");
    xbt_cfg_register_boolean("model-check/ksm", "no", _mc_cfg_cb_ksm, "Kernel same-page merging");

    xbt_cfg_register_string("model-check/property","", _mc_cfg_cb_property,
        "Name of the file containing the property, as formated by the ltl2ba program.");
    xbt_cfg_register_boolean("model-check/communications_determinism", "no", _mc_cfg_cb_comms_determinism,
        "Whether to enable the detection of communication determinism");

    xbt_cfg_register_boolean("model-check/send_determinism", "no", _mc_cfg_cb_send_determinism,
        "Enable/disable the detection of send-determinism in the communications schemes");

    /* Specify the kind of model-checking reduction */
    xbt_cfg_register_string("model-check/reduction", "dpor", _mc_cfg_cb_reduce,
        "Specify the kind of exploration reduction (either none or DPOR)");
    xbt_cfg_register_boolean("model-check/timeout", "no",  _mc_cfg_cb_timeout,
        "Whether to enable timeouts for wait requests");

    xbt_cfg_register_boolean("model-check/hash", "no", _mc_cfg_cb_hash, "Whether to enable state hash for state comparison (experimental)");
    xbt_cfg_register_boolean("model-check/snapshot_fds", "no",  _mc_cfg_cb_snapshot_fds,
        "Whether file descriptors must be snapshoted (currently unusable)");
    xbt_cfg_register_int("model-check/max_depth", 1000, _mc_cfg_cb_max_depth, "Maximal exploration depth (default: 1000)");
    xbt_cfg_register_int("model-check/visited", 0, _mc_cfg_cb_visited,
        "Specify the number of visited state stored for state comparison reduction. If value=5, the last 5 visited states are stored. If value=0 (the default), all states are stored.");

    xbt_cfg_register_string("model-check/dot_output", "", _mc_cfg_cb_dot_output, "Name of dot output file corresponding to graph state");
    xbt_cfg_register_boolean("model-check/termination", "no", _mc_cfg_cb_termination, "Whether to enable non progressive cycle detection");
#endif

    xbt_cfg_register_boolean("verbose-exit", "yes", _sg_cfg_cb_verbose_exit, "Activate the \"do nothing\" mode in Ctrl-C");

    /* context factory */
    const char *dflt_ctx_fact = "thread";
    {
      char *p = description +
        sprintf(description,
                "Context factory to use in SIMIX. Possible values: %s",
                dflt_ctx_fact);
#if HAVE_UCONTEXT_CONTEXTS
      dflt_ctx_fact = "ucontext";
      p += sprintf(p, ", %s", dflt_ctx_fact);
#endif
#if HAVE_RAW_CONTEXTS
      dflt_ctx_fact = "raw";
      p += sprintf(p, ", %s", dflt_ctx_fact);
#endif
      sprintf(p, ".");
    }
    xbt_cfg_register_string("contexts/factory", dflt_ctx_fact, _sg_cfg_cb_context_factory, description);

    xbt_cfg_register_int("contexts/stack_size", 8*1024, _sg_cfg_cb_context_stack_size, "Stack size of contexts in KiB");
    /* (FIXME: this is unpleasant) Reset this static variable that was altered when setting the default value. */
    smx_context_stack_size_was_set = 0;

    /* guard size for contexts stacks in memory pages */
    xbt_cfg_register_int("contexts/guard_size",
#if defined(_WIN32) || (PTH_STACKGROWTH != -1)
        0,
#else
        1,
#endif
    _sg_cfg_cb_context_guard_size, "Guard size for contexts stacks in memory pages");
    /* No, it was not set yet (the above setdefault() changed this to 1). */
    smx_context_guard_size_was_set = 0;

    xbt_cfg_register_int("contexts/nthreads", 1, _sg_cfg_cb_contexts_nthreads, "Number of parallel threads used to execute user contexts");

    xbt_cfg_register_int("contexts/parallel_threshold", 2, _sg_cfg_cb_contexts_parallel_threshold,
        "Minimal number of user contexts to be run in parallel (raw contexts only)");

    /* synchronization mode for parallel user contexts */
#if HAVE_FUTEX_H
    xbt_cfg_register_string("contexts/synchro", "futex",     _sg_cfg_cb_contexts_parallel_mode,
        "Synchronization mode to use when running contexts in parallel (either futex, posix or busy_wait)");
#else //No futex on mac and posix is unimplememted yet
    xbt_cfg_register_string("contexts/synchro", "busy_wait", _sg_cfg_cb_contexts_parallel_mode,
        "Synchronization mode to use when running contexts in parallel (either futex, posix or busy_wait)");
#endif

    xbt_cfg_register_boolean("network/coordinates", "no", _sg_cfg_cb__surf_network_coordinates,
        "Whether we use a coordinate-based routing (as Vivaldi)");

    xbt_cfg_register_boolean("network/crosstraffic", "yes", _sg_cfg_cb__surf_network_crosstraffic,
        "Activate the interferences between uploads and downloads for fluid max-min models (LV08, CM02)");

#if HAVE_NS3
    xbt_cfg_register_string("ns3/TcpModel", "default", NULL, "The ns3 tcp model can be : NewReno or Reno or Tahoe");
#endif

    //For smpi/bw_factor and smpi/lat_factor
    // SMPI model can be used without enable_smpi, so keep this out of the ifdef.
    xbt_cfg_register_string("smpi/bw_factor",
        "65472:0.940694;15424:0.697866;9376:0.58729;5776:1.08739;3484:0.77493;1426:0.608902;732:0.341987;257:0.338112;0:0.812084", NULL,
        "Bandwidth factors for smpi. Format: 'threshold0:value0;threshold1:value1;...;thresholdN:valueN', meaning if(size >=thresholdN ) return valueN.");

    xbt_cfg_register_string("smpi/lat_factor",
        "65472:11.6436;15424:3.48845;9376:2.59299;5776:2.18796;3484:1.88101;1426:1.61075;732:1.9503;257:1.95341;0:2.01467", NULL, "Latency factors for smpi.");
    
    xbt_cfg_register_string("smpi/IB_penalty_factors", "0.965;0.925;1.35", NULL,
        "Correction factor to communications using Infiniband model with contention (default value based on Stampede cluster profiling)");
    
#if HAVE_SMPI
    xbt_cfg_register_double("smpi/running_power", 20000.0, NULL, "Power of the host running the simulation (in flop/s). Used to bench the operations.");
    xbt_cfg_register_boolean("smpi/display_timing", "no", NULL, "Whether we should display the timing after simulation.");
    xbt_cfg_register_boolean("smpi/simulate_computation", "yes", NULL, "Whether the computational part of the simulated application should be simulated.");
    xbt_cfg_register_boolean("smpi/use_shared_malloc", "yes", NULL, "Whether SMPI_SHARED_MALLOC is enabled. Disable it for debugging purposes.");
    xbt_cfg_register_double("smpi/cpu_threshold", 1e-6, NULL, "Minimal computation time (in seconds) not discarded, or -1 for infinity.");
    xbt_cfg_register_int("smpi/async_small_thresh", 0, NULL,
        "Maximal size of messages that are to be sent asynchronously, without waiting for the receiver");
    xbt_cfg_register_int("smpi/send_is_detached_thresh", 65536, NULL,
        "Threshold of message size where MPI_Send stops behaving like MPI_Isend and becomes MPI_Ssend");

    xbt_cfg_register_boolean("smpi/privatize_global_variables", "no", NULL, "Whether we should privatize global variable at runtime.");
    xbt_cfg_register_string("smpi/os", "1:0:0:0:0", NULL,  "Small messages timings (MPI_Send minimum time for small messages)");
    xbt_cfg_register_string("smpi/ois", "1:0:0:0:0", NULL, "Small messages timings (MPI_Isend minimum time for small messages)");
    xbt_cfg_register_string("smpi/or", "1:0:0:0:0", NULL,  "Small messages timings (MPI_Recv minimum time for small messages)");
    xbt_cfg_register_double("smpi/iprobe", 1e-4, _sg_cfg_cb__iprobe_sleep, "Minimum time to inject inside a call to MPI_Iprobe");
    xbt_cfg_register_double("smpi/test", 1e-4, _sg_cfg_cb__test_sleep, "Minimum time to inject inside a call to MPI_Test");
    xbt_cfg_register_double("smpi/wtime", 0.0, _sg_cfg_cb__wtime_sleep, "Minimum time to inject inside a call to MPI_Wtime");

    xbt_cfg_register_string("smpi/coll_selector", "default", NULL, "Which collective selector to use");
    xbt_cfg_register_string("smpi/gather",        nullptr, &_check_coll_gather, "Which collective to use for gather");
    xbt_cfg_register_string("smpi/allgather",     nullptr, &_check_coll_allgather, "Which collective to use for allgather");
    xbt_cfg_register_string("smpi/barrier",       nullptr, &_check_coll_barrier, "Which collective to use for barrier");
    xbt_cfg_register_string("smpi/reduce_scatter",nullptr, &_check_coll_reduce_scatter, "Which collective to use for reduce_scatter");
    xbt_cfg_register_string("smpi/scatter",       nullptr, &_check_coll_scatter, "Which collective to use for scatter");
    xbt_cfg_register_string("smpi/allgatherv",    nullptr, &_check_coll_allgatherv, "Which collective to use for allgatherv");
    xbt_cfg_register_string("smpi/allreduce",     nullptr, &_check_coll_allreduce, "Which collective to use for allreduce");
    xbt_cfg_register_string("smpi/alltoall",      nullptr, &_check_coll_alltoall, "Which collective to use for alltoall");
    xbt_cfg_register_string("smpi/alltoallv",     nullptr, &_check_coll_alltoallv,"Which collective to use for alltoallv");
    xbt_cfg_register_string("smpi/bcast",         nullptr, &_check_coll_bcast, "Which collective to use for bcast");
    xbt_cfg_register_string("smpi/reduce",        nullptr, &_check_coll_reduce, "Which collective to use for reduce");
#endif // HAVE_SMPI

    xbt_cfg_register_boolean("exception/cutpath", "no", NULL,
        "Whether to cut all path information from call traces, used e.g. in exceptions.");

    xbt_cfg_register_boolean("clean_atexit", "yes", _sg_cfg_cb_clean_atexit,
        "Whether to cleanup SimGrid at exit. Disable it if your code segfaults after its end.");

    if (!surf_path) {
      /* retrieves the current directory of the current process */
      const char *initial_path = __surf_get_initial_path();
      xbt_assert((initial_path), "__surf_get_initial_path() failed! Can't resolve current Windows directory");

      surf_path = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
      xbt_cfg_setdefault_string("path", initial_path);
    }

    _sg_cfg_init_status = 1;

    sg_config_cmd_line(argc, argv);

    xbt_mallocator_initialization_is_done(SIMIX_context_is_parallel());
  } else {
    XBT_WARN("Call to sg_config_init() after initialization ignored");
  }
}

void sg_config_finalize(void)
{
  if (!_sg_cfg_init_status)
    return;                     /* Not initialized yet. Nothing to do */

  xbt_cfg_free(&simgrid_config);
  _sg_cfg_init_status = 0;
}

int sg_cfg_is_default_value(const char *name)
{
  return xbt_cfg_is_default_value(name);
}

int sg_cfg_get_int(const char* name)
{
  return xbt_cfg_get_int(name);
}

double sg_cfg_get_double(const char* name)
{
  return xbt_cfg_get_double(name);
}

char* sg_cfg_get_string(const char* name)
{
  return xbt_cfg_get_string(name);
}

int sg_cfg_get_boolean(const char* name)
{
  return xbt_cfg_get_boolean(name);
}

