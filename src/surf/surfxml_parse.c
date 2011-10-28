/* Copyright (c) 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h"
#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/dict.h"
#include "surf/surfxml_parse_private.h"
#include "surf/surf_private.h"

hostSG_t struct_host;
router_t struct_router;
cluster_t struct_cluster;
peer_t struct_peer;
link_t struct_lnk;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_parse, surf,
                                "Logging specific to the SURF parsing module");
#undef CLEANUP
int ETag_surfxml_include_state(void);

#include "simgrid_dtd.c"

char *platform_filename;

/* Initialize the parsing globals */
xbt_dict_t traces_set_list = NULL;
xbt_dict_t trace_connect_list_host_avail = NULL;
xbt_dict_t trace_connect_list_power = NULL;
xbt_dict_t trace_connect_list_link_avail = NULL;
xbt_dict_t trace_connect_list_bandwidth = NULL;
xbt_dict_t trace_connect_list_latency = NULL;

/* This buffer is used to store the original buffer before substituting it by out own buffer. Useful for the foreach tag */
static xbt_dynar_t surfxml_bufferstack_stack = NULL;
int surfxml_bufferstack_size = 2048;

static char *old_buff = NULL;
static void surf_parse_error(char *msg);

unsigned int surfxml_buffer_stack_stack_ptr;
unsigned int surfxml_buffer_stack_stack[1024];


void surfxml_bufferstack_push(int new)
{
  if (!new)
    old_buff = surfxml_bufferstack;
  else {
    xbt_dynar_push(surfxml_bufferstack_stack, &surfxml_bufferstack);
    surfxml_bufferstack = xbt_new0(char, surfxml_bufferstack_size);
  }
}

void surfxml_bufferstack_pop(int new)
{
  if (!new)
    surfxml_bufferstack = old_buff;
  else {
    free(surfxml_bufferstack);
    xbt_dynar_pop(surfxml_bufferstack_stack, &surfxml_bufferstack);
  }
}

/* make sure these symbols are defined as strong ones in this file so that the linked can resolve them */
xbt_dynar_t STag_surfxml_platform_cb_list = NULL;
xbt_dynar_t ETag_surfxml_platform_cb_list = NULL;
xbt_dynar_t STag_surfxml_host_cb_list = NULL;
xbt_dynar_t ETag_surfxml_host_cb_list = NULL;
xbt_dynar_t STag_surfxml_router_cb_list = NULL;
xbt_dynar_t ETag_surfxml_router_cb_list = NULL;
xbt_dynar_t STag_surfxml_link_cb_list = NULL;
xbt_dynar_t ETag_surfxml_link_cb_list = NULL;
xbt_dynar_t STag_surfxml_route_cb_list = NULL;
xbt_dynar_t ETag_surfxml_route_cb_list = NULL;
xbt_dynar_t STag_surfxml_link_ctn_cb_list = NULL;
xbt_dynar_t ETag_surfxml_link_ctn_cb_list = NULL;
xbt_dynar_t STag_surfxml_process_cb_list = NULL;
xbt_dynar_t ETag_surfxml_process_cb_list = NULL;
xbt_dynar_t STag_surfxml_argument_cb_list = NULL;
xbt_dynar_t ETag_surfxml_argument_cb_list = NULL;
xbt_dynar_t STag_surfxml_prop_cb_list = NULL;
xbt_dynar_t ETag_surfxml_prop_cb_list = NULL;
xbt_dynar_t STag_surfxml_cluster_cb_list = NULL;
xbt_dynar_t ETag_surfxml_cluster_cb_list = NULL;
xbt_dynar_t STag_surfxml_peer_cb_list = NULL;
xbt_dynar_t ETag_surfxml_peer_cb_list = NULL;
xbt_dynar_t STag_surfxml_trace_cb_list = NULL;
xbt_dynar_t ETag_surfxml_trace_cb_list = NULL;
xbt_dynar_t STag_surfxml_trace_connect_cb_list = NULL;
xbt_dynar_t ETag_surfxml_trace_connect_cb_list = NULL;
xbt_dynar_t STag_surfxml_random_cb_list = NULL;
xbt_dynar_t ETag_surfxml_random_cb_list = NULL;
xbt_dynar_t STag_surfxml_AS_cb_list = NULL;
xbt_dynar_t ETag_surfxml_AS_cb_list = NULL;
xbt_dynar_t STag_surfxml_ASroute_cb_list = NULL;
xbt_dynar_t ETag_surfxml_ASroute_cb_list = NULL;
xbt_dynar_t STag_surfxml_bypassRoute_cb_list = NULL;
xbt_dynar_t ETag_surfxml_bypassRoute_cb_list = NULL;
xbt_dynar_t STag_surfxml_config_cb_list = NULL;
xbt_dynar_t ETag_surfxml_config_cb_list = NULL;
xbt_dynar_t STag_surfxml_include_cb_list = NULL;
xbt_dynar_t ETag_surfxml_include_cb_list = NULL;

/* store the current property set for any tag */
xbt_dict_t current_property_set = NULL;
/* dictionary of random generator data */
xbt_dict_t random_data_list = NULL;

static XBT_INLINE void surfxml_call_cb_functions(xbt_dynar_t);

YY_BUFFER_STATE surf_input_buffer;
FILE *surf_file_to_parse = NULL;

static void surf_parse_error(char *msg);

static void parse_Stag_trace(void);
static void parse_Etag_trace(void);
static void parse_Stag_trace_connect(void);

static void init_randomness(void);
static void add_randomness(void);

static xbt_dynar_t surf_input_buffer_stack = NULL;
static xbt_dynar_t surf_file_to_parse_stack = NULL;

void STag_surfxml_include(void)
{
  XBT_INFO("STag_surfxml_include '%s'",A_surfxml_include_file);
  xbt_dynar_push(surf_file_to_parse_stack, &surf_file_to_parse); //save old filename

  surf_file_to_parse = surf_fopen(A_surfxml_include_file, "r"); // read new filename
  xbt_assert((surf_file_to_parse), "Unable to open \"%s\"\n",
              A_surfxml_include_file);
  xbt_dynar_push(surf_input_buffer_stack,&surf_input_buffer);
  surf_input_buffer = surf_parse__create_buffer(surf_file_to_parse, YY_BUF_SIZE);
  surf_parse_push_buffer_state(surf_input_buffer);
  fflush(NULL);
}

void ETag_surfxml_include(void)
{
//	XBT_INFO("ETag_surfxml_include '%s'",A_surfxml_include_file);
//	fflush(NULL);
//	fclose(surf_file_to_parse);
//	xbt_dynar_pop(surf_file_to_parse_stack, &surf_file_to_parse); // restore old filename
//	surf_parse_pop_buffer_state();
//	xbt_dynar_pop(surf_input_buffer_stack,&surf_input_buffer);
}

/*
 * Return 1 if tag include is opened
 */
int ETag_surfxml_include_state(void)
{
  fflush(NULL);
  XBT_INFO("ETag_surfxml_include_state '%s'",A_surfxml_include_file);
  if(xbt_dynar_length(surf_input_buffer_stack)!= 0)
	  return 1;
  fclose(surf_file_to_parse);
  xbt_dynar_pop(surf_file_to_parse_stack, &surf_file_to_parse);
  surf_parse_pop_buffer_state();
  xbt_dynar_pop(surf_input_buffer_stack,surf_input_buffer);
  return 0;
}


void surf_parse_init_callbacks(void)
{
	  STag_surfxml_platform_cb_list =
	      xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  ETag_surfxml_platform_cb_list =
	      xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  STag_surfxml_host_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  ETag_surfxml_host_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  STag_surfxml_router_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  ETag_surfxml_router_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  STag_surfxml_link_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  ETag_surfxml_link_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  STag_surfxml_route_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  ETag_surfxml_route_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  STag_surfxml_link_ctn_cb_list =
	      xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  ETag_surfxml_link_ctn_cb_list =
	      xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  STag_surfxml_process_cb_list =
	      xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  ETag_surfxml_process_cb_list =
	      xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  STag_surfxml_argument_cb_list =
	      xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  ETag_surfxml_argument_cb_list =
	      xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  STag_surfxml_prop_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  ETag_surfxml_prop_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  STag_surfxml_trace_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  ETag_surfxml_trace_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  STag_surfxml_trace_connect_cb_list =
	      xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  ETag_surfxml_trace_connect_cb_list =
	      xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  STag_surfxml_random_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  ETag_surfxml_random_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  STag_surfxml_AS_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  ETag_surfxml_AS_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  STag_surfxml_ASroute_cb_list =
	      xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  ETag_surfxml_ASroute_cb_list =
	      xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  STag_surfxml_bypassRoute_cb_list =
	      xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  ETag_surfxml_bypassRoute_cb_list =
	      xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  STag_surfxml_cluster_cb_list =
	      xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  ETag_surfxml_cluster_cb_list =
	      xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  STag_surfxml_peer_cb_list =
	      xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  ETag_surfxml_peer_cb_list =
	      xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  STag_surfxml_config_cb_list =
			  xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  ETag_surfxml_config_cb_list =
			  xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  STag_surfxml_include_cb_list =
			  xbt_dynar_new(sizeof(void_f_void_t), NULL);
	  ETag_surfxml_include_cb_list =
			  xbt_dynar_new(sizeof(void_f_void_t), NULL);
}

void surf_parse_reset_callbacks(void)
{
	surf_parse_free_callbacks();
	surf_parse_init_callbacks();
}

void surf_parse_free_callbacks(void)
{
  xbt_dynar_free(&STag_surfxml_platform_cb_list);
  xbt_dynar_free(&ETag_surfxml_platform_cb_list);
  xbt_dynar_free(&STag_surfxml_host_cb_list);
  xbt_dynar_free(&ETag_surfxml_host_cb_list);
  xbt_dynar_free(&STag_surfxml_router_cb_list);
  xbt_dynar_free(&ETag_surfxml_router_cb_list);
  xbt_dynar_free(&STag_surfxml_link_cb_list);
  xbt_dynar_free(&ETag_surfxml_link_cb_list);
  xbt_dynar_free(&STag_surfxml_route_cb_list);
  xbt_dynar_free(&ETag_surfxml_route_cb_list);
  xbt_dynar_free(&STag_surfxml_link_ctn_cb_list);
  xbt_dynar_free(&ETag_surfxml_link_ctn_cb_list);
  xbt_dynar_free(&STag_surfxml_process_cb_list);
  xbt_dynar_free(&ETag_surfxml_process_cb_list);
  xbt_dynar_free(&STag_surfxml_argument_cb_list);
  xbt_dynar_free(&ETag_surfxml_argument_cb_list);
  xbt_dynar_free(&STag_surfxml_prop_cb_list);
  xbt_dynar_free(&ETag_surfxml_prop_cb_list);
  xbt_dynar_free(&STag_surfxml_trace_cb_list);
  xbt_dynar_free(&ETag_surfxml_trace_cb_list);
  xbt_dynar_free(&STag_surfxml_trace_connect_cb_list);
  xbt_dynar_free(&ETag_surfxml_trace_connect_cb_list);
  xbt_dynar_free(&STag_surfxml_random_cb_list);
  xbt_dynar_free(&ETag_surfxml_random_cb_list);
  xbt_dynar_free(&STag_surfxml_AS_cb_list);
  xbt_dynar_free(&ETag_surfxml_AS_cb_list);
  xbt_dynar_free(&STag_surfxml_ASroute_cb_list);
  xbt_dynar_free(&ETag_surfxml_ASroute_cb_list);
  xbt_dynar_free(&STag_surfxml_bypassRoute_cb_list);
  xbt_dynar_free(&ETag_surfxml_bypassRoute_cb_list);
  xbt_dynar_free(&STag_surfxml_cluster_cb_list);
  xbt_dynar_free(&ETag_surfxml_cluster_cb_list);
  xbt_dynar_free(&STag_surfxml_peer_cb_list);
  xbt_dynar_free(&ETag_surfxml_peer_cb_list);
  xbt_dynar_free(&STag_surfxml_config_cb_list);
  xbt_dynar_free(&ETag_surfxml_config_cb_list);
  xbt_dynar_free(&STag_surfxml_include_cb_list);
  xbt_dynar_free(&ETag_surfxml_include_cb_list);
}

/* Stag and Etag parse functions */

void STag_surfxml_platform(void)
{
  double version = 0.0;

  sscanf(A_surfxml_platform_version, "%lg", &version);

  xbt_assert((version >= 1.0), "******* BIG FAT WARNING *********\n "
      "You're using an ancient XML file.\n"
      "Since SimGrid 3.1, units are Bytes, Flops, and seconds "
      "instead of MBytes, MFlops and seconds.\n"

      "Use simgrid_update_xml to update your file automatically. "
      "This program is installed automatically with SimGrid, or "
      "available in the tools/ directory of the source archive.\n"

      "Please check also out the SURF section of the ChangeLog for "
      "the 3.1 version for more information. \n"

      "Last, do not forget to also update your values for "
      "the calls to MSG_task_create (if any).");
  xbt_assert((version >= 3.0), "******* BIG FAT WARNING *********\n "
      "You're using an old XML file.\n"
      "Use simgrid_update_xml to update your file automatically. "
      "This program is installed automatically with SimGrid, or "
      "available in the tools/ directory of the source archive.");

  surfxml_call_cb_functions(STag_surfxml_platform_cb_list);

}

void STag_surfxml_host(void){
//	XBT_INFO("STag_surfxml_host [%s]",A_surfxml_host_id);
	struct_host = xbt_new0(s_hostSG_t, 1);
	struct_host->V_host_id = xbt_strdup(A_surfxml_host_id);
	struct_host->V_host_power_peak = get_cpu_power(A_surfxml_host_power);
	surf_parse_get_double(&(struct_host->V_host_power_scale), A_surfxml_host_availability);
	surf_parse_get_int(&(struct_host->V_host_core),A_surfxml_host_core);
	struct_host->V_host_power_trace = tmgr_trace_new(A_surfxml_host_availability_file);
	struct_host->V_host_state_trace = tmgr_trace_new(A_surfxml_host_state_file);
	xbt_assert((A_surfxml_host_state == A_surfxml_host_state_ON) ||
			  (A_surfxml_host_state == A_surfxml_host_state_OFF), "Invalid state");
	if (A_surfxml_host_state == A_surfxml_host_state_ON)
		struct_host->V_host_state_initial = SURF_RESOURCE_ON;
	if (A_surfxml_host_state == A_surfxml_host_state_OFF)
		struct_host->V_host_state_initial = SURF_RESOURCE_OFF;
	struct_host->V_host_coord = xbt_strdup(A_surfxml_host_coordinates);

	surfxml_call_cb_functions(STag_surfxml_host_cb_list);
}
void ETag_surfxml_host(void){
	surfxml_call_cb_functions(ETag_surfxml_host_cb_list);
	//xbt_free(struct_host->V_host_id);
	struct_host->V_host_power_peak = 0.0;
	struct_host->V_host_core = 0;
	struct_host->V_host_power_scale = 0.0;
	struct_host->V_host_state_initial = SURF_RESOURCE_ON;
	struct_host->V_host_power_trace = NULL;
	struct_host->V_host_state_trace = NULL;
	xbt_free(struct_host->V_host_coord);
	//xbt_free(host);
}

void STag_surfxml_router(void){
	struct_router = xbt_new0(s_router_t, 1);
	struct_router->V_router_id = xbt_strdup(A_surfxml_router_id);
	struct_router->V_router_coord = xbt_strdup(A_surfxml_router_coordinates);
	surfxml_call_cb_functions(STag_surfxml_router_cb_list);
}
void ETag_surfxml_router(void){
	surfxml_call_cb_functions(ETag_surfxml_router_cb_list);
	xbt_free(struct_router->V_router_id);
	xbt_free(struct_router->V_router_coord);
	xbt_free(struct_router);
}

void STag_surfxml_cluster(void){
	struct_cluster = xbt_new0(s_cluster_t, 1);
	struct_cluster->V_cluster_id = xbt_strdup(A_surfxml_cluster_id);
	struct_cluster->V_cluster_prefix = xbt_strdup(A_surfxml_cluster_prefix);
	struct_cluster->V_cluster_suffix = xbt_strdup(A_surfxml_cluster_suffix);
	struct_cluster->V_cluster_radical = xbt_strdup(A_surfxml_cluster_radical);
	struct_cluster->S_cluster_power = xbt_strdup(A_surfxml_cluster_power);
	struct_cluster->S_cluster_core = xbt_strdup(A_surfxml_cluster_core);
	struct_cluster->S_cluster_bw = xbt_strdup(A_surfxml_cluster_bw);
	struct_cluster->S_cluster_lat = xbt_strdup(A_surfxml_cluster_lat);
	struct_cluster->S_cluster_bb_bw = xbt_strdup(A_surfxml_cluster_bb_bw);
	struct_cluster->S_cluster_bb_lat = xbt_strdup(A_surfxml_cluster_bb_lat);
	if(!strcmp(A_surfxml_cluster_router_id,""))
		struct_cluster->S_cluster_router_id = bprintf("%s%s_router%s",
				struct_cluster->V_cluster_prefix,
				struct_cluster->V_cluster_id,
				struct_cluster->V_cluster_suffix);
	else
		struct_cluster->S_cluster_router_id = xbt_strdup(A_surfxml_cluster_router_id);

	struct_cluster->V_cluster_sharing_policy = AX_surfxml_cluster_sharing_policy;
	struct_cluster->V_cluster_bb_sharing_policy = AX_surfxml_cluster_bb_sharing_policy;

	surfxml_call_cb_functions(STag_surfxml_cluster_cb_list);
}
void ETag_surfxml_cluster(void){
	surfxml_call_cb_functions(ETag_surfxml_cluster_cb_list);
	xbt_free(struct_cluster->V_cluster_id);
	xbt_free(struct_cluster->V_cluster_prefix);
	xbt_free(struct_cluster->V_cluster_suffix);
	xbt_free(struct_cluster->V_cluster_radical);
	xbt_free(struct_cluster->S_cluster_power);
	xbt_free(struct_cluster->S_cluster_core);
	xbt_free(struct_cluster->S_cluster_bw);
	xbt_free(struct_cluster->S_cluster_lat);
	xbt_free(struct_cluster->S_cluster_bb_bw);
	xbt_free(struct_cluster->S_cluster_bb_lat);
	xbt_free(struct_cluster->S_cluster_router_id);
	struct_cluster->V_cluster_sharing_policy = 0;
	struct_cluster->V_cluster_bb_sharing_policy = 0;
	xbt_free(struct_cluster);
}

void STag_surfxml_peer(void){
	struct_peer = xbt_new0(s_peer_t, 1);
	struct_peer->V_peer_id = xbt_strdup(A_surfxml_peer_id);
	struct_peer->V_peer_power = xbt_strdup(A_surfxml_peer_power);
	struct_peer->V_peer_bw_in = xbt_strdup(A_surfxml_peer_bw_in);
	struct_peer->V_peer_bw_out = xbt_strdup(A_surfxml_peer_bw_out);
	struct_peer->V_peer_lat = xbt_strdup(A_surfxml_peer_lat);
	struct_peer->V_peer_coord = xbt_strdup(A_surfxml_peer_coordinates);
	struct_peer->V_peer_availability_trace = xbt_strdup(A_surfxml_peer_availability_file);
	struct_peer->V_peer_state_trace = xbt_strdup(A_surfxml_peer_state_file);
	surfxml_call_cb_functions(STag_surfxml_peer_cb_list);
}
void ETag_surfxml_peer(void){
	surfxml_call_cb_functions(ETag_surfxml_peer_cb_list);
	xbt_free(struct_peer->V_peer_id);
	xbt_free(struct_peer->V_peer_power);
	xbt_free(struct_peer->V_peer_bw_in);
	xbt_free(struct_peer->V_peer_bw_out);
	xbt_free(struct_peer->V_peer_lat);
	xbt_free(struct_peer->V_peer_coord);
	xbt_free(struct_peer->V_peer_availability_trace);
	xbt_free(struct_peer->V_peer_state_trace);
	xbt_free(struct_peer);
}
void STag_surfxml_link(void){
	struct_lnk = xbt_new0(s_link_t, 1);
	struct_lnk->V_link_id = xbt_strdup(A_surfxml_link_id);
	surf_parse_get_double(&(struct_lnk->V_link_bandwidth),A_surfxml_link_bandwidth);
	struct_lnk->V_link_bandwidth_file = tmgr_trace_new(A_surfxml_link_bandwidth_file);
	surf_parse_get_double(&(struct_lnk->V_link_latency),A_surfxml_link_latency);
	struct_lnk->V_link_latency_file = tmgr_trace_new(A_surfxml_link_latency_file);
	xbt_assert((A_surfxml_link_state == A_surfxml_link_state_ON) ||
			  (A_surfxml_link_state == A_surfxml_link_state_OFF), "Invalid state");
	if (A_surfxml_link_state == A_surfxml_link_state_ON)
		struct_lnk->V_link_state = SURF_RESOURCE_ON;
	if (A_surfxml_link_state == A_surfxml_link_state_OFF)
		struct_lnk->V_link_state = SURF_RESOURCE_OFF;
	struct_lnk->V_link_state_file = tmgr_trace_new(A_surfxml_link_state_file);
	struct_lnk->V_link_sharing_policy = A_surfxml_link_sharing_policy;

	if (A_surfxml_link_sharing_policy == A_surfxml_link_sharing_policy_SHARED)
		struct_lnk->V_policy_initial_link = SURF_LINK_SHARED;
	else
	{
	 if (A_surfxml_link_sharing_policy == A_surfxml_link_sharing_policy_FATPIPE)
		 struct_lnk->V_policy_initial_link = SURF_LINK_FATPIPE;
	 else if (A_surfxml_link_sharing_policy == A_surfxml_link_sharing_policy_FULLDUPLEX)
		 struct_lnk->V_policy_initial_link = SURF_LINK_FULLDUPLEX;
	}


	surfxml_call_cb_functions(STag_surfxml_link_cb_list);
}
void ETag_surfxml_link(void){
	surfxml_call_cb_functions(ETag_surfxml_link_cb_list);
	xbt_free(struct_lnk->V_link_id);
	struct_lnk->V_link_bandwidth = 0;
	struct_lnk->V_link_bandwidth_file = NULL;
	struct_lnk->V_link_latency = 0;
	struct_lnk->V_link_latency_file = NULL;
	struct_lnk->V_link_state = SURF_RESOURCE_ON;
	struct_lnk->V_link_state_file = NULL;
	struct_lnk->V_link_sharing_policy = 0;
	xbt_free(struct_lnk);
}

void STag_surfxml_route(void){
	surfxml_call_cb_functions(STag_surfxml_route_cb_list);
}
void STag_surfxml_link_ctn(void){
	surfxml_call_cb_functions(STag_surfxml_link_ctn_cb_list);
}
void STag_surfxml_process(void){
	surfxml_call_cb_functions(STag_surfxml_process_cb_list);
}
void STag_surfxml_argument(void){
	surfxml_call_cb_functions(STag_surfxml_argument_cb_list);
}
void STag_surfxml_prop(void){
	surfxml_call_cb_functions(STag_surfxml_prop_cb_list);
}
void STag_surfxml_trace(void){
	surfxml_call_cb_functions(STag_surfxml_trace_cb_list);
}
void STag_surfxml_trace_connect(void){
	surfxml_call_cb_functions(STag_surfxml_trace_connect_cb_list);
}
void STag_surfxml_AS(void){
	surfxml_call_cb_functions(STag_surfxml_AS_cb_list);
}
void STag_surfxml_ASroute(void){
	surfxml_call_cb_functions(STag_surfxml_ASroute_cb_list);
}
void STag_surfxml_bypassRoute(void){
	surfxml_call_cb_functions(STag_surfxml_bypassRoute_cb_list);
}
void STag_surfxml_config(void){
	surfxml_call_cb_functions(STag_surfxml_config_cb_list);
}
void STag_surfxml_random(void){
	surfxml_call_cb_functions(STag_surfxml_random_cb_list);
}

#define parse_method(type,name) \
void type##Tag_surfxml_##name(void) \
{ surfxml_call_cb_functions(type##Tag_surfxml_##name##_cb_list); }
parse_method(E, platform);
parse_method(E, route);
parse_method(E, link_ctn);
parse_method(E, process);
parse_method(E, argument);
parse_method(E, prop);
parse_method(E, trace);
parse_method(E, trace_connect);
parse_method(E, random);
parse_method(E, AS);
parse_method(E, ASroute);
parse_method(E, bypassRoute);
parse_method(E, config);

/* Open and Close parse file */

void surf_parse_open(const char *file)
{
  static int warned = 0;        /* warn only once */
  if (!file) {
    if (!warned) {
      XBT_WARN
          ("Bypassing the XML parser since surf_parse_open received a NULL pointer. If it is not what you want, go fix your code.");
      warned = 1;
    }
    return;
  }

  if (!surf_input_buffer_stack)
	surf_input_buffer_stack = xbt_dynar_new(sizeof(YY_BUFFER_STATE), NULL);
  if (!surf_file_to_parse_stack)
	surf_file_to_parse_stack = xbt_dynar_new(sizeof(FILE *), NULL);

  surf_file_to_parse = surf_fopen(file, "r");
  xbt_assert((surf_file_to_parse), "Unable to open \"%s\"\n", file);
  surf_input_buffer = surf_parse__create_buffer(surf_file_to_parse, YY_BUF_SIZE);
  surf_parse__switch_to_buffer(surf_input_buffer);
  surf_parse_lineno = 1;
}

void surf_parse_close(void)
{
  if (surf_input_buffer_stack)
	xbt_dynar_free(&surf_input_buffer_stack);
  if (surf_file_to_parse_stack)
	xbt_dynar_free(&surf_file_to_parse_stack);

  if (surf_file_to_parse) {
    surf_parse__delete_buffer(surf_input_buffer);
    fclose(surf_file_to_parse);
    surf_file_to_parse = NULL; //Must be reset for Bypass
  }
}

/* Parse Function */

static int _surf_parse(void)
{
  return surf_parse_lex();
}

int_f_void_t surf_parse = _surf_parse;

void surf_parse_error(char *msg)
{
  fprintf(stderr, "Parse error on line %d: %s\n", surf_parse_lineno, msg);
  abort();
}

void surf_parse_get_double(double *value, const char *string)
{
  int ret = 0;
  ret = sscanf(string, "%lg", value);
  if (ret != 1)
    surf_parse_error(bprintf("%s is not a double", string));
}

void surf_parse_get_int(int *value, const char *string)
{
  int ret = 0;
  ret = sscanf(string, "%d", value);
  if (ret != 1)
    surf_parse_error(bprintf("%s is not an integer", string));
}

/* Aux parse functions */

void surfxml_add_callback(xbt_dynar_t cb_list, void_f_void_t function)
{
  xbt_dynar_push(cb_list, &function);
}

void surfxml_del_callback(xbt_dynar_t cb_list, void_f_void_t function)
{
  xbt_ex_t e;
  unsigned int it=0;
  void_f_void_t null_f=NULL;

  TRY {
    it = xbt_dynar_search(cb_list,&function);
  }
  CATCH(e) {
    if (e.category == not_found_error) {
      xbt_ex_free(e);
      xbt_die("Trying to remove a callback that is not here! This should not happen");
    }
    RETHROW;
  }

  xbt_dynar_replace(cb_list, it,&null_f);
}

static XBT_INLINE void surfxml_call_cb_functions(xbt_dynar_t cb_list)
{
  unsigned int iterator;
  void_f_void_t fun;
  xbt_dynar_foreach(cb_list, iterator, fun) {
    if (fun) (*fun) ();
  }
}

/* Init and free parse data */

static void init_data(void)
{
  if (!surfxml_bufferstack_stack)
    surfxml_bufferstack_stack = xbt_dynar_new(sizeof(char *), NULL);

  traces_set_list = xbt_dict_new();
  trace_connect_list_host_avail = xbt_dict_new();
  trace_connect_list_power = xbt_dict_new();
  trace_connect_list_link_avail = xbt_dict_new();
  trace_connect_list_bandwidth = xbt_dict_new();
  trace_connect_list_latency = xbt_dict_new();

  surfxml_add_callback(STag_surfxml_trace_cb_list, &parse_Stag_trace);
  surfxml_add_callback(ETag_surfxml_trace_cb_list, &parse_Etag_trace);
  surfxml_add_callback(STag_surfxml_trace_connect_cb_list,
					   &parse_Stag_trace_connect);
}

static void free_data(void)
{
  xbt_dict_free(&trace_connect_list_host_avail);
  xbt_dict_free(&trace_connect_list_power);
  xbt_dict_free(&trace_connect_list_link_avail);
  xbt_dict_free(&trace_connect_list_bandwidth);
  xbt_dict_free(&trace_connect_list_latency);
  xbt_dict_free(&traces_set_list);
  xbt_dict_free(&random_data_list);
  xbt_dynar_free(&surfxml_bufferstack_stack);
}

/* Here start parse */
void parse_platform_file(const char *file)
{
  int parse_status;

  surfxml_buffer_stack_stack_ptr = 1;
  surfxml_buffer_stack_stack[0] = 0;

  surf_parse_open(file);
  init_data();
  parse_status = surf_parse();
  free_data();
  surf_parse_close();
  xbt_assert(!parse_status, "Parse error in %s", file);
}

/* Prop tag functions */

void parse_properties(const char* prop_id, const char* prop_value)
{
    char *value = NULL;
	if (!current_property_set)
	    current_property_set = xbt_dict_new();      // Maybe, it should be make a error
	if(!strcmp(prop_id,"coordinates")){
		if(!strcmp(prop_value,"yes") && !COORD_HOST_LEVEL)
		  {
			    XBT_INFO("Configuration change: Set '%s' to '%s'", prop_id, prop_value);
				COORD_HOST_LEVEL = xbt_lib_add_level(host_lib,xbt_dynar_free_voidp);
				COORD_ASR_LEVEL  = xbt_lib_add_level(as_router_lib,xbt_dynar_free_voidp);
		  }
		if(strcmp(A_surfxml_prop_value,"yes"))
			  xbt_die("Setting XML prop coordinates must be \"yes\"");
	  }
	else{
		  value = xbt_strdup(prop_value);
		  xbt_dict_set(current_property_set, prop_id, value, free);
	 }
}

/**
 * With XML parser
 */
void parse_properties_XML(void)
{
  parse_properties(A_surfxml_prop_id, A_surfxml_prop_value);
}

/*
 * With lua console
 */
void parse_properties_lua(const char* prop_id, const char* prop_value)
{
	 parse_properties(prop_id, prop_value);
}

/* Trace management functions */

static double trace_periodicity = -1.0;
static char *trace_file = NULL;
static char *trace_id = NULL;

static void parse_Stag_trace(void)
{
  trace_id = xbt_strdup(A_surfxml_trace_id);
  trace_file = xbt_strdup(A_surfxml_trace_file);
  surf_parse_get_double(&trace_periodicity, A_surfxml_trace_periodicity);
}

static void parse_Etag_trace(void)
{
  tmgr_trace_t trace;
  if (!trace_file || strcmp(trace_file, "") != 0) {
    trace = tmgr_trace_new(trace_file);
  } else {
    if (strcmp(surfxml_pcdata, "") == 0)
      trace = NULL;
    else
      trace =
          tmgr_trace_new_from_string(trace_id, surfxml_pcdata,
                                     trace_periodicity);
  }
  xbt_dict_set(traces_set_list, trace_id, (void *) trace, NULL);
  xbt_free(trace_file);
  trace_file = NULL;
  xbt_free(trace_id);
  trace_id = NULL;
}

static void parse_Stag_trace_connect(void)
{
  xbt_assert(xbt_dict_get_or_null
              (traces_set_list, A_surfxml_trace_connect_trace),
              "Cannot connect trace %s to %s: trace unknown",
              A_surfxml_trace_connect_trace,
              A_surfxml_trace_connect_element);

  switch (A_surfxml_trace_connect_kind) {
  case A_surfxml_trace_connect_kind_HOST_AVAIL:
    xbt_dict_set(trace_connect_list_host_avail,
                 A_surfxml_trace_connect_trace,
                 xbt_strdup(A_surfxml_trace_connect_element), free);
    break;
  case A_surfxml_trace_connect_kind_POWER:
    xbt_dict_set(trace_connect_list_power, A_surfxml_trace_connect_trace,
                 xbt_strdup(A_surfxml_trace_connect_element), free);
    break;
  case A_surfxml_trace_connect_kind_LINK_AVAIL:
    xbt_dict_set(trace_connect_list_link_avail,
                 A_surfxml_trace_connect_trace,
                 xbt_strdup(A_surfxml_trace_connect_element), free);
    break;
  case A_surfxml_trace_connect_kind_BANDWIDTH:
    xbt_dict_set(trace_connect_list_bandwidth,
                 A_surfxml_trace_connect_trace,
                 xbt_strdup(A_surfxml_trace_connect_element), free);
    break;
  case A_surfxml_trace_connect_kind_LATENCY:
    xbt_dict_set(trace_connect_list_latency, A_surfxml_trace_connect_trace,
                 xbt_strdup(A_surfxml_trace_connect_element), free);
    break;
  default:
    xbt_die("Cannot connect trace %s to %s: kind of trace unknown",
            A_surfxml_trace_connect_trace, A_surfxml_trace_connect_element);
    break;
  }
}

/* Random tag functions */

double get_cpu_power(const char *power)
{
  double power_scale = 0.0;
  const char *p, *q;
  char *generator;
  random_data_t random = NULL;
  /* randomness is inserted like this: power="$rand(my_random)" */
  if (((p = strstr(power, "$rand(")) != NULL)
      && ((q = strstr(power, ")")) != NULL)) {
    if (p < q) {
      generator = xbt_malloc(q - (p + 6) + 1);
      memcpy(generator, p + 6, q - (p + 6));
      generator[q - (p + 6)] = '\0';
      random = xbt_dict_get_or_null(random_data_list, generator);
      xbt_assert(random, "Random generator %s undefined", generator);
      power_scale = random_generate(random);
    }
  } else {
    surf_parse_get_double(&power_scale, power);
  }
  return power_scale;
}

double random_min, random_max, random_mean, random_std_deviation,
    random_generator;
char *random_id;

static void init_randomness(void)
{
  random_id = A_surfxml_random_id;
  surf_parse_get_double(&random_min, A_surfxml_random_min);
  surf_parse_get_double(&random_max, A_surfxml_random_max);
  surf_parse_get_double(&random_mean, A_surfxml_random_mean);
  surf_parse_get_double(&random_std_deviation,
                        A_surfxml_random_std_deviation);
  random_generator = A_surfxml_random_generator;
}

static void add_randomness(void)
{
  /* If needed aditional properties can be added by using the prop tag */
  random_data_t random =
      random_new(random_generator, 0, random_min, random_max, random_mean,
                 random_std_deviation);
  xbt_dict_set(random_data_list, random_id, (void *) random,
               &xbt_free_ref);
}

/**
 * create CPU resource via CPU Model
 */
void* surf_host_create_resource(char *name, double power_peak,
                               double power_scale,
                               tmgr_trace_t power_trace, int core,
                               e_surf_resource_state_t state_initial,
                               tmgr_trace_t state_trace,
                               xbt_dict_t cpu_properties)
{
  return surf_cpu_model->extension.cpu.create_resource(name, power_peak,
                                                       power_scale,
                                                       power_trace,
                                                       core,
                                                       state_initial,
                                                       state_trace,
                                                       cpu_properties);
}

/**
 * create CPU resource via worsktation_ptask_L07 model
 */

void* surf_wsL07_host_create_resource(char *name, double power_peak,
                                     double power_scale,
                                     tmgr_trace_t power_trace,
                                     e_surf_resource_state_t state_initial,
                                     tmgr_trace_t state_trace,
                                     xbt_dict_t cpu_properties)
{
  return surf_workstation_model->extension.workstation.cpu_create_resource(name,
                                                                    power_peak,
                                                                    power_scale,
                                                                    power_trace,
                                                                    state_initial,
                                                                    state_trace,
                                                                    cpu_properties);
}

/**
 * create link resource via network Model
 */
void* surf_link_create_resource(char *name,
                               double bw_initial,
                               tmgr_trace_t bw_trace,
                               double lat_initial,
                               tmgr_trace_t lat_trace,
                               e_surf_resource_state_t
                               state_initial,
                               tmgr_trace_t state_trace,
                               e_surf_link_sharing_policy_t policy,
                               xbt_dict_t properties)
{
  return surf_network_model->extension.network.create_resource(name,
                                                               bw_initial,
                                                               bw_trace,
                                                               lat_initial,
                                                               lat_trace,
                                                               state_initial,
                                                               state_trace,
                                                               policy,
                                                               properties);
}

/**
 * create link resource via workstation_ptask_L07 model
 */

void* surf_wsL07_link_create_resource(char *name,
                                     double bw_initial,
                                     tmgr_trace_t bw_trace,
                                     double lat_initial,
                                     tmgr_trace_t lat_trace,
                                     e_surf_resource_state_t
                                     state_initial,
                                     tmgr_trace_t state_trace,
                                     e_surf_link_sharing_policy_t
                                     policy, xbt_dict_t properties)
{
  return surf_workstation_model->extension.workstation.
      link_create_resource(name, bw_initial, bw_trace, lat_initial,
                           lat_trace, state_initial, state_trace, policy,
                           properties);
}

/**
 *
 *init new routing model component
 */

void surf_AS_new(const char *AS_id, const char *AS_mode)
{
  routing_AS_init(AS_id, AS_mode);
}

void surf_AS_finalize(const char *AS_id)
{
  routing_AS_end(AS_id);
}

/*
 * add host to the network element list
 */
void surf_route_add_host(const char *host_id)
{
  routing_add_host(host_id);
}

/**
 * set route
 */
void surf_routing_add_route(const char *src_id, const char *dst_id,
                            xbt_dynar_t links_id)
{
  unsigned int i;
  const char *link_id;
  routing_set_route(src_id, dst_id);
  xbt_dynar_foreach(links_id, i, link_id) {
    routing_add_link(link_id);
  }

  //store the route
  routing_store_route();
}

/**
 * Add Traces
 */
void surf_add_host_traces(void)
{
  return surf_cpu_model->extension.cpu.add_traces();
}

void surf_add_link_traces(void)
{
  return surf_network_model->extension.network.add_traces();
}

void surf_wsL07_add_traces(void)
{
  return surf_workstation_model->extension.workstation.add_traces();
}
