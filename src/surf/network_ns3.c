/* Copyright (c) 2007, 2008, 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_private.h"
#include "surf/ns3/ns3_interface.h"
#include "xbt/lib.h"

extern xbt_lib_t host_lib;
extern xbt_lib_t link_lib;
extern xbt_lib_t as_router_lib;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_network_ns3, surf,
                                "Logging specific to the SURF network NS3 module");

extern routing_global_t global_routing;

void parse_ns3_add_host(void)
{
	XBT_DEBUG("NS3_ADD_HOST '%s'",A_surfxml_host_id);
	xbt_lib_set(host_lib,
				A_surfxml_host_id,
				NS3_HOST_LEVEL,
				ns3_add_host(A_surfxml_host_id)
				);
}
void parse_ns3_add_link(void)
{
	XBT_DEBUG("NS3_ADD_LINK '%s'",A_surfxml_link_id);
	xbt_lib_set(link_lib,
				A_surfxml_link_id,
				NS3_LINK_LEVEL,
				ns3_add_link(A_surfxml_link_id)
				);
}
void parse_ns3_add_router(void)
{
	XBT_DEBUG("NS3_ADD_ROUTER '%s'",A_surfxml_router_id);
	xbt_lib_set(as_router_lib,
				A_surfxml_router_id,
				NS3_ASR_LEVEL,
				ns3_add_router(A_surfxml_router_id)
				);
}
void parse_ns3_add_AS(void)
{
	XBT_DEBUG("NS3_ADD_AS '%s'",A_surfxml_AS_id);
	xbt_lib_set(as_router_lib,
				A_surfxml_AS_id,
				NS3_ASR_LEVEL,
				ns3_add_AS(A_surfxml_AS_id)
				);
}
void parse_ns3_add_route(void)
{
	XBT_DEBUG("NS3_ADD_ROUTE from '%s' to '%s'",A_surfxml_route_src,A_surfxml_route_dst);
	ns3_add_route(A_surfxml_route_src,A_surfxml_route_dst);
}
void parse_ns3_add_ASroute(void)
{
	XBT_DEBUG("NS3_ADD_ASROUTE from '%s' to '%s'",A_surfxml_ASroute_src,A_surfxml_ASroute_dst);
	ns3_add_ASroute(A_surfxml_ASroute_src,A_surfxml_ASroute_dst);
}
void parse_ns3_add_cluster(void)
{
	XBT_DEBUG("NS3_ADD_CLUSTER '%s'",A_surfxml_cluster_id);
	routing_parse_Scluster();
}

void parse_ns3_end_platform(void)
{
	  xbt_lib_cursor_t cursor = NULL;
	  char *name = NULL;
	  void **data = NULL;
	  XBT_INFO("link_lib");
	  xbt_lib_foreach(link_lib, cursor, name, data) {
			XBT_INFO("\tSee link '%s'\t--> NS3_LEVEL %p",
					name,
					data[NS3_LINK_LEVEL]);
	  }
	  XBT_INFO(" ");
	  XBT_INFO("host_lib");
	  xbt_lib_foreach(host_lib, cursor, name, data) {
			XBT_INFO("\tSee host '%s'\t--> NS3_LEVEL %p",
					name,
					data[NS3_HOST_LEVEL]);
	  }
	  XBT_INFO(" ");
	  XBT_INFO("as_router_lib");
	  xbt_lib_foreach(as_router_lib, cursor, name, data) {
			XBT_INFO("\tSee ASR '%s'\t--> NS3_LEVEL %p",
					name,
					data[NS3_ASR_LEVEL]);
	  }
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

  surfxml_add_callback(ETag_surfxml_platform_cb_list, &parse_ns3_end_platform); //DEBUG
}

void surf_network_model_init_NS3(const char *filename)
{
	define_callbacks_ns3(filename);
	surf_network_model = surf_model_init();
	surf_network_model->name = "network NS3";

	NS3_HOST_LEVEL = xbt_lib_add_level(host_lib,(void_f_pvoid_t)free_ns3_elmts);
	NS3_ASR_LEVEL  = xbt_lib_add_level(as_router_lib,(void_f_pvoid_t)free_ns3_elmts);
	NS3_LINK_LEVEL = xbt_lib_add_level(link_lib,(void_f_pvoid_t)free_ns3_elmts);

	update_model_description(surf_network_model_description,
	            "NS3", surf_network_model);
}
