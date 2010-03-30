/* Copyright (c) 2009-2010 The SimGrid team. All rights reserved.           */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* surf_config: configuration infrastructure for the simulation world       */

#include "xbt/config.h"
#include "xbt/str.h"
#include "surf/surf_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_config, surf,
                                "About the configuration of surf (and the rest of the simulation)");

xbt_cfg_t _surf_cfg_set = NULL;


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
      DEBUG1("Did apply '%s' as config setting", opt);
      remove_it = 1;
    } else if (!strncmp(argv[i], "--cfg-help", strlen("--cfg-help") + 1) ||
               !strncmp(argv[i], "--help", strlen("--help") + 1)) {
      printf
        ("Description of the configuration accepted by this simulator:\n");
      xbt_cfg_help(_surf_cfg_set);
      remove_it = 1;
      exit(0);
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

  xbt_assert0(_surf_init_status < 2,
              "Cannot change the model after the initialization");

  val = xbt_cfg_get_string(_surf_cfg_set, name);
  /* New Module missing */

  find_model_description(surf_workstation_model_description, val);
}

/* callback of the cpu/model variable */
static void _surf_cfg_cb__cpu_model(const char *name, int pos)
{
  char *val;

  xbt_assert0(_surf_init_status < 2,
              "Cannot change the model after the initialization");

  val = xbt_cfg_get_string(_surf_cfg_set, name);
  /* New Module missing */
  find_model_description(surf_cpu_model_description, val);
}

/* callback of the workstation_model variable */
static void _surf_cfg_cb__network_model(const char *name, int pos)
{
  char *val;

  xbt_assert0(_surf_init_status < 2,
              "Cannot change the model after the initialization");

  val = xbt_cfg_get_string(_surf_cfg_set, name);
  /* New Module missing */
  find_model_description(surf_network_model_description, val);
}


/* callbacks of the network models values */
static void _surf_cfg_cb__tcp_gamma(const char *name, int pos) {
  sg_tcp_gamma = xbt_cfg_get_double(_surf_cfg_set, name);
}
static void _surf_cfg_cb__latency_factor(const char *name, int pos) {
  sg_latency_factor = xbt_cfg_get_double(_surf_cfg_set, name);
}
static void _surf_cfg_cb__bandwidth_factor(const char *name, int pos) {
  sg_bandwidth_factor = xbt_cfg_get_double(_surf_cfg_set, name);
}
static void _surf_cfg_cb__weight_S(const char *name, int pos) {
  sg_weight_S_parameter = xbt_cfg_get_double(_surf_cfg_set, name);
}
static void _surf_cfg_cb__surf_maxmin_selective_update(const char *name, int pos) {
	sg_maxmin_selective_update = xbt_cfg_get_int(_surf_cfg_set, name);
}

/* callback of the inclusion path */
static void _surf_cfg_cb__surf_path(const char *name, int pos) {
  char *path = xbt_cfg_get_string_at(_surf_cfg_set, name, pos);
  xbt_dynar_push(surf_path, &path);
}


#ifdef HAVE_GTNETS
static void _surf_cfg_cb__gtnets_jitter(const char *name, int pos){
	sg_gtnets_jitter = xbt_cfg_get_double(_surf_cfg_set, name);
}
static void _surf_cfg_cb__gtnets_jitter_seed(const char *name, int pos){
	sg_gtnets_jitter_seed = xbt_cfg_get_int(_surf_cfg_set, name);
}
#endif

/* create the config set, register what should be and parse the command line*/
void surf_config_init(int *argc, char **argv)
{

  /* Create the configuration support */
  if (_surf_init_status == 0) { /* Only create stuff if not already inited */
    _surf_init_status = 1;

    char *description = xbt_malloc(1024), *p = description;
    char *default_value;
    double double_default_value;
	int default_value_int;
    int i;

    sprintf(description, "The model to use for the CPU. Possible values: ");
    p = description;
    while (*(++p) != '\0');
    for (i = 0; surf_cpu_model_description[i].name; i++)
      p +=
        sprintf(p, "%s%s", (i == 0 ? "" : ", "),
                surf_cpu_model_description[i].name);
    default_value = xbt_strdup("Cas01");
    xbt_cfg_register(&_surf_cfg_set,
                     "cpu/model", description, xbt_cfgelm_string,
                     &default_value, 1, 1, &_surf_cfg_cb__cpu_model, NULL);

    sprintf(description,
            "The model to use for the network. Possible values: ");
    p = description;
    while (*(++p) != '\0');
    for (i = 0; surf_network_model_description[i].name; i++)
      p +=
        sprintf(p, "%s%s", (i == 0 ? "" : ", "),
                surf_network_model_description[i].name);
    default_value = xbt_strdup("LV08");
    xbt_cfg_register(&_surf_cfg_set,
                     "network/model", description, xbt_cfgelm_string,
                     &default_value, 1, 1, &_surf_cfg_cb__network_model,
                     NULL);

    sprintf(description,
            "The model to use for the workstation. Possible values: ");
    p = description;
    while (*(++p) != '\0');
    for (i = 0; surf_workstation_model_description[i].name; i++)
      p +=
        sprintf(p, "%s%s", (i == 0 ? "" : ", "),
                surf_workstation_model_description[i].name);
    default_value = xbt_strdup("CLM03");
    xbt_cfg_register(&_surf_cfg_set,
                     "workstation/model", description, xbt_cfgelm_string,
                     &default_value, 1, 1, &_surf_cfg_cb__workstation_model,
                     NULL);

    xbt_free(description);

    default_value = xbt_strdup("Full");
    xbt_cfg_register(&_surf_cfg_set, "routing",
                     "Model to use to store the routing information",
                     xbt_cfgelm_string, &default_value, 1, 1, NULL, NULL);

    xbt_cfg_register(&_surf_cfg_set, "TCP_gamma",
                     "Size of the biggest TCP window (cat /proc/sys/net/ipv4/tcp_[rw]mem for recv/send window; middle value=default value => you probably want default send value)",
                     xbt_cfgelm_double, NULL, 1, 1, _surf_cfg_cb__tcp_gamma, NULL);
    xbt_cfg_set_double(_surf_cfg_set, "TCP_gamma", 20000.0);

    /* The parameters of network models */
    double_default_value = 1.0;
    xbt_cfg_register(&_surf_cfg_set, "network/latency_factor",
                     "Correction factor to apply to the provided latency (default value set by network model)",
                     xbt_cfgelm_double, &double_default_value, 1, 1, _surf_cfg_cb__latency_factor, NULL);
    double_default_value = 1.0;
    xbt_cfg_register(&_surf_cfg_set, "network/bandwidth_factor",
                     "Correction factor to apply to the provided bandwidth (default value set by network model)",
                     xbt_cfgelm_double, &double_default_value, 1, 1, _surf_cfg_cb__bandwidth_factor, NULL);
    double_default_value = 0.0;
    xbt_cfg_register(&_surf_cfg_set, "network/weight_S",
                     "Correction factor to apply to the weight of competing streams(default value set by network model)",
                     xbt_cfgelm_double, &double_default_value, 1, 1, _surf_cfg_cb__weight_S, NULL);

    /* Inclusion path */
    xbt_cfg_register(&_surf_cfg_set, "path",
                     "Lookup path for inclusions in platform and deployment XML files",
                     xbt_cfgelm_string, NULL, 0, 0, _surf_cfg_cb__surf_path,
                     NULL);

		default_value_int = 0;
    xbt_cfg_register(&_surf_cfg_set, "maxmin_selective_update",
                     "Update the constraint set propagating recursively to others constraints",
                     xbt_cfgelm_int, &default_value_int, 0, 1, _surf_cfg_cb__surf_maxmin_selective_update, NULL);

#ifdef HAVE_GTNETS
    xbt_cfg_register(&_surf_cfg_set, "gtnets_jitter",
                     "Double value to oscillate the link latency, uniformly in random interval [-latency*gtnets_jitter,latency*gtnets_jitter)", xbt_cfgelm_double,
                     NULL, 1, 1, _surf_cfg_cb__gtnets_jitter, NULL);
    xbt_cfg_set_double(_surf_cfg_set, "gtnets_jitter", 0.0);

    default_value_int = 10;
    xbt_cfg_register(&_surf_cfg_set, "gtnets_jitter_seed",
                     "Use a positive seed to reproduce jitted results, value must be in [1,1e8], default is 10",
                     xbt_cfgelm_int, &default_value_int, 0, 1, _surf_cfg_cb__gtnets_jitter_seed, NULL);
#endif

    if (!surf_path) {
      /* retrieves the current directory of the        current process */
      const char *initial_path = __surf_get_initial_path();
      xbt_assert0((initial_path),
                  "__surf_get_initial_path() failed! Can't resolves current Windows directory");

      surf_path = xbt_dynar_new(sizeof(char *), NULL);
      xbt_cfg_set_string(_surf_cfg_set, "path", initial_path);
    }


    surf_config_cmd_line(argc, argv);
  }
}

void surf_config_finalize(void)
{
  if (!_surf_init_status)
    return;                     /* Not initialized yet. Nothing to do */

  xbt_cfg_free(&_surf_cfg_set);
  _surf_init_status = 0;
}

void surf_config_models_setup(const char *platform_file)
{
  char *workstation_model_name;
  int workstation_id = -1;

  surf_timer_model_init(platform_file);

  workstation_model_name =
    xbt_cfg_get_string(_surf_cfg_set, "workstation/model");
  char *network_model_name = xbt_cfg_get_string(_surf_cfg_set, "network/model");
  char *cpu_model_name = xbt_cfg_get_string(_surf_cfg_set, "cpu/model");

  if ((strcmp(network_model_name,"LV08") || strcmp(cpu_model_name,"Cas01"))
      && !strcmp(workstation_model_name, "CLM03")){
    const char *val = "compound";
    INFO0("Switching workstation model to compound since you changed the network and/or cpu model(s)");
    xbt_cfg_set_string(_surf_cfg_set,"workstation/model",val);
    workstation_model_name = (char*)"compound";
  }

  DEBUG1("Workstation model: %s", workstation_model_name);
  workstation_id =
    find_model_description(surf_workstation_model_description,
                           workstation_model_name);
  if (!strcmp(workstation_model_name, "compound")) {
    int network_id = -1;
    int cpu_id = -1;

    xbt_assert0(cpu_model_name,
        "Set a cpu model to use with the 'compound' workstation model");

    xbt_assert0(network_model_name,
        "Set a network model to use with the 'compound' workstation model");

    network_id =
      find_model_description(surf_network_model_description,
                             network_model_name);
    cpu_id =
      find_model_description(surf_cpu_model_description, cpu_model_name);

    surf_cpu_model_description[cpu_id].model_init_preparse(platform_file);
    surf_network_model_description[network_id].
      model_init_preparse(platform_file);
  }

  DEBUG0("Call workstation_model_init");
  surf_workstation_model_description[workstation_id].model_init_preparse
    (platform_file);
}

void surf_config_models_create_elms(void)
{
  char *workstation_model_name =
    xbt_cfg_get_string(_surf_cfg_set, "workstation/model");
  int workstation_id =
    find_model_description(surf_workstation_model_description,
                           workstation_model_name);
  if (surf_workstation_model_description[workstation_id].
      model_init_postparse != NULL)
    surf_workstation_model_description[workstation_id].model_init_postparse();
}
