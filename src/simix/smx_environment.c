/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smx_private.h"
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
  double start = 0, end = 0;
  if(XBT_LOG_ISENABLED(simix_environment, xbt_log_priority_debug))
    start = xbt_os_time();
  parse_platform_file(file);
  if(XBT_LOG_ISENABLED(simix_environment, xbt_log_priority_debug))
    end = xbt_os_time();
  XBT_DEBUG("PARSE TIME: %g", (end - start));

}

void SIMIX_post_create_environment(void)
{
  void **storage = NULL;
  xbt_lib_cursor_t cursor = NULL;
  char *name = NULL;

  /* Create storage at SIMIX level */
  xbt_lib_foreach(storage_lib, cursor, name, storage) {
    if(storage[SURF_STORAGE_LEVEL])
      SIMIX_storage_create(name, storage[SURF_STORAGE_LEVEL], NULL);
  }

  surf_presolve();
}
