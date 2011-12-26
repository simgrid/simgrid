/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/xbt_os_time.h"
#include "xbt/config.h"
#include "surf/surfxml_parse.h"

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
  xbt_lib_cursor_t cursor = NULL;
  char *name = NULL;
  void **workstation = NULL;

  double start, end;

  start = xbt_os_time();
  parse_platform_file(file);
  end = xbt_os_time();
  XBT_DEBUG("PARSE TIME: %lg", (end - start));

  xbt_lib_foreach(host_lib, cursor, name, workstation) {
	  if(workstation[SURF_WKS_LEVEL])
		  SIMIX_host_create(name, workstation[SURF_WKS_LEVEL], NULL);
  }
  surf_presolve();
}
