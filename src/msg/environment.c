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
 * \param file a filename of a description of a platform. This file is a simple text 
 * file in which you can use C-style comments and C-style strings.
 * Here is a simple description of the format:
 \verbatim
<CPU>
host_name "power in MFlops" "availability" "availability file" ON/OFF "failure file"
host_name "power in MFlops" "availability" "availability file" ON/OFF "failure file"
...
</LINKS>
link_name "bandwidth in Mbytes" "bandwidth file" "latency in ms" "latency file"
link_name "bandwidth in Mbytes" "bandwidth file" "latency in ms" "latency file"
...
ROUTES
src_name dst_name (link_name link_name link_name ... )
src_name dst_name (link_name link_name link_name ... )
...
\endverbatim
 * Have a look in the directory examples/msg/ to have a better idea of what
 * it looks like.
 */
void MSG_create_environment(const char *file) {
  surf_workstation_resource_init(file);

  return;
}

