/* 	$Id$	 */

/* Copyright (c) 2007 Arnaud Legrand, Bruno Donnassolo.
   All rights reserved.                                          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_environment, simix,
				"Logging specific to SIMIX (environment)");

/********************************* SIMIX **************************************/

/** 
 * \brief A platform constructor.
 *
 * Creates a new platform, including hosts, links and the
 * routing_table. 
 * \param file a filename of a xml description of a platform. This file 
 * follows this DTD :
 *
 *     \include surfxml.dtd
 *
 * Here is a small example of such a platform 
 *
 *     \include small_platform.xml
 *
 */
void SIMIX_create_environment(const char *file) 
{
  xbt_dict_cursor_t cursor = NULL;
  char *name = NULL;
  void *workstation = NULL;
  char *workstation_model_name;

  simix_config_init(); /* make sure that our configuration set is created */
  surf_timer_resource_init(file);

  /* which model do you want today? */
  workstation_model_name = xbt_cfg_get_string (_simix_cfg_set, "workstation_model");

  DEBUG1("Model : %s", workstation_model_name);
  if (!strcmp(workstation_model_name,"KCCFLN05")) {
    surf_workstation_resource_init_KCCFLN05(file);
#ifdef HAVE_SDP
  } else if (!strcmp(workstation_model_name,"SDP")) {
    surf_workstation_resource_init_KCCFLN05_proportional(file);
#endif
  } else if (!strcmp(workstation_model_name,"Vegas")) {
    surf_workstation_resource_init_KCCFLN05_Vegas(file);
  } else if (!strcmp(workstation_model_name,"Reno")) {
    surf_workstation_resource_init_KCCFLN05_Reno(file);
  } else if (!strcmp(workstation_model_name,"CLM03")) {
    surf_workstation_resource_init_CLM03(file);
#ifdef HAVE_GTNETS
  } else if (!strcmp(workstation_model_name,"GTNets")) {
    surf_workstation_resource_init_GTNETS(file);
#endif
  } else if (!strcmp(workstation_model_name,"compound")) {
    char *network_model_name = xbt_cfg_get_string (_simix_cfg_set, "network_model");
    char *cpu_model_name = xbt_cfg_get_string (_simix_cfg_set, "cpu_model");
    
    if(!strcmp(cpu_model_name,"Cas01")) {
      surf_cpu_resource_init_Cas01(file);
    } else DIE_IMPOSSIBLE;

    if(!strcmp(network_model_name,"CM02")) {
      surf_network_resource_init_CM02(file);
#ifdef HAVE_GTNETS
    } else if(!strcmp(network_model_name,"GTNets")) {
      surf_network_resource_init_GTNETS(file);
#endif
    } else if(!strcmp(network_model_name,"Reno")) {
      surf_network_resource_init_Reno(file);
    } else if(!strcmp(network_model_name,"Vegas")) {
      surf_network_resource_init_Vegas(file);
#ifdef HAVE_SDP
    } else if(!strcmp(network_model_name,"SDP")) {
      surf_network_resource_init_SDP(file);
#endif
    } else
      DIE_IMPOSSIBLE;
    
    surf_workstation_resource_init_compound(file);
  } else {
    DIE_IMPOSSIBLE;
  }
  _simix_init_status = 2; /* inited; don't change settings now */

  xbt_dict_foreach(workstation_set, cursor, name, workstation) {
    __SIMIX_host_create(name, workstation, NULL);
  }

  return;
}

