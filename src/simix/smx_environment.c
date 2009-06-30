/* 	$Id$	 */

/* Copyright (c) 2007 Arnaud Legrand, Bruno Donnassolo.
   All rights reserved.                                          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/xbt_os_time.h"
#include "xbt/config.h"

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

  double start, end;

  surf_timer_model_init(file);
  surf_config_models_setup(file);
  parse_platform_file(file);
  surf_config_models_create_elms();
  start = xbt_os_time();

  end = xbt_os_time();
  DEBUG1("PARSE TIME: %lg", (end - start));

  xbt_dict_foreach(surf_model_resource_set(surf_workstation_model), cursor,
                   name, workstation) {
    __SIMIX_host_create(name, workstation, NULL);
  }

  return;
}
