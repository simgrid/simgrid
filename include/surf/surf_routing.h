/* Copyright (c) 2004-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_SURF_ROUTING_H
#define _SURF_SURF_ROUTING_H

#include "xbt/lib.h"

SG_BEGIN_DECL()

XBT_PUBLIC_DATA(xbt_dict_t) host_list;
XBT_PUBLIC_DATA(int) SIMIX_STORAGE_LEVEL; //Simix storage level
XBT_PUBLIC_DATA(int) COORD_HOST_LEVEL;    //Coordinates level

XBT_PUBLIC_DATA(xbt_lib_t) as_router_lib;
XBT_PUBLIC_DATA(int) ROUTING_ASR_LEVEL;  //Routing level
XBT_PUBLIC_DATA(int) COORD_ASR_LEVEL;  //Coordinates level
XBT_PUBLIC_DATA(int) NS3_ASR_LEVEL;    //host node for ns3
XBT_PUBLIC_DATA(int) ROUTING_PROP_ASR_LEVEL; //Properties for AS and router

XBT_PUBLIC_DATA(xbt_lib_t) storage_lib;
XBT_PUBLIC_DATA(int) ROUTING_STORAGE_LEVEL;        //Routing storage level
XBT_PUBLIC_DATA(int) ROUTING_STORAGE_HOST_LEVEL;
XBT_PUBLIC_DATA(int) SURF_STORAGE_LEVEL;  // Surf storage level
XBT_PUBLIC_DATA(xbt_lib_t) file_lib;
XBT_PUBLIC_DATA(xbt_lib_t) storage_type_lib;
XBT_PUBLIC_DATA(int) ROUTING_STORAGE_TYPE_LEVEL;   //Routing storage_type level

SG_END_DECL()

#endif                          /* _SURF_SURF_H */
