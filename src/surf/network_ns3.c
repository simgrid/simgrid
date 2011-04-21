/* Copyright (c) 2007, 2008, 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_private.h"
#include "surf/ns3/ns3_interface.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_network_ns3, surf,
                                "Logging specific to the SURF network NS3 module");

extern routing_global_t global_routing;

void parse_ns3_add_host(void)
{
	XBT_INFO("NS3_ADD_HOST '%s'",A_surfxml_host_id);
	//ns3_add_host(A_surfxml_host_id);
}
void parse_ns3_add_router(void)
{
	XBT_INFO("NS3_ADD_ROUTER '%s'",A_surfxml_router_id);
}
void parse_ns3_add_link(void)
{
	XBT_INFO("NS3_ADD_LINK '%s'",A_surfxml_link_id);
}
void parse_ns3_add_AS(void)
{
	XBT_INFO("NS3_ADD_AS '%s'",A_surfxml_AS_id);
}
void parse_ns3_add_route(void)
{
	XBT_INFO("NS3_ADD_ROUTE from '%s' to '%s'",A_surfxml_route_src,A_surfxml_route_dst);
}
void parse_ns3_add_ASroute(void)
{
	XBT_INFO("NS3_ADD_ASROUTE from '%s' to '%s'",A_surfxml_ASroute_src,A_surfxml_ASroute_dst);
}
void parse_ns3_add_cluster(void)
{
	XBT_INFO("NS3_ADD_CLUSTER '%s'",A_surfxml_cluster_id);
	routing_parse_Scluster();
}

static void define_callbacks_ns3(const char *filename)
{
  surfxml_add_callback(STag_surfxml_host_cb_list, &parse_ns3_add_host);	//HOST
  surfxml_add_callback(STag_surfxml_router_cb_list, &parse_ns3_add_router);	//ROUTER
  surfxml_add_callback(STag_surfxml_link_cb_list, &parse_ns3_add_link);	//LINK
  surfxml_add_callback(STag_surfxml_AS_cb_list, &parse_ns3_add_AS);		//AS
  surfxml_add_callback(STag_surfxml_route_cb_list, &parse_ns3_add_route);	//ROUTE
  surfxml_add_callback(STag_surfxml_ASroute_cb_list, &parse_ns3_add_ASroute);	//ASROUTE
  surfxml_add_callback(STag_surfxml_cluster_cb_list, &parse_ns3_add_cluster); //CLUSTER
}

void surf_network_model_init_NS3(const char *filename)
{
	define_callbacks_ns3(filename);
	surf_network_model = surf_model_init();
	surf_network_model->name = "network NS3";

	update_model_description(surf_network_model_description,
	            "NS3", surf_network_model);
}
