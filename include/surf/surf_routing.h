/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_SURF_ROUTING_H
#define _SURF_SURF_ROUTING_H

#include "xbt/lib.h"

extern xbt_lib_t host_lib;
extern int ROUTING_HOST_LEVEL; //Routing level
extern int	SURF_CPU_LEVEL;		//Surf cpu level
extern int SURF_WKS_LEVEL;		//Surf workstation level
extern int SIMIX_HOST_LEVEL;	//Simix level
extern int	MSG_HOST_LEVEL;		//Msg level
extern int	SD_HOST_LEVEL;		//Simdag level
extern int	COORD_HOST_LEVEL;	//Coordinates level
extern int NS3_HOST_LEVEL;		//host node for ns3

extern xbt_lib_t link_lib;
extern int SD_LINK_LEVEL;		//Simdag level
extern int SURF_LINK_LEVEL;	//Surf level
extern int NS3_LINK_LEVEL;		//link for ns3

extern xbt_lib_t as_router_lib;
extern int ROUTING_ASR_LEVEL;	//Routing level
extern int COORD_ASR_LEVEL;	//Coordinates level
extern int NS3_ASR_LEVEL;		//host node for ns3


/* The callbacks to register for the routing to work */
void routing_AS_begin(const char *AS_id, const char *wanted_routing_type);
void routing_AS_end(void);

#endif                          /* _SURF_SURF_H */
