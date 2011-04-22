/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf/ns3/ns3_interface.h"
#include "ns3/core-module.h"
#include "ns3/simulator-module.h"
#include "ns3/node-module.h"
#include "ns3/helper-module.h"
#include "ns3/global-route-manager.h"

using namespace ns3;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(interface_ns3, surf,
                                "Logging specific to the SURF network NS3 module");

NodeContainer nodes;
int number_of_nodes = 0;

void * ns3_add_host(char * id)
{
	ns3_nodes_t host  = xbt_new0(s_ns3_nodes_t,1);
	XBT_INFO("Interface ns3 add host '%s'",id);
	Ptr<Node> node =  CreateObject<Node> (0);
	nodes.Add(node);

	host->node_num = number_of_nodes;
	host->type = NS3_NETWORK_ELEMENT_HOST;
	host->data = nodes.Get(number_of_nodes);
	XBT_INFO("node %p",host->data);
	number_of_nodes++;
	return host;
}

void * ns3_add_router(char * id)
{
	ns3_nodes_t router  = xbt_new0(s_ns3_nodes_t,1);
	XBT_INFO("Interface ns3 add router '%s'",id);
	Ptr<Node> node =  CreateObject<Node> (0);
	nodes.Add(node);
	router->node_num = number_of_nodes;
	router->type = NS3_NETWORK_ELEMENT_ROUTER;
	router->data = node;
	number_of_nodes++;
	return router;
}

void * ns3_add_link(char * id)
{
	XBT_INFO("Interface ns3 add link '%s'",id);
	PointToPointHelper pointToPoint_5Mbps;
	pointToPoint_5Mbps.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
	pointToPoint_5Mbps.SetChannelAttribute ("Delay", StringValue ("2ms"));
	return	NULL;//&pointToPoint_5Mbps;
}

void * ns3_add_AS(char * id)
{
	XBT_INFO("Interface ns3 add AS '%s'",id);
	return NULL;
}

void ns3_add_route(char * src,char * dst)
{
	XBT_INFO("Interface ns3 add route from '%s' to '%s'",src,dst);
}

void ns3_add_ASroute(char * src,char * dst)
{
	XBT_INFO("Interface ns3 add ASroute from '%s' to '%s'",src,dst);
}

void free_ns3_elmts(void * elmts)
{
	XBT_INFO("Free ns3 elmts");
}
