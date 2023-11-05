/* Copyright (c) 2009-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* sg_config: configuration infrastructure for the simulation world         */

#include <simgrid/instr.h>
#include <simgrid/version.h>
#include <xbt/config.hpp>
#include <xbt/file.hpp>

#include "src/instr/instr_private.hpp"
#include "src/internal_config.h"
#include "src/kernel/context/Context.hpp"
#include "src/kernel/lmm/maxmin.hpp"
#include "src/kernel/resource/NetworkModel.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/simgrid/module.hpp"
#include "src/simgrid/sg_config.hpp"
#include "src/smpi/include/smpi_config.hpp"

#include <string_view>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(config, kernel, "About the configuration of SimGrid");

static simgrid::config::Flag<bool> cfg_continue_after_help
  {"help-nostop", "Do not stop the execution when --help is found", false};

/** @brief Allow other libraries to react to the --help flag, too
 *
 * When finding --help on the command line, SimGrid usually stops right after displaying its help message.
 * If you are writing a library using SimGrid, you may want to display your own help message before everything stops.
 * If so, just call this function before having SimGrid parsing the command line, and you will be given the control
 * even if the user is asking for help.
 */
void sg_config_continue_after_help()
{
  cfg_continue_after_help = true;
}

/* 0: beginning of time (config cannot be changed yet)
 * 1: initialized: cfg_set created (config can now be changed)
 * 2: configured: command line parsed and config part of platform file was
 *    integrated also, platform construction ongoing or done.
 *    (Config cannot be changed anymore!)
 */
int _sg_cfg_init_status = 0;

/* Parse the command line, looking for options */
static void sg_config_cmd_line(int *argc, char **argv)
{
  bool shall_exit = false;
  bool parse_args = true; // Stop parsing the parameters once we found '--'

  int j = 1;
  for (int i = j; i < *argc; i++) {
    if (not strcmp("--", argv[i])) {
      parse_args = false;
      // Remove that '--' from the arguments
    } else if (parse_args && not strncmp(argv[i], "--cfg=", strlen("--cfg="))) {
      char *opt = strchr(argv[i], '=');
      opt++;

      simgrid::config::set_parse(opt);
      XBT_DEBUG("Did apply '%s' as config setting", opt);
    } else if (parse_args && not strcmp(argv[i], "--version")) {
      sg_version();
      shall_exit = true;
    } else if (parse_args && (not strcmp(argv[i], "--cfg-help") || not strcmp(argv[i], "--help"))) {
      XBT_HELP("Description of the configuration accepted by this simulator:");
      simgrid::config::help();
      XBT_HELP("\n"
               "Each of these configurations can be used by adding\n"
               "    --cfg=<option name>:<option value>\n"
               "to the command line. Try passing \"help\" as a value\n"
               "to get the list of values accepted by a given option.\n"
               "For example, \"--cfg=plugin:help\" gives you the list of\n"
               "plugins available in your installation of SimGrid.\n"
               "\n"
               "For more information, please refer to:\n"
               "   --help-aliases for the list of all option aliases.\n"
               "   --help-logs and --help-log-categories for the details of logging output.\n"
               "   --help-models for a list of all models known by this simulator.\n"
               "   --help-tracing for the details of all tracing options known by this simulator.\n"
               "   --version to get SimGrid version information.\n");
      shall_exit = not cfg_continue_after_help;
      argv[j++]  = argv[i]; // Preserve the --help in argv just in case someone else wants to see it
    } else if (parse_args && not strcmp(argv[i], "--help-aliases")) {
      XBT_HELP("Here is a list of all deprecated option names, with their replacement.");
      simgrid::config::show_aliases();
      XBT_HELP("Please consider using the recent names");
      shall_exit = true;
    } else if (parse_args && not strcmp(argv[i], "--help-models")) {
      simgrid_host_models().help();
      XBT_HELP("%s", "");
      simgrid_cpu_models().help();
      XBT_HELP("%s", "");
      simgrid_network_models().help();
      shall_exit = true;
    } else if (parse_args && not strcmp(argv[i], "--help-tracing")) {
      TRACE_help();
      shall_exit = true;
    } else {
      argv[j++] = argv[i];
    }
  }
  if (j < *argc) {
    argv[j] = nullptr;
    *argc = j;
  }
  if (shall_exit)
    exit(0);
}

static void _sg_cfg_cb_contexts_parallel_mode(std::string_view mode_name)
{
  if (mode_name == "posix") {
    simgrid::kernel::context::Context::parallel_mode = XBT_PARMAP_POSIX;
  } else if (mode_name == "futex") {
    simgrid::kernel::context::Context::parallel_mode = XBT_PARMAP_FUTEX;
  } else if (mode_name == "busy_wait") {
    simgrid::kernel::context::Context::parallel_mode = XBT_PARMAP_BUSY_WAIT;
  } else {
    xbt_die("Command line setting of the parallel synchronization mode should "
            "be one of \"posix\", \"futex\" or \"busy_wait\"");
  }
}

/* create the config set, register what should be and parse the command line*/
void sg_config_init(int *argc, char **argv)
{
  /* Create the configuration support */
  if (_sg_cfg_init_status != 0) { /* Only create stuff if not already inited */
    XBT_WARN("Call to sg_config_init() after initialization ignored");
    return;
  }
  _sg_cfg_init_status = 1;

  /* Plugins and models configuration */
  simgrid_plugins().create_flag("plugin", "The plugins", "", true);
  simgrid_cpu_models().create_flag("cpu/model", "The model to use for the CPU", "Cas01", false);
  simgrid_network_models().create_flag("network/model", "The model to use for the network", "LV08", false);
  simgrid_host_models().create_flag("host/model", "The model to use for the host", "default", false);
  simgrid_disk_models().create_flag("disk/model", "The model to use for the disk", "S19", false);

  simgrid::config::bind_flag(sg_precision_timing, "precision/timing", {"surf/precision"},
                             "Numerical precision used when updating simulation times (in seconds)");

  simgrid::config::bind_flag(sg_precision_workamount, "precision/work-amount", {"maxmin/precision"},
                             "Numerical precision used when computing resource sharing (in flops/sec or bytes/sec)");

  simgrid::config::bind_flag(sg_concurrency_limit, "maxmin/concurrency-limit",
                             "Maximum number of concurrent variables in the maxmim system. Also limits the number of "
                             "processes on each host, at higher level. (default: -1 means no such limitation)");

  /* The parameters of network models */
  static simgrid::config::Flag<double> _sg_network_loopback_latency{
      "network/loopback-lat",
      "For network models with an implicit loopback link (L07, CM02, LV08), "
      "latency of the loopback link. 0 by default",
      0.0};

  static simgrid::config::Flag<double> _sg_network_loopback_bandwidth{
      "network/loopback-bw",
      "For network models with an implicit loopback link (L07, CM02, LV08), "
      "bandwidth of the loopback link. 10GBps by default",
      10e9};

  /* Inclusion path */
  static simgrid::config::Flag<std::string> cfg_path{
      "path", "Lookup path for inclusions in platform and deployment XML files", "./", [](std::string const& path) {
        if (not path.empty())
          simgrid::xbt::path_push(path);
      }};

  static simgrid::config::Flag<bool> cfg_cpu_maxmin_selective_update{
      "cpu/maxmin-selective-update",
      "Update the constraint set propagating recursively to others constraints "
      "(off by default unless optim is set to lazy)",
      false};
  static simgrid::config::Flag<bool> cfg_network_maxmin_selective_update{"network/maxmin-selective-update",
                                                                         "Update the constraint set propagating "
                                                                         "recursively to others constraints (off by "
                                                                         "default unless optim is set to lazy)",
                                                                         false};

  static simgrid::config::Flag<int> cfg_context_stack_size{
      "contexts/stack-size", "Stack size of contexts in KiB (not with threads)", 8 * 1024,
      [](int value) { simgrid::kernel::context::Context::stack_size = value * 1024; }};

  /* guard size for contexts stacks in memory pages */
#if (PTH_STACKGROWTH != -1)
  int default_guard_size = 0;
#else
  int default_guard_size = 1;
#endif
  static simgrid::config::Flag<int> cfg_context_guard_size{
      "contexts/guard-size", "Guard size for contexts stacks in memory pages", default_guard_size,
      [](int value) { simgrid::kernel::context::Context::guard_size = value * xbt_pagesize; }};

  static simgrid::config::Flag<int> cfg_context_nthreads{
      "contexts/nthreads", "Number of parallel threads used to execute user contexts", 1,
      [](int nthreads) { simgrid::kernel::context::Context::set_nthreads(nthreads); }};

  /* synchronization mode for parallel user contexts */
#if HAVE_FUTEX_H
  std::string default_synchro_mode = "futex";
#else // No futex on mac and posix is unimplemented yet
  std::string default_synchro_mode = "busy_wait";
#endif
  static simgrid::config::Flag<std::string> cfg_context_synchro{"contexts/synchro",
                                                                "Synchronization mode to use when running contexts in "
                                                                "parallel (either futex, posix or busy_wait)",
                                                                default_synchro_mode,
                                                                &_sg_cfg_cb_contexts_parallel_mode};

  // SMPI model can be used without enable_smpi, so keep this out of the ifdef.
  static simgrid::config::Flag<std::string> cfg_smpi_IB_penalty_factors{
      "smpi/IB-penalty-factors",
      "Correction factor to communications using Infiniband model with "
      "contention (default value based on Stampede cluster profiling)",
      "0.965;0.925;1.35"};
  /* Others */

  static simgrid::config::Flag<bool> cfg_execution_cutpath{
      "exception/cutpath", "Whether to cut all path information from call traces, used e.g. in exceptions.", false};

  sg_config_cmd_line(argc, argv);

  xbt_mallocator_initialization_is_done(simgrid::kernel::context::Context::is_parallel());
}

void sg_config_finalize()
{
  simgrid::config::finalize();
  _sg_cfg_init_status = 0;
}
