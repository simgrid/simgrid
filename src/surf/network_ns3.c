/* Copyright (c) 2007, 2008, 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_private.h"
#include "surf/ns3/ns3_interface.h"
#include "xbt/lib.h"
#include "surf/network_ns3_private.h"
#include "xbt/str.h"

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

	tmgr_trace_t bw_trace;
	tmgr_trace_t state_trace;
	tmgr_trace_t lat_trace;

	bw_trace = tmgr_trace_new(A_surfxml_link_bandwidth_file);
	lat_trace = tmgr_trace_new(A_surfxml_link_latency_file);
	state_trace = tmgr_trace_new(A_surfxml_link_state_file);

	if (bw_trace)
		XBT_INFO("The NS3 network model doesn't support bandwidth state traces");
	if (lat_trace)
		XBT_INFO("The NS3 network model doesn't support latency state traces");
	if (state_trace)
		XBT_INFO("The NS3 network model doesn't support link state traces");

	ns3_link_t link_ns3 = xbt_new0(s_ns3_link_t,1);;
	link_ns3->id = xbt_strdup(A_surfxml_link_id);
	link_ns3->bdw = xbt_strdup(A_surfxml_link_bandwidth);
	link_ns3->lat = xbt_strdup(A_surfxml_link_latency);

	surf_ns3_link_t link = xbt_new0(s_surf_ns3_link_t,1);
	link->generic_resource.name = xbt_strdup(A_surfxml_link_id);
	link->generic_resource.properties = current_property_set;
	link->data = link_ns3;

	xbt_lib_set(link_lib,A_surfxml_link_id,NS3_LINK_LEVEL,link_ns3);
	xbt_lib_set(link_lib,A_surfxml_link_id,SURF_LINK_LEVEL,link);
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
void parse_ns3_add_cluster(void)
{
	XBT_DEBUG("NS3_ADD_CLUSTER '%s'",A_surfxml_cluster_id);
	routing_parse_Scluster();
}

double ns3_get_link_latency (const void *link)
{
	double lat;
	XBT_INFO("link_id:%s link_lat:%s link_bdw:%s",((surf_ns3_link_t)link)->data->id,((surf_ns3_link_t)link)->data->lat,((surf_ns3_link_t)link)->data->bdw);
	sscanf(((surf_ns3_link_t)link)->data->lat,"%lg",&lat);
	return lat;
}
double ns3_get_link_bandwidth (const void *link)
{
	double bdw;
	XBT_INFO("link_id:%s link_lat:%s link_bdw:%s",((surf_ns3_link_t)link)->data->id,((surf_ns3_link_t)link)->data->lat,((surf_ns3_link_t)link)->data->bdw);
	sscanf(((surf_ns3_link_t)link)->data->bdw,"%lg",&bdw);
	return bdw;
}

static xbt_dynar_t ns3_get_route(const char *src, const char *dst)
{
  return global_routing->get_route(src, dst);
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

static void replace_str(char *str, const char *orig, const char *rep)
{
  char buffer[30];
  char *p;

  if(!(p = strstr(str, orig)))  // Is 'orig' even in 'str'?
    return;

  strncpy(buffer, str, p-str); // Copy characters from 'str' start to 'orig' st$
  buffer[p-str] = '\0';

  sprintf(buffer+(p-str), "%s%s", rep, p+strlen(orig));
  xbt_free(str);
  str = xbt_strdup(buffer);
}

static void replace_bdw_ns3(char * bdw)
{
	char *temp = xbt_strdup(bdw);

	replace_str(bdw,"000000000","Gbps");
	if(strcmp(bdw,temp)) {xbt_free(temp);return;}
	replace_str(bdw,"000000","Mbps");
	if(strcmp(bdw,temp)) {xbt_free(temp);return;}
	replace_str(bdw,"000","Kbps");
	if(strcmp(bdw,temp)) {xbt_free(temp);return;}

	xbt_free(bdw);
	bdw = bprintf("%s%s",temp,"bps");
	xbt_free(temp);
}

static void replace_lat_ns3(char * lat)
{
	char *temp = xbt_strdup(lat);

	replace_str(lat,"E-1","00ms");
	if(strcmp(lat,temp)) {xbt_free(temp);return;}
	replace_str(lat,"E-2","0ms");
	if(strcmp(lat,temp)) {xbt_free(temp);return;}
	replace_str(lat,"E-3","ms");
	if(strcmp(lat,temp)) {xbt_free(temp);return;}
	replace_str(lat,"E-4","00us");
	if(strcmp(lat,temp)) {xbt_free(temp);return;}
	replace_str(lat,"E-5","0us");
	if(strcmp(lat,temp)) {xbt_free(temp);return;}
	replace_str(lat,"E-6","us");
	if(strcmp(lat,temp)) {xbt_free(temp);return;}
	replace_str(lat,"E-7","00ns");
	if(strcmp(lat,temp)) {xbt_free(temp);return;}
	replace_str(lat,"E-8","0ns");
	if(strcmp(lat,temp)) {xbt_free(temp);return;}
	replace_str(lat,"E-9","ns");
	if(strcmp(lat,temp)) {xbt_free(temp);return;}

	xbt_free(lat);
	lat = bprintf("%s%s",temp,"s");
	xbt_free(temp);
}

/* Create the ns3 topology based on routing strategy */
void create_ns3_topology()
{
   int src_id,dst_id;

   XBT_DEBUG("Starting topology generation");

   //get the onelinks from the parsed platform
   xbt_dynar_t onelink_routes = global_routing->get_onelink_routes();
   if (!onelink_routes)
     xbt_die("There is no routes!");

   //save them in trace file
   onelink_t onelink;
   unsigned int iter;
   xbt_dynar_foreach(onelink_routes, iter, onelink) {
     char *src = onelink->src;
     char *dst = onelink->dst;
     void *link = onelink->link_ptr;
     src_id = *((int *) xbt_dict_get_or_null(global_routing->root->to_index,src));
     dst_id = *((int *) xbt_dict_get_or_null(global_routing->root->to_index,dst));

   if(src_id != dst_id){
	 char * link_bdw = xbt_strdup(((surf_ns3_link_t)link)->data->bdw);
	 char * link_lat = xbt_strdup(((surf_ns3_link_t)link)->data->lat);
	 //     replace_bdw_ns3(link_bdw);
	 //     replace_lat_ns3(link_lat);
	 XBT_INFO("src (%s), dst (%s), src_id = %d, dst_id = %d",src,dst, src_id, dst_id);
     XBT_INFO("\tLink (%s) bdw:%s->%s lat:%s->%s",((surf_ns3_link_t)link)->data->id,
    		 ((surf_ns3_link_t)link)->data->bdw,link_bdw,
    		 ((surf_ns3_link_t)link)->data->lat,link_lat
    		 );
     xbt_free(link_bdw);
     xbt_free(link_lat);
     }
   }
}

static void define_callbacks_ns3(const char *filename)
{
  surfxml_add_callback(STag_surfxml_host_cb_list, &parse_ns3_add_host);	//HOST
  surfxml_add_callback(STag_surfxml_router_cb_list, &parse_ns3_add_router);	//ROUTER
  surfxml_add_callback(STag_surfxml_link_cb_list, &parse_ns3_add_link);	//LINK
  surfxml_add_callback(STag_surfxml_AS_cb_list, &parse_ns3_add_AS);		//AS
  //surfxml_add_callback(STag_surfxml_cluster_cb_list, &parse_ns3_add_cluster); //CLUSTER

  surfxml_add_callback(ETag_surfxml_platform_cb_list, &parse_ns3_end_platform); //DEBUG
  surfxml_add_callback(ETag_surfxml_platform_cb_list, &create_ns3_topology);
}

static void free_ns3_elmts(void * elmts)
{
}

static void free_ns3_link(void * elmts)
{
	ns3_link_t link = elmts;
	free(link->id);
	free(link->bdw);
	free(link->lat);
	free(link);
}

static void free_ns3_host(void * elmts)
{
	ns3_nodes_t host = elmts;
	free(host);
}

void surf_network_model_init_NS3(const char *filename)
{
	define_callbacks_ns3(filename);
	surf_network_model = surf_model_init();
	surf_network_model->name = "network NS3";
	surf_network_model->extension.network.get_link_latency = ns3_get_link_latency;
	surf_network_model->extension.network.get_link_bandwidth = ns3_get_link_bandwidth;
	surf_network_model->extension.network.get_route = ns3_get_route;
	routing_model_create(sizeof(s_surf_ns3_link_t), NULL, NULL);

	NS3_HOST_LEVEL = xbt_lib_add_level(host_lib,(void_f_pvoid_t)free_ns3_host);
	NS3_ASR_LEVEL  = xbt_lib_add_level(as_router_lib,(void_f_pvoid_t)free_ns3_host);
	NS3_LINK_LEVEL = xbt_lib_add_level(link_lib,(void_f_pvoid_t)free_ns3_link);

	update_model_description(surf_network_model_description,
	            "NS3", surf_network_model);
}
