/* 	$Id$	 */

/* Copyright (c) 2002,2003,2004 Arnaud Legrand. All rights reserved.        */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include"private.h"
#include"xbt/sysdep.h"
#include "xbt/error.h"
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(environment, msg,
				"Logging specific to MSG (environment)");

/********************************* MSG **************************************/

/** \ingroup msg_easier_life
 * \brief A name directory service...
 *
 * Finds a m_host_t using its name.
 * \param name the name of an host.
 * \return the corresponding host
 */
m_host_t MSG_get_host_by_name(const char *name)
{
  xbt_fifo_item_t i = NULL;
  m_host_t host = NULL;

  xbt_assert0(((msg_global != NULL)
	  && (msg_global->host != NULL)), "Environment not set yet");

  xbt_fifo_foreach(msg_global->host,i,host,m_host_t) {
    if(strcmp(host->name, name) == 0) return host;
  }
  return NULL;
}

/** \ingroup msg_easier_life
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
 * Have a look in the directory examples/msg/ to have a big example.
 */
void MSG_create_environment(const char *file) {
  xbt_dict_cursor_t cursor = NULL;
  char *name = NULL;
  void *workstation = NULL;

  surf_workstation_resource_init_CLM03(file);
/*   surf_workstation_resource_init_KCCFLN05(file); */

  xbt_dict_foreach(workstation_set, cursor, name, workstation) {
    __MSG_host_create(name, workstation, NULL);
  }

  return;
}

