/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_NETWORK_NS3_PRIVATE_H
#define _SURF_NETWORK_NS3_PRIVATE_H

#include "surf_private.h"
#include "xbt/dict.h"

typedef struct ns3_link{
	char * id;
	char * lat;
	char * bdw;
}s_ns3_link_t, *ns3_link_t;

typedef struct surf_ns3_link{
	s_surf_resource_t generic_resource;
	ns3_link_t data;
	int created;
}s_surf_ns3_link_t, *surf_ns3_link_t;

typedef struct surf_action_network_ns3 {
  s_surf_action_t generic_action;
#ifdef HAVE_TRACING
  double last_sent;
  sg_routing_edge_t src_elm;
  sg_routing_edge_t dst_elm;
#endif //HAVE_TRACING
} s_surf_action_network_ns3_t, *surf_action_network_ns3_t;

#endif                          /* _SURF_NETWORK_NS3_PRIVATE_H */
