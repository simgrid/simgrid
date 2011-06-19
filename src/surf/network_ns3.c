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
	xbt_free(bdw);
	bdw = bprintf("%fbps",atof(temp));
	xbt_free(temp);

}

static void replace_lat_ns3(char * lat)
{
	char *temp = xbt_strdup(lat);
	xbt_free(lat);
	lat = bprintf("%fs",atof(temp));
	xbt_free(temp);
}

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
	link->created = 1;

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
	char *cluster_prefix = A_surfxml_cluster_prefix;
	char *cluster_suffix = A_surfxml_cluster_suffix;
	char *cluster_radical = A_surfxml_cluster_radical;
	char *cluster_bb_bw = A_surfxml_cluster_bb_bw;
	char *cluster_bb_lat = A_surfxml_cluster_bb_lat;
	char *cluster_bw = A_surfxml_cluster_bw;
	char *cluster_lat = A_surfxml_cluster_lat;
	char *groups = NULL;

	int start, end, i;
	unsigned int iter;

	xbt_dynar_t radical_elements;
	xbt_dynar_t radical_ends;
	xbt_dynar_t tab_elements_num = xbt_dynar_new(sizeof(int), NULL);

	char *router_id,*host_id;

	radical_elements = xbt_str_split(cluster_radical, ",");
	xbt_dynar_foreach(radical_elements, iter, groups) {
		radical_ends = xbt_str_split(groups, "-");

		switch (xbt_dynar_length(radical_ends)) {
		case 1:
		  surf_parse_get_int(&start,xbt_dynar_get_as(radical_ends, 0, char *));
		  xbt_dynar_push_as(tab_elements_num, int, start);
		  router_id = bprintf("ns3_%s%d%s", cluster_prefix, start, cluster_suffix);
		  xbt_lib_set(host_lib,
						router_id,
						NS3_HOST_LEVEL,
						ns3_add_host_cluster(router_id)
						);
		  XBT_DEBUG("NS3_ADD_ROUTER '%s'",router_id);
		  free(router_id);
		  break;

		case 2:
		  surf_parse_get_int(&start,xbt_dynar_get_as(radical_ends, 0, char *));
		  surf_parse_get_int(&end, xbt_dynar_get_as(radical_ends, 1, char *));
		  for (i = start; i <= end; i++){
			xbt_dynar_push_as(tab_elements_num, int, i);
			router_id = bprintf("ns3_%s%d%s", cluster_prefix, i, cluster_suffix);
			xbt_lib_set(host_lib,
						router_id,
						NS3_HOST_LEVEL,
						ns3_add_host_cluster(router_id)
						);
			XBT_DEBUG("NS3_ADD_ROUTER '%s'",router_id);
			free(router_id);
		  }
		  break;

		default:
		  XBT_DEBUG("Malformed radical");
		}
	}



	//Create links
	unsigned int cpt;
	int elmts;
	char * lat = xbt_strdup(cluster_lat);
	char * bw =  xbt_strdup(cluster_bw);
	replace_lat_ns3(lat);
	replace_bdw_ns3(bw);

	xbt_dynar_foreach(tab_elements_num,cpt,elmts)
	{
		host_id   = bprintf("%s%d%s", cluster_prefix, elmts, cluster_suffix);
		router_id = bprintf("ns3_%s%d%s", cluster_prefix, elmts, cluster_suffix);
		XBT_DEBUG("Create link from '%s' to '%s'",host_id,router_id);

		ns3_nodes_t host_src = xbt_lib_get_or_null(host_lib,host_id,  NS3_HOST_LEVEL);
		ns3_nodes_t host_dst = xbt_lib_get_or_null(host_lib,router_id,NS3_HOST_LEVEL);

		if(host_src && host_dst){}
		else xbt_die("\tns3_add_link from %d to %d",host_src->node_num,host_dst->node_num);

		ns3_add_link(host_src->node_num,host_dst->node_num,bw,lat);

		free(router_id);
		free(host_id);
	}
	xbt_dynar_free(&tab_elements_num);


	//Create link backbone
	lat = xbt_strdup(cluster_bb_lat);
	bw =  xbt_strdup(cluster_bb_bw);
	replace_lat_ns3(lat);
	replace_bdw_ns3(bw);
	ns3_add_cluster(bw,lat,A_surfxml_cluster_id);
	xbt_free(lat);
	xbt_free(bw);	
}

double ns3_get_link_latency (const void *link)
{
	double lat;
	//XBT_DEBUG("link_id:%s link_lat:%s link_bdw:%s",((surf_ns3_link_t)link)->data->id,((surf_ns3_link_t)link)->data->lat,((surf_ns3_link_t)link)->data->bdw);
	sscanf(((surf_ns3_link_t)link)->data->lat,"%lg",&lat);
	return lat;
}
double ns3_get_link_bandwidth (const void *link)
{
	double bdw;
	//XBT_DEBUG("link_id:%s link_lat:%s link_bdw:%s",((surf_ns3_link_t)link)->data->id,((surf_ns3_link_t)link)->data->lat,((surf_ns3_link_t)link)->data->bdw);
	sscanf(((surf_ns3_link_t)link)->data->bdw,"%lg",&bdw);
	return bdw;
}

static xbt_dynar_t ns3_get_route(const char *src, const char *dst)
{
  return global_routing->get_route(src, dst);
}

void parse_ns3_end_platform(void)
{
	ns3_end_platform();

	  xbt_lib_cursor_t cursor = NULL;
	  char *name = NULL;
	  void **data = NULL;
	  XBT_DEBUG("link_lib");
	  xbt_lib_foreach(link_lib, cursor, name, data) {
			XBT_DEBUG("\tSee link '%s'\t--> NS3_LEVEL %p",
					name,
					data[NS3_LINK_LEVEL]);
	  }
	  XBT_DEBUG(" ");
	  XBT_DEBUG("host_lib");
	  xbt_lib_foreach(host_lib, cursor, name, data) {
			XBT_DEBUG("\tSee host '%s'\t--> NS3_LEVEL %p",
					name,
					data[NS3_HOST_LEVEL]);
	  }
	  XBT_DEBUG(" ");
	  XBT_DEBUG("as_router_lib");
	  xbt_lib_foreach(as_router_lib, cursor, name, data) {
			XBT_DEBUG("\tSee ASR '%s'\t--> NS3_LEVEL %p",
					name,
					data[NS3_ASR_LEVEL]);
	  }

	  XBT_DEBUG(" ");
}

/* Create the ns3 topology based on routing strategy */
void create_ns3_topology()
{
   XBT_INFO("Starting topology generation");

   //get the onelinks from the parsed platform
   xbt_dynar_t onelink_routes = global_routing->get_onelink_routes();
   if (!onelink_routes)
     xbt_die("There is no routes!");
   XBT_INFO("Have get_onelink_routes, found %ld routes",onelink_routes->used);
   //save them in trace file
   onelink_t onelink;
   unsigned int iter;
   xbt_dynar_foreach(onelink_routes, iter, onelink) {
     char *src = onelink->src;
     char *dst = onelink->dst;
     void *link = onelink->link_ptr;

     if( strcmp(src,dst) && ((surf_ns3_link_t)link)->created){
     XBT_INFO("Route from '%s' to '%s' with link '%s'",src,dst,((surf_ns3_link_t)link)->data->id);
     char * link_bdw = xbt_strdup(((surf_ns3_link_t)link)->data->bdw);
	 char * link_lat = xbt_strdup(((surf_ns3_link_t)link)->data->lat);
	 ((surf_ns3_link_t)link)->created = 0;

	 replace_bdw_ns3(link_bdw);
	 replace_lat_ns3(link_lat);
//	 XBT_INFO("src (%s), dst (%s), src_id = %d, dst_id = %d",src,dst, src_id, dst_id);
     XBT_INFO("\tLink (%s) bdw:%s->%s lat:%s->%s",((surf_ns3_link_t)link)->data->id,
    		 ((surf_ns3_link_t)link)->data->bdw,link_bdw,
    		 ((surf_ns3_link_t)link)->data->lat,link_lat
    		 );

     //create link ns3
     ns3_nodes_t host_src = xbt_lib_get_or_null(host_lib,src,NS3_HOST_LEVEL);
     if(!host_src) host_src = xbt_lib_get_or_null(as_router_lib,src,NS3_ASR_LEVEL);
     ns3_nodes_t host_dst = xbt_lib_get_or_null(host_lib,dst,NS3_HOST_LEVEL);
     if(!host_dst) host_dst = xbt_lib_get_or_null(as_router_lib,dst,NS3_ASR_LEVEL);

     if(host_src && host_dst){}
     else xbt_die("\tns3_add_link from %d to %d",host_src->node_num,host_dst->node_num);

     ns3_add_link(host_src->node_num,host_dst->node_num,link_bdw,link_lat);

     xbt_free(link_bdw);
     xbt_free(link_lat);
     }
   }
}

static void define_callbacks_ns3(const char *filename)
{
  surfxml_add_callback(STag_surfxml_host_cb_list, &parse_ns3_add_host);	      //HOST
  surfxml_add_callback(STag_surfxml_router_cb_list, &parse_ns3_add_router);	  //ROUTER
  surfxml_add_callback(STag_surfxml_link_cb_list, &parse_ns3_add_link);	      //LINK
  surfxml_add_callback(STag_surfxml_AS_cb_list, &parse_ns3_add_AS);		      //AS
  surfxml_add_callback(STag_surfxml_cluster_cb_list, &parse_ns3_add_cluster); //CLUSTER

  surfxml_add_callback(ETag_surfxml_platform_cb_list, &create_ns3_topology);    //get_one_link_routes
  surfxml_add_callback(ETag_surfxml_platform_cb_list, &parse_ns3_end_platform); //InitializeRoutes
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
	surf_network_model = surf_model_init();
	surf_network_model->name = "network NS3";
	surf_network_model->extension.network.get_link_latency = ns3_get_link_latency;
	surf_network_model->extension.network.get_link_bandwidth = ns3_get_link_bandwidth;
	surf_network_model->extension.network.get_route = ns3_get_route;
	routing_model_create(sizeof(s_surf_ns3_link_t), NULL, NULL);
	define_callbacks_ns3(filename);

	NS3_HOST_LEVEL = xbt_lib_add_level(host_lib,(void_f_pvoid_t)free_ns3_host);
	NS3_ASR_LEVEL  = xbt_lib_add_level(as_router_lib,(void_f_pvoid_t)free_ns3_host);
	NS3_LINK_LEVEL = xbt_lib_add_level(link_lib,(void_f_pvoid_t)free_ns3_link);

	update_model_description(surf_network_model_description,
	            "NS3", surf_network_model);
}
