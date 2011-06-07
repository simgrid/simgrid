/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _NS3_INTERFACE_H
#define _NS3_INTERFACE_H

#include "xbt/log.h"
#include "xbt/dynar.h"
#include "xbt/misc.h"
#include "xbt/sysdep.h"

typedef enum {
	NS3_NETWORK_ELEMENT_NULL = 0,    /* NULL */
	NS3_NETWORK_ELEMENT_HOST,    	/* host type */
	NS3_NETWORK_ELEMENT_ROUTER,   	/* router type */
	NS3_NETWORK_ELEMENT_AS,      	/* AS type */
} e_ns3_network_element_type_t;

typedef struct ns3_nodes{
	int node_num;
	e_ns3_network_element_type_t type;
	void * data;
}s_ns3_nodes_t, *ns3_nodes_t;

#ifdef __cplusplus
extern "C" {
#endif

XBT_PUBLIC(void *) ns3_add_host(char * id);
XBT_PUBLIC(void *) ns3_add_router(char * id);
XBT_PUBLIC(void *) ns3_add_AS(char * id);
XBT_PUBLIC(void) ns3_add_cluster(char * id);
#ifdef __cplusplus
}
#endif

#endif
