/* Copyright (c) 2009-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* sg_config: configuration infrastructure for the simulation world         */

#include <simgrid/instr.h>
#include <simgrid/version.h>
#include <xbt/config.hpp>

#include "simgrid/sg_config.hpp"
#include "src/instr/instr_private.hpp"
#include "src/internal_config.h"
#include "src/kernel/context/Context.hpp"
#include "src/kernel/lmm/maxmin.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/smpi/include/smpi_config.hpp"
#include "src/surf/surf_interface.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_config, surf, "About the configuration of SimGrid");

static simgrid::config::Flag<bool> cfg_continue_after_help
  {"help-nostop", "Do not stop the execution when --help is found", false};

/** @brief Allow other libraries to react to the --help flag, too
 *
 * When finding --help on the command line, simgrid usually stops right after displaying its help message.
 * If you are writing a library using simgrid, you may want to display your own help message before everything stops.
 * If so, just call this function before having simgrid parsing the command line, and you will be given the control
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
      model_help("host", surf_host_model_description);
      XBT_HELP("%s", "");
      model_help("CPU", surf_cpu_model_description);
      XBT_HELP("%s", "");
      model_help("network", surf_network_model_description);
      XBT_HELP("\nLong description of all optimization levels accepted by the models of this simulator:");
      for (auto const& item : surf_optimization_mode_description)
        XBT_HELP("  %s: %s", item.name, item.description);
      XBT_HELP("Both network and CPU models have 'Lazy' as default optimization level\n");
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

/* callback of the plugin variable */
static void _sg_cfg_cb__plugin(const std::string& value)
{
  xbt_assert(_sg_cfg_init_status < 2, "Cannot load a plugin after the initialization");

  if (value.empty())
    return;

  if (value == "help") {
    model_help("plugin", surf_plugin_description());
    exit(0);
  }

  const auto* plugin = find_model_description(surf_plugin_description(), value);
  plugin->model_init_preparse();
}

/* callback of the host/model variable */
static void _sg_cfg_cb__host_model(const std::string& value)
{
  xbt_assert(_sg_cfg_init_status < 2, "Cannot change the model after the initialization");

  if (value == "help") {
    model_help("host", surf_host_model_description);
    exit(0);
  }

  /* Make sure that the model exists */
  find_model_description(surf_host_model_description, value);
}

/* callback of the cpu/model variable */
static void _sg_cfg_cb__cpu_model(const std::string& value)
{
  xbt_assert(_sg_cfg_init_status < 2, "Cannot change the model after the initialization");

  if (value == "help") {
    model_help("CPU", surf_cpu_model_description);
    exit(0);
  }

  /* New Module missing */
  find_model_description(surf_cpu_model_description, value);
}

/* callback of the cpu/model variable */
static void _sg_cfg_cb__optimization_mode(const std::string& value)
{
  xbt_assert(_sg_cfg_init_status < 2, "Cannot change the model after the initialization");

  if (value == "help") {
    model_help("optimization", surf_optimization_mode_description);
    exit(0);
  }

  /* New Module missing */
  find_model_description(surf_optimization_mode_description, value);
}

static void _sg_cfg_cb__disk_model(const std::string& value)
{
  xbt_assert(_sg_cfg_init_status < 2, "Cannot change the model after the initialization");

  if (value == "help") {
    model_help("disk", surf_disk_model_description);
    exit(0);
  }

  find_model_description(surf_disk_model_description, value);
}

/* callback of the network_model variable */
static void _sg_cfg_cb__network_model(const std::string& value)
{
  xbt_assert(_sg_cfg_init_status < 2, "Cannot change the model after the initialization");

  if (value == "help") {
    model_help("network", surf_network_model_description);
    exit(0);
  }

  /* New Module missing */
  find_model_description(surf_network_model_description, value);
}

static void _sg_cfg_cb_contexts_parallel_mode(const std::string& mode_name)
{
  if (mode_name == "posix") {
    simgrid::kernel::context::set_parallel_mode(XBT_PARMAP_POSIX);
  } else if (mode_name == "futex") {
    simgrid::kernel::context::set_parallel_mode(XBT_PARMAP_FUTEX);
  } else if (mode_name == "busy_wait") {
    simgrid::kernel::context::set_parallel_mode(XBT_PARMAP_BUSY_WAIT);
  } else {
    xbt_die("Command line setting of the parallel synchronization mode should "
            "be one of \"posix\", \"futex\" or \"busy_wait\"");
  }
}

/* build description line with possible values */
static void declare_model_flag(const std::string& name, const std::string& value,
                               const std::function<void(std::string const&)>& callback,
                               const std::vector<surf_model_description_t>& model_description, const std::string& type,
                               const std::string& descr)
{
  std::string description = descr + ". Possible values: ";
  std::string sep         = "";
  for (auto const& item : model_description) {
    description += sep + item.name;
    sep = ", ";
  }
  description += ".\n       (use 'help' as a value to see the long description of each " + type + ")";
  simgrid::config::declare_flag<std::string>(name, description, value, callback);
}

/* create the config set, register what should be and parse the command line*/
void sg_config_init(int *argc, char **argv)
{
  /* Create the configuration support */
  if (_sg_cfg_init_status != 0) { /* Only create stuff if not already inited */
    XBT_WARN("Call to sg_config_init() after initialization ignored");
    return;
  }

  /* Plugins configuration */
  declare_model_flag("plugin", "", &_sg_cfg_cb__plugin, surf_plugin_description(), "plugin", "The plugins");

  declare_model_flag("cpu/model", "Cas01", &_sg_cfg_cb__cpu_model, surf_cpu_model_description, "model",
                     "The model to use for the CPU");

  declare_model_flag("disk/model", "default", &_sg_cfg_cb__disk_model, surf_disk_model_description, "model",
                     "The model to use for the disk");

  declare_model_flag("network/model", "LV08", &_sg_cfg_cb__network_model, surf_network_model_description, "model",
                     "The model to use for the network");

  declare_model_flag("network/optim", "Lazy", &_sg_cfg_cb__optimization_mode, surf_optimization_mode_description,
                     "optimization mode", "The optimization modes to use for the network");

  declare_model_flag("host/model", "default", &_sg_cfg_cb__host_model, surf_host_model_description, "model",
                     "The model to use for the host");

  simgrid::config::bind_flag(sg_surf_precision, "surf/precision",
                             "Numerical precision used when updating simulation times (in seconds)");

  simgrid::config::bind_flag(sg_maxmin_precision, "maxmin/precision",
                             "Numerical precision used when computing resource sharing (in flops/sec or bytes/sec)");

  simgrid::config::bind_flag(sg_concurrency_limit, "maxmin/concurrency-limit",
                             "Maximum number of concurrent variables in the maxmim system. Also limits the number of "
                             "processes on each host, at higher level. (default: -1 means no such limitation)");

  simgrid::config::bind_flag(sg_bmf_max_iterations, "bmf/max-iterations",
                             "Maximum number of steps to be performed while searching for a BMF allocation");

  simgrid::config::Flag<bool> _sg_bmf_selective_update{
      "bmf/selective-update",
      "Update the constraint set propagating recursively to others constraints "
      "(off by default)",
      false};

  /* The parameters of network models */

  sg_latency_factor = 13.01; // comes from the default LV08 network model
  simgrid::config::bind_flag(sg_latency_factor, "network/latency-factor",
                             "Correction factor to apply to the provided latency (default value set by network model)");

  sg_bandwidth_factor = 0.97; // comes from the default LV08 network model
  simgrid::config::bind_flag(
      sg_bandwidth_factor, "network/bandwidth-factor",
      "Correction factor to apply to the provided bandwidth (default value set by network model)");

  sg_weight_S_parameter = 20537; // comes from the default LV08 network model
  simgrid::config::bind_flag(
      sg_weight_S_parameter, "network/weight-S",
      "Correction factor to apply to the weight of competing streams (default value set by network model)");

  simgrid::config::Flag<double> _sg_network_loopback_latency{
      "network/loopback-lat",
      "For network models with an implicit loopback link (L07, CM02, LV08), "
      "latency of the loopback link. 0 by default",
      0.0};

  simgrid::config::Flag<double> _sg_network_loopback_bandwidth{
      "network/loopback-bw",
      "For network models with an implicit loopback link (L07, CM02, LV08), "
      "bandwidth of the loopback link. 10GBps by default",
      10e9};

  /* Inclusion path */
  simgrid::config::declare_flag<std::string>("path", "Lookup path for inclusions in platform and deployment XML files",
                                             "", [](std::string const& path) {
                                               if (not path.empty())
                                                 surf_path.push_back(path);
                                             });

  simgrid::config::Flag<bool> _sg_cpu_maxmin_selective_update{
      "cpu/maxmin-selective-update",
      "Update the constraint set propagating recursively to others constraints "
      "(off by default unless optim is set to lazy)",
      false};
  simgrid::config::Flag<bool> _sg_network_maxmin_selective_update{"network/maxmin-selective-update",
                                                                  "Update the constraint set propagating "
                                                                  "recursively to others constraints (off by "
                                                                  "default unless optim is set to lazy)",
                                                                  false};

  simgrid::config::Flag<int> _sg_context_stack_size{
      "contexts/stack-size", "Stack size of contexts in KiB (not with threads)", 8 * 1024,
      [](int value) { simgrid::kernel::context::stack_size = value * 1024; }};

  /* guard size for contexts stacks in memory pages */
#if defined(_WIN32) || (PTH_STACKGROWTH != -1)
  int default_guard_size = 0;
#else
  int default_guard_size = 1;
#endif
  simgrid::config::Flag<int> _sg_context_guard_size{
      "contexts/guard-size", "Guard size for contexts stacks in memory pages", default_guard_size,
      [](int value) { simgrid::kernel::context::guard_size = value * xbt_pagesize; }};
  simgrid::config::Flag<int> _sg_context_nthreads{"contexts/nthreads",
                                                  "Number of parallel threads used to execute user contexts", 1,
                                                  &simgrid::kernel::context::set_nthreads};

  /* synchronization mode for parallel user contexts */
#if HAVE_FUTEX_H
  std::string default_synchro_mode = "futex";
#else // No futex on mac and posix is unimplemented yet
  std::string default_synchro_mode = "busy_wait";
#endif
  simgrid::config::Flag<std::string> _sg_context_synchro{"contexts/synchro",
                                                         "Synchronization mode to use when running contexts in "
                                                         "parallel (either futex, posix or busy_wait)",
                                                         default_synchro_mode, &_sg_cfg_cb_contexts_parallel_mode};

  // For smpi/bw-factor and smpi/lat-factor
  // SMPI model can be used without enable_smpi, so keep this out of the ifdef.
  simgrid::config::declare_flag<std::string>("smpi/bw-factor",
                                             "Bandwidth factors for smpi. Format: "
                                             "'threshold0:value0;threshold1:value1;...;thresholdN:valueN', "
                                             "meaning if(size >=thresholdN ) return valueN.",
                                             "65472:0.940694;15424:0.697866;9376:0.58729;5776:1.08739;3484:0.77493;"
                                             "1426:0.608902;732:0.341987;257:0.338112;0:0.812084");

  simgrid::config::declare_flag<std::string>("smpi/lat-factor", "Latency factors for smpi.",
                                             "65472:11.6436;15424:3.48845;9376:2.59299;5776:2.18796;3484:1.88101;"
                                             "1426:1.61075;732:1.9503;257:1.95341;0:2.01467");
  simgrid::config::declare_flag<std::string>("smpi/IB-penalty-factors",
                                             "Correction factor to communications using Infiniband model with "
                                             "contention (default value based on Stampede cluster profiling)",
                                             "0.965;0.925;1.35");
  /* Others */

  simgrid::config::Flag<bool> _sg_execution_cutpath{
      "exception/cutpath", "Whether to cut all path information from call traces, used e.g. in exceptions.", false};

  if (surf_path.empty())
    simgrid::config::set_default<std::string>("path", "./");

  _sg_cfg_init_status = 1;

  sg_config_cmd_line(argc, argv);

  xbt_mallocator_initialization_is_done(simgrid::kernel::context::is_parallel());
}

void sg_config_finalize()
{
  simgrid::config::finalize();
  _sg_cfg_init_status = 0;
}
