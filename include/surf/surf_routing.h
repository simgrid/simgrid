/* Copyright (c) 2004-2013. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_SURF_ROUTING_H
#define _SURF_SURF_ROUTING_H

#include "xbt/lib.h"
#include "simgrid/platf_interface.h"

XBT_PUBLIC(xbt_lib_t) host_lib;
XBT_PUBLIC(int) ROUTING_HOST_LEVEL; //Routing level
XBT_PUBLIC(int)  SURF_CPU_LEVEL;    //Surf cpu level
XBT_PUBLIC(int) SURF_WKS_LEVEL;    //Surf workstation level
XBT_PUBLIC(int) SIMIX_HOST_LEVEL;  //Simix host level
XBT_PUBLIC(int) SIMIX_STORAGE_LEVEL;  //Simix storage level
XBT_PUBLIC(int)  MSG_HOST_LEVEL;    //Msg level
XBT_PUBLIC(int)  SD_HOST_LEVEL;    //Simdag host level
XBT_PUBLIC(int)  SD_STORAGE_LEVEL;    //Simdag storage level
XBT_PUBLIC(int)  COORD_HOST_LEVEL;  //Coordinates level
XBT_PUBLIC(int) NS3_HOST_LEVEL;    //host node for ns3

XBT_PUBLIC(xbt_lib_t) link_lib;
XBT_PUBLIC(int) SD_LINK_LEVEL;    //Simdag level
XBT_PUBLIC(int) SURF_LINK_LEVEL;  //Surf level
XBT_PUBLIC(int) NS3_LINK_LEVEL;    //link for ns3

XBT_PUBLIC(xbt_lib_t) as_router_lib;
XBT_PUBLIC(int) ROUTING_ASR_LEVEL;  //Routing level
XBT_PUBLIC(int) COORD_ASR_LEVEL;  //Coordinates level
XBT_PUBLIC(int) NS3_ASR_LEVEL;    //host node for ns3
XBT_PUBLIC(int) ROUTING_PROP_ASR_LEVEL; //Properties for AS and router

XBT_PUBLIC(xbt_lib_t) storage_lib;
XBT_PUBLIC(int) ROUTING_STORAGE_LEVEL;        //Routing storage level
XBT_PUBLIC(int) ROUTING_STORAGE_HOST_LEVEL;
XBT_PUBLIC(int) SURF_STORAGE_LEVEL;  // Surf storage level

XBT_PUBLIC(xbt_lib_t) storage_type_lib;
XBT_PUBLIC(int) ROUTING_STORAGE_TYPE_LEVEL;   //Routing storage_type level

/* The callbacks to register for the routing to work */
void routing_AS_begin(sg_platf_AS_cbarg_t AS);
void routing_AS_end(sg_platf_AS_cbarg_t AS);

void routing_cluster_add_backbone(void* bb);

#endif                          /* _SURF_SURF_H */
