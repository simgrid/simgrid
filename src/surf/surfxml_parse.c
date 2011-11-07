/* Copyright (c) 2006, 2007, 2008, 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h"
#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/dict.h"
#include "surf/surfxml_parse_private.h"
#include "surf/surf_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_parse, surf,
                                "Logging specific to the SURF parsing module");
#undef CLEANUP
int ETag_surfxml_include_state(void);

#include "simgrid_dtd.c"

/*
 * Helping functions
 */
static void surf_parse_error(char *msg) {
  xbt_die("Parse error on line %d: %s\n", surf_parse_lineno, msg);
}

double surf_parse_get_double(const char *string) {
  double res;
  int ret = sscanf(string, "%lg", &res);
  if (ret != 1)
    surf_parse_error(bprintf("%s is not a double", string));
  return res;
}

int surf_parse_get_int(const char *string) {
  int res;
  int ret = sscanf(string, "%d", &res);
  if (ret != 1)
    surf_parse_error(bprintf("%s is not an integer", string));
  return res;
}


/*
 * All the callback lists that can be overiden anywhere.
 * (this list should probably be reduced to the bare minimum to allow the models to work)
 */

/* make sure these symbols are defined as strong ones in this file so that the linker can resolve them */
//xbt_dynar_t STag_surfxml_host_cb_list = NULL;
xbt_dynar_t STag_surfxml_platform_cb_list = NULL;
xbt_dynar_t ETag_surfxml_platform_cb_list = NULL;
xbt_dynar_t ETag_surfxml_host_cb_list = NULL;
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
xbt_dynar_t STag_surfxml_include_cb_list = NULL;
xbt_dynar_t ETag_surfxml_include_cb_list = NULL;

/* The default current property receiver. Setup in the corresponding opening callbacks. */
xbt_dict_t current_property_set = NULL;
/* dictionary of random generator data */
xbt_dict_t random_data_list = NULL;

/* Call all the callbacks of a specific SAX event */
static XBT_INLINE void surfxml_call_cb_functions(xbt_dynar_t);

YY_BUFFER_STATE surf_input_buffer;
FILE *surf_file_to_parse = NULL;

static void parse_Stag_trace(void);
static void parse_Etag_trace(void);
static void parse_Stag_trace_connect(void);

static void init_randomness(void);
static void add_randomness(void);

/*
 * Stuff relative to the <include> tag
 */

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

void ETag_surfxml_include(void) {
/* Nothing to do when done with reading the include tag.
 * Instead, the handling should be deferred until the EOF of current buffer -- see below */
}

/** @brief When reaching EOF, check whether we are in an include tag, and behave accordingly if yes
 *
 * This function is called automatically by sedding the parser in buildtools/Cmake/MaintainerMode.cmake
 * Every FAIL on "Premature EOF" is preceded by a call to this function, which role is to restore the
 * previous buffer if we reached the EOF /of an include file/. Its return code is used to avoid the
 * error message in that case.
 *
 * Yeah, that's terribly hackish, but it works. A better solution should be dealed with in flexml
 * directly: a command line flag could instruct it to do the correct thing when #include is encountered
 * on a line.
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

	  sg_platf_init();
	  ETag_surfxml_host_cb_list = xbt_dynar_new(sizeof(void_f_void_t), NULL);
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
  sg_platf_exit();

  xbt_dynar_free(&STag_surfxml_platform_cb_list);
  xbt_dynar_free(&ETag_surfxml_platform_cb_list);
  xbt_dynar_free(&ETag_surfxml_host_cb_list);
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
  xbt_dynar_free(&STag_surfxml_include_cb_list);
  xbt_dynar_free(&ETag_surfxml_include_cb_list);
}

/* Stag and Etag parse functions */

void STag_surfxml_platform(void)
{
  _XBT_GNUC_UNUSED double version = surf_parse_get_double(A_surfxml_platform_version);

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
  s_sg_platf_host_cbarg_t host;
  memset(&host,0,sizeof(host));

	host.V_host_id = A_surfxml_host_id;
	host.V_host_power_peak = get_cpu_power(A_surfxml_host_power);
	host.V_host_power_scale = surf_parse_get_double( A_surfxml_host_availability);
	host.V_host_core = surf_parse_get_int(A_surfxml_host_core);
	host.V_host_power_trace = tmgr_trace_new(A_surfxml_host_availability_file);
	host.V_host_state_trace = tmgr_trace_new(A_surfxml_host_state_file);
	xbt_assert((A_surfxml_host_state == A_surfxml_host_state_ON) ||
			  (A_surfxml_host_state == A_surfxml_host_state_OFF), "Invalid state");
	if (A_surfxml_host_state == A_surfxml_host_state_ON)
		host.V_host_state_initial = SURF_RESOURCE_ON;
	if (A_surfxml_host_state == A_surfxml_host_state_OFF)
		host.V_host_state_initial = SURF_RESOURCE_OFF;
	host.V_host_coord = A_surfxml_host_coordinates;

	sg_platf_new_host(&host);
}
void ETag_surfxml_host(void){
	surfxml_call_cb_functions(ETag_surfxml_host_cb_list);
}


void STag_surfxml_router(void){
  s_sg_platf_router_cbarg_t router;
  memset(&router, 0, sizeof(router));

	router.V_router_id = xbt_strdup(A_surfxml_router_id);
	router.V_router_coord = xbt_strdup(A_surfxml_router_coordinates);
	sg_platf_new_router(&router);
}
void ETag_surfxml_router(void){
	surfxml_call_cb_functions(ETag_surfxml_router_cb_list);
}

void STag_surfxml_cluster(void){
	struct_cluster = xbt_new0(s_surf_parsing_cluster_arg_t, 1);
	struct_cluster->V_cluster_id = xbt_strdup(A_surfxml_cluster_id);
	struct_cluster->V_cluster_prefix = xbt_strdup(A_surfxml_cluster_prefix);
	struct_cluster->V_cluster_suffix = xbt_strdup(A_surfxml_cluster_suffix);
	struct_cluster->V_cluster_radical = xbt_strdup(A_surfxml_cluster_radical);
	struct_cluster->S_cluster_power= surf_parse_get_double(A_surfxml_cluster_power);
	struct_cluster->S_cluster_core = surf_parse_get_int(A_surfxml_cluster_core);
	struct_cluster->S_cluster_bw =   surf_parse_get_double(A_surfxml_cluster_bw);
	struct_cluster->S_cluster_lat =  surf_parse_get_double(A_surfxml_cluster_lat);
	if(strcmp(A_surfxml_cluster_bb_bw,""))
	  struct_cluster->S_cluster_bb_bw = surf_parse_get_double(A_surfxml_cluster_bb_bw);
	if(strcmp(A_surfxml_cluster_bb_lat,""))
	  struct_cluster->S_cluster_bb_lat = surf_parse_get_double(A_surfxml_cluster_bb_lat);
	if(!strcmp(A_surfxml_cluster_router_id,""))
		struct_cluster->S_cluster_router_id = bprintf("%s%s_router%s",
				struct_cluster->V_cluster_prefix,
				struct_cluster->V_cluster_id,
				struct_cluster->V_cluster_suffix);
	else
		struct_cluster->S_cluster_router_id = xbt_strdup(A_surfxml_cluster_router_id);

	struct_cluster->V_cluster_sharing_policy = AX_surfxml_cluster_sharing_policy;
	struct_cluster->V_cluster_bb_sharing_policy = AX_surfxml_cluster_bb_sharing_policy;

	struct_cluster->V_cluster_availability_file = xbt_strdup(A_surfxml_cluster_availability_file);
	struct_cluster->V_cluster_state_file = xbt_strdup(A_surfxml_cluster_state_file);

	surfxml_call_cb_functions(STag_surfxml_cluster_cb_list);
}
void ETag_surfxml_cluster(void){
	surfxml_call_cb_functions(ETag_surfxml_cluster_cb_list);
	xbt_free(struct_cluster->V_cluster_id);
	xbt_free(struct_cluster->V_cluster_prefix);
	xbt_free(struct_cluster->V_cluster_suffix);
	xbt_free(struct_cluster->V_cluster_radical);
	xbt_free(struct_cluster->S_cluster_router_id);
	xbt_free(struct_cluster->V_cluster_availability_file);
	xbt_free(struct_cluster->V_cluster_state_file);
	xbt_free(struct_cluster);
}

void STag_surfxml_peer(void){
	struct_peer = xbt_new0(s_surf_parsing_peer_arg_t, 1);
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
	struct_lnk = xbt_new0(s_surf_parsing_link_arg_t, 1);
	struct_lnk->V_link_id = xbt_strdup(A_surfxml_link_id);
	struct_lnk->V_link_bandwidth = surf_parse_get_double(A_surfxml_link_bandwidth);
	struct_lnk->V_link_bandwidth_file = tmgr_trace_new(A_surfxml_link_bandwidth_file);
	struct_lnk->V_link_latency = surf_parse_get_double(A_surfxml_link_latency);
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

	surf_parse_link();
}
void surf_parse_link(void){
	surfxml_call_cb_functions(STag_surfxml_link_cb_list);
}
void ETag_surfxml_link(void){
	surfxml_call_cb_functions(ETag_surfxml_link_cb_list);
	xbt_free(struct_lnk->V_link_id);
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
  XBT_DEBUG("START configuration name = %s",A_surfxml_config_id);
  xbt_assert(current_property_set == NULL, "Someone forgot to reset the property set to NULL in its closing tag (or XML malformed)");
  current_property_set = xbt_dict_new();

}
void ETag_surfxml_config(void){
  xbt_dict_cursor_t cursor = NULL;
  char *key;
  char *elem;
  char *cfg;
  xbt_dict_foreach(current_property_set, cursor, key, elem) {
    cfg = bprintf("%s:%s",key,elem);
    if(xbt_cfg_is_default_value(_surf_cfg_set, key))
      xbt_cfg_set_parse(_surf_cfg_set, cfg);
    else
      XBT_INFO("The custom configuration '%s' is already defined by user!",key);
    free(cfg);
  }
  XBT_DEBUG("End configuration name = %s",A_surfxml_config_id);
  current_property_set = NULL;
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

/* Open and Close parse file */

void surf_parse_open(const char *file)
{
  static int warned = 0;        /* warn only once */
  if (!file) {
    if (!warned) {
      XBT_WARN
          ("Bypassing the XML parser since surf_parse_open received a NULL pointer. "
              "If it is not what you want, go fix your code.");
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

/* Call the lexer to parse the currently opened file. This pointer to function enables bypassing of the parser */

static int _surf_parse(void) {
  return surf_parse_lex();
}
int_f_void_t surf_parse = _surf_parse;


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


/* Prop tag functions */

void parse_properties(const char* prop_id, const char* prop_value)
{
    char *value = NULL;
	if (!current_property_set)
	    current_property_set = xbt_dict_new();      // Maybe, it should raise an error
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
    power_scale = surf_parse_get_double(power);
  }
  return power_scale;
}

double random_min, random_max, random_mean, random_std_deviation,
    random_generator;
char *random_id;

static void init_randomness(void)
{
  random_id = A_surfxml_random_id;
  random_min = surf_parse_get_double(A_surfxml_random_min);
  random_max = surf_parse_get_double(A_surfxml_random_max);
  random_mean = surf_parse_get_double(A_surfxml_random_mean);
  random_std_deviation = surf_parse_get_double(A_surfxml_random_std_deviation);
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
