/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* surf_config: configuration infrastructure for the simulation world       */

#include "xbt/config.h"
#include "xbt/str.h"
#include "surf/surf_private.h"
#include "surf/surf_routing.h"	/* COORD_HOST_LEVEL and COORD_ASR_LEVEL */
#include "simix/context.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_config, surf,
                                "About the configuration of surf (and the rest of the simulation)");

xbt_cfg_t _surf_cfg_set = NULL;

static void LOG_help(void)
{
  printf(
"Description of the logging output:\n"
"\n"
"   Threshold configuration: --log=CATEGORY_NAME.thres:PRIORITY_LEVEL\n"
"      CATEGORY_NAME: defined in code with function 'XBT_LOG_NEW_CATEGORY'\n"
"      PRIORITY_LEVEL: the level to print (trace,debug,verbose,info,warning,error,critical)\n"
"         -> trace: enter and return of some functions\n"
"         -> debug: crufty output\n"
"         -> verbose: verbose output for the user wanting more\n"
"         -> info: output about the regular functionning\n"
"         -> warning: minor issue encountered\n"
"         -> error: issue encountered\n"
"         -> critical: major issue encountered\n"
"\n"
"   Format configuration: --log=CATEGORY_NAME.fmt:OPTIONS\n"
"      OPTIONS may be:\n"
"         -> %%%%: the %% char\n"
"         -> %%n: platform-dependent line separator (LOG4J compatible)\n"
"         -> %%e: plain old space (SimGrid extension)\n"
"\n"
"         -> %%m: user-provided message\n"
"\n"
"         -> %%c: Category name (LOG4J compatible)\n"
"         -> %%p: Priority name (LOG4J compatible)\n"
"\n"
"         -> %%h: Hostname (SimGrid extension)\n"
"         -> %%P: Process name (SimGrid extension)\n"
"         -> %%t: Thread \"name\" (LOG4J compatible -- actually the address of the thread in memory)\n"
"         -> %%i: Process PID (SimGrid extension -- this is a 'i' as in 'i'dea)\n"
"\n"
"         -> %%F: file name where the log event was raised (LOG4J compatible)\n"
"         -> %%l: location where the log event was raised (LOG4J compatible, like '%%F:%%L' -- this is a l as in 'l'etter)\n"
"         -> %%L: line number where the log event was raised (LOG4J compatible)\n"
"         -> %%M: function name (LOG4J compatible -- called method name here of course).\n"
"                 Defined only when using gcc because there is no __FUNCTION__ elsewhere.\n"
"\n"
"         -> %%b: full backtrace (Called %%throwable in LOG4J). Defined only under windows or when using the GNU libc because\n"
"                 backtrace() is not defined elsewhere, and we only have a fallback for windows boxes, not mac ones for example.\n"
"         -> %%B: short backtrace (only the first line of the %%b). Called %%throwable{short} in LOG4J; defined where %%b is.\n"
"\n"
"         -> %%d: date (UNIX-like epoch)\n"
"         -> %%r: application age (time elapsed since the beginning of the application)\n"
    );
}

/* Parse the command line, looking for options */
static void surf_config_cmd_line(int *argc, char **argv)
{
  int i, j;
  char *opt;

  for (i = 1; i < *argc; i++) {
    int remove_it = 0;
    if (!strncmp(argv[i], "--cfg=", strlen("--cfg="))) {
      opt = strchr(argv[i], '=');
      opt++;

      xbt_cfg_set_parse(_surf_cfg_set, opt);
      XBT_DEBUG("Did apply '%s' as config setting", opt);
      remove_it = 1;
    } else if (!strncmp(argv[i], "--cfg-help", strlen("--cfg-help") + 1) ||
               !strncmp(argv[i], "--help", strlen("--help") + 1)) {
      printf
          ("Description of the configuration accepted by this simulator:\n");
      xbt_cfg_help(_surf_cfg_set);
      printf(
"\n"
"You can also use --help-models to see the details of all models known by this simulator.\n"
#ifdef HAVE_TRACING
"\n"
"You can also use --help-tracing to see the details of all tracing options known by this simulator.\n"
#endif
"\n"
"You can also use --help-logs to see the details of logging output.\n"
"\n"
        );
      exit(0);
    } else if (!strncmp(argv[i], "--help-models", strlen("--help-models") + 1)) {
      model_help("workstation", surf_workstation_model_description);
      printf("\n");
      model_help("CPU", surf_cpu_model_description);
      printf("\n");
      model_help("network", surf_network_model_description);
      printf("\nLong description of all optimization levels accepted by the models of this simulator:\n");
      for (i = 0; surf_optimization_mode_description[i].name; i++)
        printf("  %s: %s\n", surf_optimization_mode_description[i].name, surf_optimization_mode_description[i].description);
      printf("Both network and CPU models have 'Lazy' as default optimization level\n");
      exit(0);
    } else if (!strncmp(argv[i], "--help-logs", strlen("--help-logs") + 1)) {
      LOG_help ();
      exit(0);
#ifdef HAVE_TRACING
    } else if (!strncmp(argv[i], "--help-tracing", strlen("--help-tracing") + 1)) {
      TRACE_help (1);
      exit(0);
#endif
    }
    if (remove_it) {            /*remove this from argv */
      for (j = i + 1; j < *argc; j++) {
        argv[j - 1] = argv[j];
      }

      argv[j - 1] = NULL;
      (*argc)--;
      i--;                      /* compensate effect of next loop incrementation */
    }
  }
}


int _surf_init_status = 0;      /* 0: beginning of time;
                                   1: pre-inited (cfg_set created);
                                   2: inited (running) */

/* callback of the workstation/model variable */
static void _surf_cfg_cb__workstation_model(const char *name, int pos)
{
  char *val;

  xbt_assert(_surf_init_status < 2,
              "Cannot change the model after the initialization");

  val = xbt_cfg_get_string(_surf_cfg_set, name);

  if (!strcmp(val, "help")) {
    model_help("workstation", surf_workstation_model_description);
    exit(0);
  }

  /* Make sure that the model exists */
  find_model_description(surf_workstation_model_description, val);
}

/* callback of the cpu/model variable */
static void _surf_cfg_cb__cpu_model(const char *name, int pos)
{
  char *val;

  xbt_assert(_surf_init_status < 2,
              "Cannot change the model after the initialization");

  val = xbt_cfg_get_string(_surf_cfg_set, name);

  if (!strcmp(val, "help")) {
    model_help("CPU", surf_cpu_model_description);
    exit(0);
  }

  /* New Module missing */
  find_model_description(surf_cpu_model_description, val);
}

/* callback of the cpu/model variable */
static void _surf_cfg_cb__optimization_mode(const char *name, int pos)
{
  char *val;

  xbt_assert(_surf_init_status < 2,
              "Cannot change the model after the initialization");

  val = xbt_cfg_get_string(_surf_cfg_set, name);

  if (!strcmp(val, "help")) {
    model_help("optimization", surf_optimization_mode_description);
    exit(0);
  }

  /* New Module missing */
  find_model_description(surf_optimization_mode_description, val);
}

/* callback of the cpu/model variable */
static void _surf_cfg_cb__storage_mode(const char *name, int pos)
{
  char *val;

  xbt_assert(_surf_init_status < 2,
              "Cannot change the model after the initialization");

  val = xbt_cfg_get_string(_surf_cfg_set, name);

  if (!strcmp(val, "help")) {
    model_help("storage", surf_storage_model_description);
    exit(0);
  }

  /* New Module missing */
  find_model_description(surf_storage_model_description, val);
}

/* callback of the workstation_model variable */
static void _surf_cfg_cb__network_model(const char *name, int pos)
{
  char *val;

  xbt_assert(_surf_init_status < 2,
              "Cannot change the model after the initialization");

  val = xbt_cfg_get_string(_surf_cfg_set, name);

  if (!strcmp(val, "help")) {
    model_help("network", surf_network_model_description);
    exit(0);
  }

  /* New Module missing */
  find_model_description(surf_network_model_description, val);
}


/* callbacks of the network models values */
static void _surf_cfg_cb__tcp_gamma(const char *name, int pos)
{
  sg_tcp_gamma = xbt_cfg_get_double(_surf_cfg_set, name);
}

static void _surf_cfg_cb__maxmin_precision(const char* name, int pos)
{
  sg_maxmin_precision = xbt_cfg_get_double(_surf_cfg_set, name);
}

static void _surf_cfg_cb__sender_gap(const char* name, int pos)
{
  sg_sender_gap = xbt_cfg_get_double(_surf_cfg_set, name);
}

static void _surf_cfg_cb__latency_factor(const char *name, int pos)
{
  sg_latency_factor = xbt_cfg_get_double(_surf_cfg_set, name);
}

static void _surf_cfg_cb__bandwidth_factor(const char *name, int pos)
{
  sg_bandwidth_factor = xbt_cfg_get_double(_surf_cfg_set, name);
}

static void _surf_cfg_cb__weight_S(const char *name, int pos)
{
  sg_weight_S_parameter = xbt_cfg_get_double(_surf_cfg_set, name);
}

/* callback of the inclusion path */
static void _surf_cfg_cb__surf_path(const char *name, int pos)
{
  char *path = xbt_cfg_get_string_at(_surf_cfg_set, name, pos);
  xbt_dynar_push(surf_path, &path);
}

/* callback to decide if we want to use the model-checking */
#include "xbt_modinter.h"
extern int _surf_do_model_check;   /* this variable lives in xbt_main until I find a right location for it */

static void _surf_cfg_cb_model_check(const char *name, int pos)
{
  _surf_do_model_check = xbt_cfg_get_int(_surf_cfg_set, name);

  if (_surf_do_model_check) {
    /* Tell modules using mallocators that they shouldn't. MC don't like them */
    xbt_fifo_preinit();
    xbt_dict_preinit();
  }
}

extern int _surf_do_verbose_exit;

static void _surf_cfg_cb_verbose_exit(const char *name, int pos)
{
  _surf_do_verbose_exit = xbt_cfg_get_int(_surf_cfg_set, name);
}


static void _surf_cfg_cb_context_factory(const char *name, int pos)
{
  smx_context_factory_name = xbt_cfg_get_string(_surf_cfg_set, name);
}

static void _surf_cfg_cb_context_stack_size(const char *name, int pos)
{
  smx_context_stack_size = xbt_cfg_get_int(_surf_cfg_set, name) * 1024;
}

static void _surf_cfg_cb_contexts_nthreads(const char *name, int pos)
{
  SIMIX_context_set_nthreads(xbt_cfg_get_int(_surf_cfg_set, name));
}

static void _surf_cfg_cb_contexts_parallel_threshold(const char *name, int pos)
{
  SIMIX_context_set_parallel_threshold(xbt_cfg_get_int(_surf_cfg_set, name));
}

static void _surf_cfg_cb_contexts_parallel_mode(const char *name, int pos)
{
  const char* mode_name = xbt_cfg_get_string(_surf_cfg_set, name);
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

static void _surf_cfg_cb_surf_nthreads(const char *name, int pos)
{
  surf_set_nthreads(xbt_cfg_get_int(_surf_cfg_set, name));
}

static void _surf_cfg_cb__surf_network_coordinates(const char *name,
                                                   int pos)
{
  char *val = xbt_cfg_get_string(_surf_cfg_set, name);
  if (!strcmp(val, "yes")) {
    if (!COORD_HOST_LEVEL) {
      COORD_HOST_LEVEL = xbt_lib_add_level(host_lib,xbt_dynar_free_voidp);
      COORD_ASR_LEVEL  = xbt_lib_add_level(as_router_lib,xbt_dynar_free_voidp);
    }
  } else if (!strcmp(val, "no")) {
    if (COORD_HOST_LEVEL)
      xbt_die("Setting of whether to use coordinate cannot be disabled once set.");
  } else {
    xbt_die("Command line setting of whether to use coordinates must be either \"yes\" or \"no\"");
  }
}

static void _surf_cfg_cb__surf_network_crosstraffic(const char *name,
                                                  int pos)
{
  sg_network_crosstraffic = xbt_cfg_get_int(_surf_cfg_set, name);
}

#ifdef HAVE_GTNETS
static void _surf_cfg_cb__gtnets_jitter(const char *name, int pos)
{
  sg_gtnets_jitter = xbt_cfg_get_double(_surf_cfg_set, name);
}

static void _surf_cfg_cb__gtnets_jitter_seed(const char *name, int pos)
{
  sg_gtnets_jitter_seed = xbt_cfg_get_int(_surf_cfg_set, name);
}
#endif

/* create the config set, register what should be and parse the command line*/
void surf_config_init(int *argc, char **argv)
{
  char *description = xbt_malloc(1024), *p = description;
  char *default_value;
  double double_default_value;
  int default_value_int;
  int i;

  /* Create the configuration support */
  if (_surf_init_status == 0) { /* Only create stuff if not already inited */
    _surf_init_status = 1;

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
    xbt_cfg_register(&_surf_cfg_set, "cpu/model", description, xbt_cfgelm_string,
                     &default_value, 1, 1, &_surf_cfg_cb__cpu_model, NULL);

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
    xbt_cfg_register(&_surf_cfg_set, "cpu/optim", description, xbt_cfgelm_string,
                     &default_value, 1, 1, &_surf_cfg_cb__optimization_mode, NULL);

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
    xbt_cfg_register(&_surf_cfg_set, "storage/model", description, xbt_cfgelm_string,
                     &default_value, 1, 1, &_surf_cfg_cb__storage_mode,
                     NULL);

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
    xbt_cfg_register(&_surf_cfg_set, "network/model", description, xbt_cfgelm_string,
                     &default_value, 1, 1, &_surf_cfg_cb__network_model,
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
    xbt_cfg_register(&_surf_cfg_set, "network/optim", description, xbt_cfgelm_string,
                     &default_value, 1, 1, &_surf_cfg_cb__optimization_mode, NULL);

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
    xbt_cfg_register(&_surf_cfg_set, "workstation/model", description, xbt_cfgelm_string,
                     &default_value, 1, 1,
                     &_surf_cfg_cb__workstation_model, NULL);

    xbt_free(description);

    xbt_cfg_register(&_surf_cfg_set, "network/TCP_gamma",
                     "Size of the biggest TCP window (cat /proc/sys/net/ipv4/tcp_[rw]mem for recv/send window; Use the last given value, which is the max window size)",
                     xbt_cfgelm_double, NULL, 1, 1,
                     _surf_cfg_cb__tcp_gamma, NULL);
    xbt_cfg_setdefault_double(_surf_cfg_set, "network/TCP_gamma", 20000.0);

    xbt_cfg_register(&_surf_cfg_set, "maxmin/precision",
                     "Numerical precision used when updating simulation models (epsilon in double comparisons)",
                     xbt_cfgelm_double, NULL, 1, 1, _surf_cfg_cb__maxmin_precision, NULL);
    xbt_cfg_setdefault_double(_surf_cfg_set, "maxmin/precision", 0.00001); // FIXME use setdefault everywhere here!

    /* The parameters of network models */

    double_default_value = 0.0;
    xbt_cfg_register(&_surf_cfg_set, "network/sender_gap",
                     "Minimum gap between two overlapping sends",
                     xbt_cfgelm_double, &double_default_value, 1, 1,
                     _surf_cfg_cb__sender_gap, NULL);

    double_default_value = 1.0;
    xbt_cfg_register(&_surf_cfg_set, "network/latency_factor",
                     "Correction factor to apply to the provided latency (default value set by network model)",
                     xbt_cfgelm_double, &double_default_value, 1, 1,
                     _surf_cfg_cb__latency_factor, NULL);
    double_default_value = 1.0;
    xbt_cfg_register(&_surf_cfg_set, "network/bandwidth_factor",
                     "Correction factor to apply to the provided bandwidth (default value set by network model)",
                     xbt_cfgelm_double, &double_default_value, 1, 1,
                     _surf_cfg_cb__bandwidth_factor, NULL);
    double_default_value = 0.0;
    xbt_cfg_register(&_surf_cfg_set, "network/weight_S",
                     "Correction factor to apply to the weight of competing streams(default value set by network model)",
                     xbt_cfgelm_double, &double_default_value, 1, 1,
                     _surf_cfg_cb__weight_S, NULL);

    /* Inclusion path */
    xbt_cfg_register(&_surf_cfg_set, "path",
                     "Lookup path for inclusions in platform and deployment XML files",
                     xbt_cfgelm_string, NULL, 0, 0,
                     _surf_cfg_cb__surf_path, NULL);

    default_value_int = 0;
    xbt_cfg_register(&_surf_cfg_set, "cpu/maxmin_selective_update",
                     "Update the constraint set propagating recursively to others constraints (1 by default when optim is set to lazy)",
                     xbt_cfgelm_int, &default_value_int, 0, 1,
                     NULL, NULL);
    default_value_int = 0;
    xbt_cfg_register(&_surf_cfg_set, "network/maxmin_selective_update",
                     "Update the constraint set propagating recursively to others constraints (1 by default when optim is set to lazy)",
                     xbt_cfgelm_int, &default_value_int, 0, 1,
                     NULL, NULL);

    /* do model-check */
    default_value_int = 0;
    xbt_cfg_register(&_surf_cfg_set, "model-check",
                     "Activate the model-checking of the \"simulated\" system (EXPERIMENTAL -- msg only for now)",
                     xbt_cfgelm_int, &default_value_int, 0, 1,
                     _surf_cfg_cb_model_check, NULL);
    
    /*
       FIXME: this function is not setting model-check to it's default value because
       internally it calls to variable->cb_set that in this case is the function 
       _surf_cfg_cb_model_check which sets it's value to 1 (instead of the default value 0)
       xbt_cfg_set_int(_surf_cfg_set, "model-check", default_value_int); */

    /* do verbose-exit */
    default_value_int = 1;
    xbt_cfg_register(&_surf_cfg_set, "verbose-exit",
                     "Activate the \"do nothing\" mode in Ctrl-C",
                     xbt_cfgelm_int, &default_value_int, 0, 1,
                     _surf_cfg_cb_verbose_exit, NULL);
    
    
    /* context factory */
    default_value = xbt_strdup("ucontext");
    xbt_cfg_register(&_surf_cfg_set, "contexts/factory",
                     "Context factory to use in SIMIX (ucontext, thread or raw)",
                     xbt_cfgelm_string, &default_value, 1, 1, _surf_cfg_cb_context_factory, NULL);

    /* stack size of contexts in Ko */
    default_value_int = 128;
    xbt_cfg_register(&_surf_cfg_set, "contexts/stack_size",
                     "Stack size of contexts in Kib (ucontext or raw only)",
                     xbt_cfgelm_int, &default_value_int, 1, 1,
                     _surf_cfg_cb_context_stack_size, NULL);

    /* number of parallel threads for user processes */
    default_value_int = 1;
    xbt_cfg_register(&_surf_cfg_set, "contexts/nthreads",
                     "Number of parallel threads used to execute user contexts",
                     xbt_cfgelm_int, &default_value_int, 1, 1,
                     _surf_cfg_cb_contexts_nthreads, NULL);

    /* minimal number of user contexts to be run in parallel */
    default_value_int = 2;
    xbt_cfg_register(&_surf_cfg_set, "contexts/parallel_threshold",
        "Minimal number of user contexts to be run in parallel (raw contexts only)",
        xbt_cfgelm_int, &default_value_int, 1, 1,
        _surf_cfg_cb_contexts_parallel_threshold, NULL);

    /* synchronization mode for parallel user contexts */
#ifdef HAVE_FUTEX_H
    default_value = xbt_strdup("futex");
#else //No futex on mac and posix is unimplememted yet
    default_value = xbt_strdup("busy_wait");
#endif
    xbt_cfg_register(&_surf_cfg_set, "contexts/synchro",
        "Synchronization mode to use when running contexts in parallel (either futex, posix or busy_wait)",
        xbt_cfgelm_string, &default_value, 1, 1,
        _surf_cfg_cb_contexts_parallel_mode, NULL);

    /* number of parallel threads for Surf */
    default_value_int = surf_get_nthreads();
    xbt_cfg_register(&_surf_cfg_set, "surf/nthreads",
                     "Number of parallel threads used to update Surf models",
                     xbt_cfgelm_int, &default_value_int, 1, 1,
                     _surf_cfg_cb_surf_nthreads, NULL);

    default_value = xbt_strdup("no");
    xbt_cfg_register(&_surf_cfg_set, "network/coordinates",
                     "\"yes\" or \"no\", specifying whether we use a coordinate-based routing (as Vivaldi)",
                     xbt_cfgelm_string, &default_value, 1, 1,
                     _surf_cfg_cb__surf_network_coordinates, NULL);
    xbt_cfg_setdefault_string(_surf_cfg_set, "network/coordinates", default_value);

    default_value_int = 0;
    xbt_cfg_register(&_surf_cfg_set, "network/crosstraffic",
                     "Activate the interferences between uploads and downloads for fluid max-min models (LV08, CM02)",
                     xbt_cfgelm_int, &default_value_int, 0, 1,
                     _surf_cfg_cb__surf_network_crosstraffic, NULL);
    xbt_cfg_setdefault_int(_surf_cfg_set, "network/crosstraffic", default_value_int);

#ifdef HAVE_GTNETS
    xbt_cfg_register(&_surf_cfg_set, "gtnets/jitter",
                     "Double value to oscillate the link latency, uniformly in random interval [-latency*gtnets_jitter,latency*gtnets_jitter)",
                     xbt_cfgelm_double, NULL, 1, 1,
                     _surf_cfg_cb__gtnets_jitter, NULL);
    xbt_cfg_setdefault_double(_surf_cfg_set, "gtnets/jitter", 0.0);

    default_value_int = 10;
    xbt_cfg_register(&_surf_cfg_set, "gtnets/jitter_seed",
                     "Use a positive seed to reproduce jitted results, value must be in [1,1e8], default is 10",
                     xbt_cfgelm_int, &default_value_int, 0, 1,
                     _surf_cfg_cb__gtnets_jitter_seed, NULL);
#endif
#ifdef HAVE_NS3
    xbt_cfg_register(&_surf_cfg_set, "ns3/TcpModel",
                     "The ns3 tcp model can be : NewReno or Reno or Tahoe",
                     xbt_cfgelm_string, NULL, 1, 1,
                     NULL, NULL);
    xbt_cfg_setdefault_string(_surf_cfg_set, "ns3/TcpModel", "default");
#endif

//SMPI
    double default_reference_speed = 20000.0;
    xbt_cfg_register(&_surf_cfg_set, "smpi/running_power",
                     "Power of the host running the simulation (in flop/s). Used to bench the operations.",
                     xbt_cfgelm_double, &default_reference_speed, 1, 1, NULL,
                     NULL);

    int default_display_timing = 0;
    xbt_cfg_register(&_surf_cfg_set, "smpi/display_timing",
                     "Boolean indicating whether we should display the timing after simulation.",
                     xbt_cfgelm_int, &default_display_timing, 1, 1, NULL,
                     NULL);

    double default_threshold = 1e-6;
    xbt_cfg_register(&_surf_cfg_set, "smpi/cpu_threshold",
                     "Minimal computation time (in seconds) not discarded.",
                     xbt_cfgelm_double, &default_threshold, 1, 1, NULL,
                     NULL);

    //For smpi/bw_factor and smpi/lat_factor
    //Default value have to be "threshold0:value0;threshold1:value1;...;thresholdN:valueN"
    //test is if( size >= thresholdN ) return valueN;
    //Values can be modified with command line --cfg=smpi/bw_factor:"threshold0:value0;threshold1:value1;...;thresholdN:valueN"
    //	or with tag config put line <prop id="smpi/bw_factor" value="threshold0:value0;threshold1:value1;...;thresholdN:valueN"></prop>
    xbt_cfg_register(&_surf_cfg_set, "smpi/bw_factor",
                     "Bandwidth factors for smpi.",
                     xbt_cfgelm_string, NULL, 1, 1, NULL,
                     NULL);
    xbt_cfg_setdefault_string(_surf_cfg_set, "smpi/bw_factor", "65472:0.940694;15424:0.697866;9376:0.58729;5776:1.08739;3484:0.77493;1426:0.608902;732:0.341987;257:0.338112;0:0.812084");

    xbt_cfg_register(&_surf_cfg_set, "smpi/lat_factor",
                     "Latency factors for smpi.",
                     xbt_cfgelm_string, NULL, 1, 1, NULL,
                     NULL);
    xbt_cfg_setdefault_string(_surf_cfg_set, "smpi/lat_factor", "65472:11.6436;15424:3.48845;9376:2.59299;5776:2.18796;3484:1.88101;1426:1.61075;732:1.9503;257:1.95341;0:2.01467");
//END SMPI


    if (!surf_path) {
      /* retrieves the current directory of the        current process */
      const char *initial_path = __surf_get_initial_path();
      xbt_assert((initial_path),
                  "__surf_get_initial_path() failed! Can't resolves current Windows directory");

      surf_path = xbt_dynar_new(sizeof(char *), NULL);
      xbt_cfg_setdefault_string(_surf_cfg_set, "path", initial_path);
    }


    surf_config_cmd_line(argc, argv);
  } else {
    XBT_WARN("Call to surf_config_init() after initialization ignored");
  }
}

void surf_config_finalize(void)
{
  if (!_surf_init_status)
    return;                     /* Not initialized yet. Nothing to do */

  xbt_cfg_free(&_surf_cfg_set);
  _surf_init_status = 0;
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
      xbt_cfg_get_string(_surf_cfg_set, "workstation/model");
  network_model_name = xbt_cfg_get_string(_surf_cfg_set, "network/model");
  cpu_model_name = xbt_cfg_get_string(_surf_cfg_set, "cpu/model");
  storage_model_name = xbt_cfg_get_string(_surf_cfg_set, "storage/model");

  /* Check whether we use a net/cpu model differing from the default ones, in which case
   * we should switch to the "compound" workstation model to correctly dispatch stuff to
   * the right net/cpu models.
   */

  if((!xbt_cfg_is_default_value(_surf_cfg_set, "network/model") ||
	  !xbt_cfg_is_default_value(_surf_cfg_set, "cpu/model")) &&
	  xbt_cfg_is_default_value(_surf_cfg_set, "workstation/model"))
  {
	    const char *val = "compound";
	    XBT_INFO
	        ("Switching workstation model to compound since you changed the network and/or cpu model(s)");
	    xbt_cfg_set_string(_surf_cfg_set, "workstation/model", val);
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
}
