/* Copyright (c) 2009, 2010. The SimGrid Team.
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
#include "smpi/smpi_interface.h"
#include "mc/mc.h"
#include "instr/instr.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_config, surf,
                                "About the configuration of simgrid");

xbt_cfg_t _sg_cfg_set = NULL;

int _sg_init_status = 0;      /* 0: beginning of time (config cannot be changed yet);
                                  1: initialized: cfg_set created (config can now be changed);
                                  2: configured: command line parsed and config part of platform file was integrated also, platform construction ongoing or done.
                                     (Config cannot be changed anymore!) */

/* Parse the command line, looking for options */
static void sg_config_cmd_line(int *argc, char **argv)
{
  int shall_exit = 0;
  int i, j;
  char *opt;

  for (j = i = 1; i < *argc; i++) {
    if (!strncmp(argv[i], "--cfg=", strlen("--cfg="))) {
      opt = strchr(argv[i], '=');
      opt++;

      xbt_cfg_set_parse(_sg_cfg_set, opt);
      XBT_DEBUG("Did apply '%s' as config setting", opt);
    } else if (!strcmp(argv[i], "--cfg-help") || !strcmp(argv[i], "--help")) {
      printf
          ("Description of the configuration accepted by this simulator:\n");
      xbt_cfg_help(_sg_cfg_set);
      printf(
"\n"
"Each of these configurations can be used by adding\n"
"    --cfg=<option name>:<option value>\n"
"to the command line.\n"
"You can also use --help-models to see the details of all models known by this simulator.\n"
#ifdef HAVE_TRACING
"\n"
"You can also use --help-tracing to see the details of all tracing options known by this simulator.\n"
#endif
"\n"
"You can also use --help-logs and --help-log-categories to see the details of logging output.\n"
"\n"
        );
      shall_exit = 1;
    } else if (!strcmp(argv[i], "--help-models")) {
      int k;

      model_help("workstation", surf_workstation_model_description);
      printf("\n");
      model_help("CPU", surf_cpu_model_description);
      printf("\n");
      model_help("network", surf_network_model_description);
      printf("\nLong description of all optimization levels accepted by the models of this simulator:\n");
      for (k = 0; surf_optimization_mode_description[k].name; k++)
        printf("  %s: %s\n",
               surf_optimization_mode_description[k].name,
               surf_optimization_mode_description[k].description);
      printf("Both network and CPU models have 'Lazy' as default optimization level\n\n");
      shall_exit = 1;
#ifdef HAVE_TRACING
    } else if (!strcmp(argv[i], "--help-tracing")) {
      TRACE_help (1);
      shall_exit = 1;
#endif
    } else {
      argv[j++] = argv[i];
    }
  }
  if (j < *argc) {
    argv[j] = NULL;
    *argc = j;
  }
  if (shall_exit) {
    _sg_init_status=1; // get everything cleanly cleaned on exit
    exit(0);
  }
}

/* callback of the workstation/model variable */
static void _sg_cfg_cb__workstation_model(const char *name, int pos)
{
  char *val;

  xbt_assert(_sg_init_status == 1,
              "Cannot change the model after the initialization");

  val = xbt_cfg_get_string(_sg_cfg_set, name);

  if (!strcmp(val, "help")) {
    model_help("workstation", surf_workstation_model_description);
    exit(0);
  }

  /* Make sure that the model exists */
  find_model_description(surf_workstation_model_description, val);
}

/* callback of the cpu/model variable */
static void _sg_cfg_cb__cpu_model(const char *name, int pos)
{
  char *val;

  xbt_assert(_sg_init_status == 1,
              "Cannot change the model after the initialization");

  val = xbt_cfg_get_string(_sg_cfg_set, name);

  if (!strcmp(val, "help")) {
    model_help("CPU", surf_cpu_model_description);
    exit(0);
  }

  /* New Module missing */
  find_model_description(surf_cpu_model_description, val);
}

/* callback of the cpu/model variable */
static void _sg_cfg_cb__optimization_mode(const char *name, int pos)
{
  char *val;

  xbt_assert(_sg_init_status == 1,
              "Cannot change the model after the initialization");

  val = xbt_cfg_get_string(_sg_cfg_set, name);

  if (!strcmp(val, "help")) {
    model_help("optimization", surf_optimization_mode_description);
    exit(0);
  }

  /* New Module missing */
  find_model_description(surf_optimization_mode_description, val);
}

/* callback of the cpu/model variable */
static void _sg_cfg_cb__storage_mode(const char *name, int pos)
{
  char *val;

  xbt_assert(_sg_init_status == 1,
              "Cannot change the model after the initialization");

  val = xbt_cfg_get_string(_sg_cfg_set, name);

  if (!strcmp(val, "help")) {
    model_help("storage", surf_storage_model_description);
    exit(0);
  }

  /* New Module missing */
  find_model_description(surf_storage_model_description, val);
}

/* callback of the workstation_model variable */
static void _sg_cfg_cb__network_model(const char *name, int pos)
{
  char *val;

  xbt_assert(_sg_init_status == 1,
              "Cannot change the model after the initialization");

  val = xbt_cfg_get_string(_sg_cfg_set, name);

  if (!strcmp(val, "help")) {
    model_help("network", surf_network_model_description);
    exit(0);
  }

  /* New Module missing */
  find_model_description(surf_network_model_description, val);
}


/* callbacks of the network models values */
static void _sg_cfg_cb__tcp_gamma(const char *name, int pos)
{
  sg_tcp_gamma = xbt_cfg_get_double(_sg_cfg_set, name);
}

static void _sg_cfg_cb__maxmin_precision(const char* name, int pos)
{
  sg_maxmin_precision = xbt_cfg_get_double(_sg_cfg_set, name);
}

static void _sg_cfg_cb__sender_gap(const char* name, int pos)
{
  sg_sender_gap = xbt_cfg_get_double(_sg_cfg_set, name);
}

static void _sg_cfg_cb__latency_factor(const char *name, int pos)
{
  sg_latency_factor = xbt_cfg_get_double(_sg_cfg_set, name);
}

static void _sg_cfg_cb__bandwidth_factor(const char *name, int pos)
{
  sg_bandwidth_factor = xbt_cfg_get_double(_sg_cfg_set, name);
}

static void _sg_cfg_cb__weight_S(const char *name, int pos)
{
  sg_weight_S_parameter = xbt_cfg_get_double(_sg_cfg_set, name);
}

#ifdef HAVE_SMPI
/* callback of the mpi collectives */
static void _sg_cfg_cb__coll(const char *category,
		             s_mpi_coll_description_t * table,
		             const char *name, int pos)
{
  char *val;

  xbt_assert(_sg_init_status == 1,
              "Cannot change the model after the initialization");

  val = xbt_cfg_get_string(_sg_cfg_set, name);

  if (!strcmp(val, "help")) {
    coll_help(category, table);
    exit(0);
  }

  /* New Module missing */
  find_coll_description(table, val);
}
static void _sg_cfg_cb__coll_allgather(const char *name, int pos){
  _sg_cfg_cb__coll("allgather", mpi_coll_allgather_description, name, pos);
}
static void _sg_cfg_cb__coll_allgatherv(const char *name, int pos){
  _sg_cfg_cb__coll("allgatherv", mpi_coll_allgatherv_description, name, pos);
}
static void _sg_cfg_cb__coll_allreduce(const char *name, int pos)
{
  _sg_cfg_cb__coll("allreduce", mpi_coll_allreduce_description, name, pos);
}
static void _sg_cfg_cb__coll_alltoall(const char *name, int pos)
{
  _sg_cfg_cb__coll("alltoall", mpi_coll_alltoall_description, name, pos);  
}
static void _sg_cfg_cb__coll_alltoallv(const char *name, int pos)
{
  _sg_cfg_cb__coll("alltoallv", mpi_coll_alltoallv_description, name, pos);  
}
static void _sg_cfg_cb__coll_bcast(const char *name, int pos)
{
  _sg_cfg_cb__coll("bcast", mpi_coll_bcast_description, name, pos);  
}
static void _sg_cfg_cb__coll_reduce(const char *name, int pos)
{
  _sg_cfg_cb__coll("reduce", mpi_coll_reduce_description, name, pos);  
}
#endif

/* callback of the inclusion path */
static void _sg_cfg_cb__surf_path(const char *name, int pos)
{
  char *path = xbt_cfg_get_string_at(_sg_cfg_set, name, pos);
  xbt_dynar_push(surf_path, &path);
}

/* callback to decide if we want to use the model-checking */
#include "xbt_modinter.h"
#ifdef HAVE_MC
extern int _sg_do_model_check;   /* this variable lives in xbt_main until I find a right location for it */
#endif

static void _sg_cfg_cb_model_check(const char *name, int pos)
{
#ifdef HAVE_MC
  _sg_do_model_check = xbt_cfg_get_boolean(_sg_cfg_set, name);
#else
  if (xbt_cfg_get_boolean(_sg_cfg_set, name)) {
    xbt_die("You tried to activate the model-checking from the command line, but it was not compiled in. Change your settings in cmake, recompile and try again");
  }
#endif
}

extern int _sg_do_verbose_exit;

static void _sg_cfg_cb_verbose_exit(const char *name, int pos)
{
  _sg_do_verbose_exit = xbt_cfg_get_boolean(_sg_cfg_set, name);
}


static void _sg_cfg_cb_context_factory(const char *name, int pos) {
  smx_context_factory_name = xbt_cfg_get_string(_sg_cfg_set, name);
}

static void _sg_cfg_cb_context_stack_size(const char *name, int pos)
{
  smx_context_stack_size_was_set = 1;
  smx_context_stack_size = xbt_cfg_get_int(_sg_cfg_set, name) * 1024;
}

static void _sg_cfg_cb_contexts_nthreads(const char *name, int pos)
{
  SIMIX_context_set_nthreads(xbt_cfg_get_int(_sg_cfg_set, name));
}

static void _sg_cfg_cb_contexts_parallel_threshold(const char *name, int pos)
{
  SIMIX_context_set_parallel_threshold(xbt_cfg_get_int(_sg_cfg_set, name));
}

static void _sg_cfg_cb_contexts_parallel_mode(const char *name, int pos)
{
  const char* mode_name = xbt_cfg_get_string(_sg_cfg_set, name);
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

static void _sg_cfg_cb__surf_network_coordinates(const char *name,
                                                   int pos)
{
  int val = xbt_cfg_get_boolean(_sg_cfg_set, name);
  if (val) {
    if (!COORD_HOST_LEVEL) {
      COORD_HOST_LEVEL = xbt_lib_add_level(host_lib,xbt_dynar_free_voidp);
      COORD_ASR_LEVEL  = xbt_lib_add_level(as_router_lib,xbt_dynar_free_voidp);
    }
  } else
    if (COORD_HOST_LEVEL)
      xbt_die("Setting of whether to use coordinate cannot be disabled once set.");
}

static void _sg_cfg_cb__surf_network_crosstraffic(const char *name,
                                                  int pos)
{
  sg_network_crosstraffic = xbt_cfg_get_boolean(_sg_cfg_set, name);
}

#ifdef HAVE_GTNETS
static void _sg_cfg_cb__gtnets_jitter(const char *name, int pos)
{
  sg_gtnets_jitter = xbt_cfg_get_double(_sg_cfg_set, name);
}

static void _sg_cfg_cb__gtnets_jitter_seed(const char *name, int pos)
{
  sg_gtnets_jitter_seed = xbt_cfg_get_int(_sg_cfg_set, name);
}
#endif

/* create the config set, register what should be and parse the command line*/
void sg_config_init(int *argc, char **argv)
{
  char *description = xbt_malloc(1024), *p = description;
  char *default_value;
  double double_default_value;
  int default_value_int;
  int i;

  /* Create the configuration support */
  if (_sg_init_status == 0) { /* Only create stuff if not already inited */
    sprintf(description,
            "The model to use for the CPU. Possible values: ");
    p = description;
    while (*(++p) != '\0');
    for (i = 0; surf_cpu_model_description[i].name; i++)
      p += sprintf(p, "%s%s", (i == 0 ? "" : ", "),
                   surf_cpu_model_description[i].name);
    sprintf(p,
            ".\n       (use 'help' as a value to see the long description of each model)");
    default_value = xbt_strdup("Cas01");
    xbt_cfg_register(&_sg_cfg_set, "cpu/model", description, xbt_cfgelm_string,
                     &default_value, 1, 1, &_sg_cfg_cb__cpu_model, NULL);

    sprintf(description,
            "The optimization modes to use for the CPU. Possible values: ");
    p = description;
    while (*(++p) != '\0');
    for (i = 0; surf_optimization_mode_description[i].name; i++)
      p += sprintf(p, "%s%s", (i == 0 ? "" : ", "),
                   surf_optimization_mode_description[i].name);
    sprintf(p,
            ".\n       (use 'help' as a value to see the long description of each optimization mode)");
    default_value = xbt_strdup("Lazy");
    xbt_cfg_register(&_sg_cfg_set, "cpu/optim", description, xbt_cfgelm_string,
                     &default_value, 1, 1, &_sg_cfg_cb__optimization_mode, NULL);

    sprintf(description,
            "The model to use for the storage. Possible values: ");
    p = description;
    while (*(++p) != '\0');
    for (i = 0; surf_storage_model_description[i].name; i++)
      p += sprintf(p, "%s%s", (i == 0 ? "" : ", "),
                   surf_storage_model_description[i].name);
    sprintf(p,
            ".\n       (use 'help' as a value to see the long description of each model)");
    default_value = xbt_strdup("default");
    xbt_cfg_register(&_sg_cfg_set, "storage/model", description, xbt_cfgelm_string,
                     &default_value, 1, 1, &_sg_cfg_cb__storage_mode,
                     NULL);

    /* ********************************************************************* */
    /* TUTORIAL: New model                                                   */
    sprintf(description,
            "The model to use for the New model. Possible values: ");
    p = description;
    while (*(++p) != '\0');
    for (i = 0; surf_new_model_description[i].name; i++)
      p += sprintf(p, "%s%s", (i == 0 ? "" : ", "),
                   surf_new_model_description[i].name);
    sprintf(p,
            ".\n       (use 'help' as a value to see the long description of each model)");
    default_value = xbt_strdup("default");
    xbt_cfg_register(&_sg_cfg_set, "new_model/model", description, xbt_cfgelm_string,
                     &default_value, 1, 1, &_sg_cfg_cb__storage_mode,
                     NULL);
    /* ********************************************************************* */

    sprintf(description,
            "The model to use for the network. Possible values: ");
    p = description;
    while (*(++p) != '\0');
    for (i = 0; surf_network_model_description[i].name; i++)
      p += sprintf(p, "%s%s", (i == 0 ? "" : ", "),
                   surf_network_model_description[i].name);
    sprintf(p,
            ".\n       (use 'help' as a value to see the long description of each model)");
    default_value = xbt_strdup("LV08");
    xbt_cfg_register(&_sg_cfg_set, "network/model", description, xbt_cfgelm_string,
                     &default_value, 1, 1, &_sg_cfg_cb__network_model,
                     NULL);

    sprintf(description,
            "The optimization modes to use for the network. Possible values: ");
    p = description;
    while (*(++p) != '\0');
    for (i = 0; surf_optimization_mode_description[i].name; i++)
      p += sprintf(p, "%s%s", (i == 0 ? "" : ", "),
                   surf_optimization_mode_description[i].name);
    sprintf(p,
            ".\n       (use 'help' as a value to see the long description of each optimization mode)");
    default_value = xbt_strdup("Lazy");
    xbt_cfg_register(&_sg_cfg_set, "network/optim", description, xbt_cfgelm_string,
                     &default_value, 1, 1, &_sg_cfg_cb__optimization_mode, NULL);

    sprintf(description,
            "The model to use for the workstation. Possible values: ");
    p = description;
    while (*(++p) != '\0');
    for (i = 0; surf_workstation_model_description[i].name; i++)
      p += sprintf(p, "%s%s", (i == 0 ? "" : ", "),
                   surf_workstation_model_description[i].name);
    sprintf(p,
            ".\n       (use 'help' as a value to see the long description of each model)");
    default_value = xbt_strdup("default");
    xbt_cfg_register(&_sg_cfg_set, "workstation/model", description, xbt_cfgelm_string,
                     &default_value, 1, 1,
                     &_sg_cfg_cb__workstation_model, NULL);

    xbt_free(description);

    xbt_cfg_register(&_sg_cfg_set, "network/TCP_gamma",
                     "Size of the biggest TCP window (cat /proc/sys/net/ipv4/tcp_[rw]mem for recv/send window; Use the last given value, which is the max window size)",
                     xbt_cfgelm_double, NULL, 1, 1,
                     _sg_cfg_cb__tcp_gamma, NULL);
    xbt_cfg_setdefault_double(_sg_cfg_set, "network/TCP_gamma", 4194304.0);

    xbt_cfg_register(&_sg_cfg_set, "maxmin/precision",
                     "Numerical precision used when updating simulation models (epsilon in double comparisons)",
                     xbt_cfgelm_double, NULL, 1, 1, _sg_cfg_cb__maxmin_precision, NULL);
    xbt_cfg_setdefault_double(_sg_cfg_set, "maxmin/precision", 0.00001); 

    /* The parameters of network models */

    xbt_cfg_register(&_sg_cfg_set, "network/sender_gap",
                     "Minimum gap between two overlapping sends",
                     xbt_cfgelm_double, NULL, 1, 1, /* default is set in network.c */
                     _sg_cfg_cb__sender_gap, NULL);

    double_default_value = 1.0; // FIXME use setdefault everywhere here!
    xbt_cfg_register(&_sg_cfg_set, "network/latency_factor",
                     "Correction factor to apply to the provided latency (default value set by network model)",
                     xbt_cfgelm_double, &double_default_value, 1, 1,
                     _sg_cfg_cb__latency_factor, NULL);
    double_default_value = 1.0;
    xbt_cfg_register(&_sg_cfg_set, "network/bandwidth_factor",
                     "Correction factor to apply to the provided bandwidth (default value set by network model)",
                     xbt_cfgelm_double, &double_default_value, 1, 1,
                     _sg_cfg_cb__bandwidth_factor, NULL);

    xbt_cfg_register(&_sg_cfg_set, "network/weight_S",
                     "Correction factor to apply to the weight of competing streams (default value set by network model)",
                     xbt_cfgelm_double, NULL, 1, 1, /* default is set in network.c */
                     _sg_cfg_cb__weight_S, NULL);

    /* Inclusion path */
    xbt_cfg_register(&_sg_cfg_set, "path",
                     "Lookup path for inclusions in platform and deployment XML files",
                     xbt_cfgelm_string, NULL, 0, 0,
                     _sg_cfg_cb__surf_path, NULL);

    default_value = xbt_strdup("off");
    xbt_cfg_register(&_sg_cfg_set, "cpu/maxmin_selective_update",
                     "Update the constraint set propagating recursively to others constraints (off by default when optim is set to lazy)",
                     xbt_cfgelm_boolean, &default_value, 0, 1,
                     NULL, NULL);
    default_value = xbt_strdup("off");
    xbt_cfg_register(&_sg_cfg_set, "network/maxmin_selective_update",
                     "Update the constraint set propagating recursively to others constraints (off by default when optim is set to lazy)",
                     xbt_cfgelm_boolean, &default_value, 0, 1,
                     NULL, NULL);

#ifdef HAVE_MC
    /* do model-checking */
    default_value = xbt_strdup("off");
    xbt_cfg_register(&_sg_cfg_set, "model-check",
                     "Verify the system through model-checking instead of simulating it (EXPERIMENTAL)",
                     xbt_cfgelm_boolean, NULL, 0, 1,
                     _sg_cfg_cb_model_check, NULL);
    xbt_cfg_setdefault_boolean(_sg_cfg_set, "model-check", default_value);

    /* do stateful model-checking */
    default_value = xbt_strdup("off");
    xbt_cfg_register(&_sg_cfg_set, "model-check/checkpoint",
                     "Specify the amount of steps between checkpoints during stateful model-checking (default: off => stateless verification). "
                     "If value=on, one checkpoint is saved for each step => faster verification, but huge memory consumption; higher values are good compromises between speed and memory consumption.",
                     xbt_cfgelm_boolean, NULL, 0, 1,
                     _mc_cfg_cb_checkpoint, NULL);
    xbt_cfg_setdefault_boolean(_sg_cfg_set, "model-check/checkpoint", default_value);
    
    /* do liveness model-checking */
    xbt_cfg_register(&_sg_cfg_set, "model-check/property",
                     "Specify the name of the file containing the property. It must be the result of the ltl2ba program.",
                     xbt_cfgelm_string, NULL, 0, 1,
                     _mc_cfg_cb_property, NULL);
    xbt_cfg_setdefault_string(_sg_cfg_set, "model-check/property", "");

    /* Specify the kind of model-checking reduction */
    xbt_cfg_register(&_sg_cfg_set, "model-check/reduction",
                     "Specify the kind of exploration reduction (either none or DPOR)",
                     xbt_cfgelm_string, NULL, 0, 1,
                     _mc_cfg_cb_reduce, NULL);
    xbt_cfg_setdefault_string(_sg_cfg_set, "model-check/reduction", "dpor");

    /* Enable/disable timeout for wait requests with model-checking */
    default_value = xbt_strdup("off");
    xbt_cfg_register(&_sg_cfg_set, "model-check/timeout",
                     "Enable/Disable timeout for wait requests",
                     xbt_cfgelm_boolean, NULL, 0, 1,
                     _mc_cfg_cb_timeout, NULL);
    xbt_cfg_setdefault_boolean(_sg_cfg_set, "model-check/timeout", default_value);

    /* Set max depth exploration */
    xbt_cfg_register(&_sg_cfg_set, "model-check/max_depth",
                     "Specify the max depth of exploration (default : 1000)",
                     xbt_cfgelm_int, NULL, 0, 1,
                     _mc_cfg_cb_max_depth, NULL);
    xbt_cfg_setdefault_int(_sg_cfg_set, "model-check/max_depth", 1000);

    /* Set number of visited state stored for state comparison reduction*/
    xbt_cfg_register(&_sg_cfg_set, "model-check/visited",
                     "Specify the number of visited state stored for state comparison reduction. If value=5, the last 5 visited states are stored",
                     xbt_cfgelm_int, NULL, 0, 1,
                     _mc_cfg_cb_visited, NULL);
    xbt_cfg_setdefault_int(_sg_cfg_set, "model-check/visited", 0);

    /* Set file name for dot output of graph state */
    xbt_cfg_register(&_sg_cfg_set, "model-check/dot_output",
                     "Specify the name of dot file corresponding to graph state",
                     xbt_cfgelm_string, NULL, 0, 1,
                     _mc_cfg_cb_dot_output, NULL);
    xbt_cfg_setdefault_string(_sg_cfg_set, "model-check/dot_output", "");
#endif

    /* do verbose-exit */
    default_value = xbt_strdup("on");
    xbt_cfg_register(&_sg_cfg_set, "verbose-exit",
                     "Activate the \"do nothing\" mode in Ctrl-C",
                     xbt_cfgelm_boolean, &default_value, 0, 1,
                     _sg_cfg_cb_verbose_exit, NULL);
    
    
    /* context factory */
    default_value = xbt_strdup("ucontext");
    xbt_cfg_register(&_sg_cfg_set, "contexts/factory",
                     "Context factory to use in SIMIX (ucontext, thread or raw)",
                     xbt_cfgelm_string, &default_value, 1, 1, _sg_cfg_cb_context_factory, NULL);

    /* stack size of contexts in Ko */
    default_value_int = 128;
    xbt_cfg_register(&_sg_cfg_set, "contexts/stack_size",
                     "Stack size of contexts in Kib (ucontext or raw only)",
                     xbt_cfgelm_int, &default_value_int, 1, 1,
                     _sg_cfg_cb_context_stack_size, NULL);

    /* number of parallel threads for user processes */
    default_value_int = 1;
    xbt_cfg_register(&_sg_cfg_set, "contexts/nthreads",
                     "Number of parallel threads used to execute user contexts",
                     xbt_cfgelm_int, &default_value_int, 1, 1,
                     _sg_cfg_cb_contexts_nthreads, NULL);

    /* minimal number of user contexts to be run in parallel */
    default_value_int = 2;
    xbt_cfg_register(&_sg_cfg_set, "contexts/parallel_threshold",
        "Minimal number of user contexts to be run in parallel (raw contexts only)",
        xbt_cfgelm_int, &default_value_int, 1, 1,
        _sg_cfg_cb_contexts_parallel_threshold, NULL);

    /* synchronization mode for parallel user contexts */
#ifdef HAVE_FUTEX_H
    default_value = xbt_strdup("futex");
#else //No futex on mac and posix is unimplememted yet
    default_value = xbt_strdup("busy_wait");
#endif
    xbt_cfg_register(&_sg_cfg_set, "contexts/synchro",
        "Synchronization mode to use when running contexts in parallel (either futex, posix or busy_wait)",
        xbt_cfgelm_string, &default_value, 1, 1,
        _sg_cfg_cb_contexts_parallel_mode, NULL);

    default_value = xbt_strdup("no");
    xbt_cfg_register(&_sg_cfg_set, "network/coordinates",
                     "\"yes\" or \"no\", specifying whether we use a coordinate-based routing (as Vivaldi)",
                     xbt_cfgelm_boolean, &default_value, 1, 1,
                     _sg_cfg_cb__surf_network_coordinates, NULL);
    xbt_cfg_setdefault_boolean(_sg_cfg_set, "network/coordinates", default_value);

    default_value = xbt_strdup("no");
    xbt_cfg_register(&_sg_cfg_set, "network/crosstraffic",
                     "Activate the interferences between uploads and downloads for fluid max-min models (LV08, CM02)",
                     xbt_cfgelm_boolean, &default_value, 0, 1,
                     _sg_cfg_cb__surf_network_crosstraffic, NULL);
    xbt_cfg_setdefault_boolean(_sg_cfg_set, "network/crosstraffic", default_value);

#ifdef HAVE_GTNETS
    xbt_cfg_register(&_sg_cfg_set, "gtnets/jitter",
                     "Double value to oscillate the link latency, uniformly in random interval [-latency*gtnets_jitter,latency*gtnets_jitter)",
                     xbt_cfgelm_double, NULL, 1, 1,
                     _sg_cfg_cb__gtnets_jitter, NULL);
    xbt_cfg_setdefault_double(_sg_cfg_set, "gtnets/jitter", 0.0);

    default_value_int = 10;
    xbt_cfg_register(&_sg_cfg_set, "gtnets/jitter_seed",
                     "Use a positive seed to reproduce jitted results, value must be in [1,1e8], default is 10",
                     xbt_cfgelm_int, &default_value_int, 0, 1,
                     _sg_cfg_cb__gtnets_jitter_seed, NULL);
#endif
#ifdef HAVE_NS3
    xbt_cfg_register(&_sg_cfg_set, "ns3/TcpModel",
                     "The ns3 tcp model can be : NewReno or Reno or Tahoe",
                     xbt_cfgelm_string, NULL, 1, 1,
                     NULL, NULL);
    xbt_cfg_setdefault_string(_sg_cfg_set, "ns3/TcpModel", "default");
#endif

#ifdef HAVE_SMPI
    double default_reference_speed = 20000.0;
    xbt_cfg_register(&_sg_cfg_set, "smpi/running_power",
                     "Power of the host running the simulation (in flop/s). Used to bench the operations.",
                     xbt_cfgelm_double, &default_reference_speed, 1, 1, NULL,
                     NULL);

    default_value = xbt_strdup("no");
    xbt_cfg_register(&_sg_cfg_set, "smpi/display_timing",
                     "Boolean indicating whether we should display the timing after simulation.",
                     xbt_cfgelm_boolean, &default_value, 1, 1, NULL,
                     NULL);
    xbt_cfg_setdefault_boolean(_sg_cfg_set, "smpi/display_timing", default_value);

    double default_threshold = 1e-6;
    xbt_cfg_register(&_sg_cfg_set, "smpi/cpu_threshold",
                     "Minimal computation time (in seconds) not discarded.",
                     xbt_cfgelm_double, &default_threshold, 1, 1, NULL,
                     NULL);

    int default_small_messages_threshold = 0;
    xbt_cfg_register(&_sg_cfg_set, "smpi/async_small_thres",
                     "Maximal size of messages that are to be sent asynchronously, without waiting for the receiver",
                     xbt_cfgelm_int, &default_small_messages_threshold, 1, 1, NULL,
                     NULL);

    int default_send_is_detached_threshold = 65536;
    xbt_cfg_register(&_sg_cfg_set, "smpi/send_is_detached_thres",
                     "Threshold of message size where MPI_Send stops behaving like MPI_Isend and becomes MPI_Ssend",
                     xbt_cfgelm_int, &default_send_is_detached_threshold, 1, 1, NULL,
                     NULL);

    //For smpi/bw_factor and smpi/lat_factor
    //Default value have to be "threshold0:value0;threshold1:value1;...;thresholdN:valueN"
    //test is if( size >= thresholdN ) return valueN;
    //Values can be modified with command line --cfg=smpi/bw_factor:"threshold0:value0;threshold1:value1;...;thresholdN:valueN"
    //  or with tag config put line <prop id="smpi/bw_factor" value="threshold0:value0;threshold1:value1;...;thresholdN:valueN"></prop>
    xbt_cfg_register(&_sg_cfg_set, "smpi/bw_factor",
                     "Bandwidth factors for smpi.",
                     xbt_cfgelm_string, NULL, 1, 1, NULL,
                     NULL);
    xbt_cfg_setdefault_string(_sg_cfg_set, "smpi/bw_factor", "65472:0.940694;15424:0.697866;9376:0.58729;5776:1.08739;3484:0.77493;1426:0.608902;732:0.341987;257:0.338112;0:0.812084");

    xbt_cfg_register(&_sg_cfg_set, "smpi/lat_factor",
                     "Latency factors for smpi.",
                     xbt_cfgelm_string, NULL, 1, 1, NULL,
                     NULL);
    xbt_cfg_setdefault_string(_sg_cfg_set, "smpi/lat_factor", "65472:11.6436;15424:3.48845;9376:2.59299;5776:2.18796;3484:1.88101;1426:1.61075;732:1.9503;257:1.95341;0:2.01467");

    xbt_cfg_register(&_sg_cfg_set, "smpi/os",
                     "Small messages timings (MPI_Send minimum time for small messages)",
                     xbt_cfgelm_string, NULL, 1, 1, NULL,
                     NULL);
    xbt_cfg_setdefault_string(_sg_cfg_set, "smpi/os", "1:0:0:0:0");

    xbt_cfg_register(&_sg_cfg_set, "smpi/ois",
                     "Small messages timings (MPI_Isend minimum time for small messages)",
                     xbt_cfgelm_string, NULL, 1, 1, NULL,
                     NULL);
    xbt_cfg_setdefault_string(_sg_cfg_set, "smpi/ois", "1:0:0:0:0");

    xbt_cfg_register(&_sg_cfg_set, "smpi/or",
                     "Small messages timings (MPI_Recv minimum time for small messages)",
                     xbt_cfgelm_string, NULL, 1, 1, NULL,
                     NULL);
    xbt_cfg_setdefault_string(_sg_cfg_set, "smpi/or", "1:0:0:0:0");
    double default_iprobe_time = 1e-4;
    xbt_cfg_register(&_sg_cfg_set, "smpi/iprobe",
                     "Minimum time to inject inside a call to MPI_Iprobe",
                     xbt_cfgelm_double, &default_iprobe_time, 1, 1, NULL,
                     NULL);
    default_value = xbt_strdup("default");
    xbt_cfg_register(&_sg_cfg_set, "smpi/coll_selector",
		     "Which collective selector to use",
		     xbt_cfgelm_string, &default_value, 1, 1, NULL,
		     NULL);
    xbt_cfg_register(&_sg_cfg_set, "smpi/allgather",
		     "Which collective to use for allgather",
		     xbt_cfgelm_string, NULL, 1, 1, &_sg_cfg_cb__coll_allgather,
		     NULL);

    xbt_cfg_register(&_sg_cfg_set, "smpi/allgatherv",
		     "Which collective to use for allgatherv",
		     xbt_cfgelm_string, NULL, 1, 1, &_sg_cfg_cb__coll_allgatherv,
		     NULL);

    xbt_cfg_register(&_sg_cfg_set, "smpi/allreduce",
		     "Which collective to use for allreduce",
		     xbt_cfgelm_string, NULL, 1, 1, &_sg_cfg_cb__coll_allreduce,
		     NULL);

    xbt_cfg_register(&_sg_cfg_set, "smpi/alltoall",
		     "Which collective to use for alltoall",
		     xbt_cfgelm_string, NULL, 1, 1, &_sg_cfg_cb__coll_alltoall,
		     NULL);

    xbt_cfg_register(&_sg_cfg_set, "smpi/alltoallv",
		     "Which collective to use for alltoallv",
		     xbt_cfgelm_string, NULL, 1, 1, &_sg_cfg_cb__coll_alltoallv,
		     NULL);

    xbt_cfg_register(&_sg_cfg_set, "smpi/bcast",
		     "Which collective to use for bcast",
		     xbt_cfgelm_string, NULL, 1, 1, &_sg_cfg_cb__coll_bcast,
		     NULL);

    xbt_cfg_register(&_sg_cfg_set, "smpi/reduce",
		     "Which collective to use for reduce",
		     xbt_cfgelm_string, NULL, 1, 1, &_sg_cfg_cb__coll_reduce,
		     NULL);
#endif // HAVE_SMPI

    if (!surf_path) {
      /* retrieves the current directory of the        current process */
      const char *initial_path = __surf_get_initial_path();
      xbt_assert((initial_path),
                  "__surf_get_initial_path() failed! Can't resolves current Windows directory");

      surf_path = xbt_dynar_new(sizeof(char *), NULL);
      xbt_cfg_setdefault_string(_sg_cfg_set, "path", initial_path);
    }

    _sg_init_status = 1;

    sg_config_cmd_line(argc, argv);

    xbt_mallocator_initialization_is_done(SIMIX_context_is_parallel());

  } else {
    XBT_WARN("Call to sg_config_init() after initialization ignored");
  }
}

void sg_config_finalize(void)
{
  if (!_sg_init_status)
    return;                     /* Not initialized yet. Nothing to do */

  xbt_cfg_free(&_sg_cfg_set);
  _sg_init_status = 0;
}

/* Pick the right models for CPU, net and workstation, and call their model_init_preparse */
void surf_config_models_setup()
{
  char *workstation_model_name;
  int workstation_id = -1;
  char *network_model_name = NULL;
  char *cpu_model_name = NULL;
  int storage_id = -1;
  char *storage_model_name = NULL;

  workstation_model_name =
      xbt_cfg_get_string(_sg_cfg_set, "workstation/model");
  network_model_name = xbt_cfg_get_string(_sg_cfg_set, "network/model");
  cpu_model_name = xbt_cfg_get_string(_sg_cfg_set, "cpu/model");
  storage_model_name = xbt_cfg_get_string(_sg_cfg_set, "storage/model");

  /* Check whether we use a net/cpu model differing from the default ones, in which case
   * we should switch to the "compound" workstation model to correctly dispatch stuff to
   * the right net/cpu models.
   */

  if((!xbt_cfg_is_default_value(_sg_cfg_set, "network/model") ||
    !xbt_cfg_is_default_value(_sg_cfg_set, "cpu/model")) &&
    xbt_cfg_is_default_value(_sg_cfg_set, "workstation/model"))
  {
      const char *val = "compound";
      XBT_INFO
          ("Switching workstation model to compound since you changed the network and/or cpu model(s)");
      xbt_cfg_set_string(_sg_cfg_set, "workstation/model", val);
      workstation_model_name = (char *) "compound";
  }

  XBT_DEBUG("Workstation model: %s", workstation_model_name);
  workstation_id =
      find_model_description(surf_workstation_model_description,
                             workstation_model_name);
  if (!strcmp(workstation_model_name, "compound")) {
    int network_id = -1;
    int cpu_id = -1;

    xbt_assert(cpu_model_name,
                "Set a cpu model to use with the 'compound' workstation model");

    xbt_assert(network_model_name,
                "Set a network model to use with the 'compound' workstation model");

    network_id =
        find_model_description(surf_network_model_description,
                               network_model_name);
    cpu_id =
        find_model_description(surf_cpu_model_description, cpu_model_name);

    surf_cpu_model_description[cpu_id].model_init_preparse();
    surf_network_model_description[network_id].model_init_preparse();
  }

  XBT_DEBUG("Call workstation_model_init");
  surf_workstation_model_description[workstation_id].model_init_preparse();

  XBT_DEBUG("Call storage_model_init");
  storage_id = find_model_description(surf_storage_model_description, storage_model_name);
  surf_storage_model_description[storage_id].model_init_preparse();

  /* ********************************************************************* */
  /* TUTORIAL: New model                                                   */
  int new_model_id = -1;
  char *new_model_name = NULL;
  new_model_name = xbt_cfg_get_string(_sg_cfg_set, "new_model/model");
  XBT_DEBUG("Call new model_init");
  new_model_id = find_model_description(surf_new_model_description, new_model_name);
  surf_new_model_description[new_model_id].model_init_preparse();
  /* ********************************************************************* */
}

int sg_cfg_get_int(const char* name)
{
	return xbt_cfg_get_int(_sg_cfg_set,name);
}
double sg_cfg_get_double(const char* name)
{
	return xbt_cfg_get_double(_sg_cfg_set,name);
}
char* sg_cfg_get_string(const char* name)
{
	return xbt_cfg_get_string(_sg_cfg_set,name);
}
int sg_cfg_get_boolean(const char* name)
{
	return xbt_cfg_get_boolean(_sg_cfg_set,name);
}
void sg_cfg_get_peer(const char *name, char **peer, int *port)
{
	xbt_cfg_get_peer(_sg_cfg_set,name, peer, port);
}
xbt_dynar_t sg_cfg_get_dynar(const char* name)
{
	return xbt_cfg_get_dynar(_sg_cfg_set,name);
}
