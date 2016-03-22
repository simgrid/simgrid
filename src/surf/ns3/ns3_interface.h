/* Copyright (c) 2007-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _NS3_INTERFACE_H
#define _NS3_INTERFACE_H

#include "xbt/log.h"
#include "xbt/dynar.h"
#include "xbt/misc.h"
#include "xbt/sysdep.h"
#include <xbt/Extendable.hpp>

#include <simgrid/s4u/host.hpp>
#include <surf/surf_routing.h>

namespace simgrid{
  namespace surf{
    class NetworkNS3Action;
  }
}
typedef enum {
  NS3_NETWORK_ELEMENT_NULL = 0,    /* NULL */
  NS3_NETWORK_ELEMENT_HOST,      /* host type */
  NS3_NETWORK_ELEMENT_ROUTER,     /* router type */
  NS3_NETWORK_ELEMENT_AS,        /* AS type */
} e_ns3_network_element_type_t;


typedef struct ns3_node {
  int node_num;
  e_ns3_network_element_type_t type;
} s_ns3_node_t, *ns3_node_t;

XBT_PUBLIC_DATA(int) NS3_EXTENSION_ID;

SG_BEGIN_DECL()

XBT_PUBLIC(int)    ns3_initialize(const char* TcpProtocol);
XBT_PUBLIC(int)    ns3_create_flow(const char* a,const char *b,double start,u_int32_t TotalBytes,simgrid::surf::NetworkNS3Action * action);
XBT_PUBLIC(void)   ns3_simulator(double min);
XBT_PUBLIC(void *) ns3_add_host_cluster(const char * id);
XBT_PUBLIC(void *) ns3_add_router(const char * id);
XBT_PUBLIC(void *) ns3_add_AS(const char * id);
XBT_PUBLIC(void) ns3_add_link(int src, e_ns3_network_element_type_t type_src,
                int dst, e_ns3_network_element_type_t type_dst,
                char * bw,char * lat);
XBT_PUBLIC(void) ns3_end_platform(void);
XBT_PUBLIC(void) ns3_add_cluster(char * bw,char * lat,const char *id);

inline
ns3_node_t ns3_find_host(const char* id)
{
  sg_host_t host = sg_host_by_name(id);
  if (host == nullptr)
    return nullptr;
  else
    return (ns3_node_t) host->extension(NS3_EXTENSION_ID);
}

SG_END_DECL()

#endif
