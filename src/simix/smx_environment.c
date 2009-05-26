/* 	$Id$	 */

/* Copyright (c) 2007 Arnaud Legrand, Bruno Donnassolo.
   All rights reserved.                                          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/xbt_os_time.h"

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
  int workstation_id = -1;
  double start, end;

  simix_config_init();          /* make sure that our configuration set is created */
  surf_timer_model_init(file);

  /* which model do you want today? */
  workstation_model_name =
    xbt_cfg_get_string(_simix_cfg_set, "workstation_model");

  DEBUG1("Model : %s", workstation_model_name);
  workstation_id =
    find_model_description(surf_workstation_model_description,
                           workstation_model_name);
  if (!strcmp(workstation_model_name, "compound")) {
    xbt_ex_t e;
    char *network_model_name = NULL;
    char *cpu_model_name = NULL;
    int network_id = -1;
    int cpu_id = -1;

    TRY {
      cpu_model_name = xbt_cfg_get_string(_simix_cfg_set, "cpu_model");
    } CATCH(e) {
      if (e.category == bound_error) {
        xbt_assert0(0,
                    "Set a cpu model to use with the 'compound' workstation model");
        xbt_ex_free(e);
      } else {
        RETHROW;
      }
    }

    TRY {
      network_model_name =
        xbt_cfg_get_string(_simix_cfg_set, "network_model");
    }
    CATCH(e) {
      if (e.category == bound_error) {
        xbt_assert0(0,
                    "Set a network model to use with the 'compound' workstation model");
        xbt_ex_free(e);
      } else {
        RETHROW;
      }
    }

    network_id =
      find_model_description(surf_network_model_description,
                             network_model_name);
    cpu_id =
      find_model_description(surf_cpu_model_description, cpu_model_name);

    surf_cpu_model_description[cpu_id].model_init(file);
    surf_network_model_description[network_id].model_init(file);


  }
  surf_workstation_model_description[workstation_id].model_init(file);

  start = xbt_os_time();
  parse_platform_file(file);

  if (surf_workstation_model_description[workstation_id].create_ws != NULL)
    surf_workstation_model_description[workstation_id].create_ws();
  end = xbt_os_time();
  DEBUG1("PARSE TIME: %lg", (end - start));

  _simix_init_status = 2;       /* inited; don't change settings now */

  xbt_dict_foreach(workstation_set, cursor, name, workstation) {
    __SIMIX_host_create(name, workstation, NULL);
  }

  return;
}
