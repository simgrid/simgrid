/* Copyright (c) 2006, 2007, 2008, 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <errno.h>
#include <math.h>
#include <stdarg.h> /* va_arg */

#include "xbt/misc.h"
#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/dict.h"
#include "surf/surfxml_parse.h"
#include "surf/surf_private.h"
#include "simgrid/sg_config.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_parse, surf,
                                "Logging specific to the SURF parsing module");
#undef CLEANUP
int ETag_surfxml_include_state(void);

#include "simgrid_dtd.c"

char* surf_parsed_filename = NULL; // to locate parse error messages

xbt_dynar_t parsed_link_list = NULL;   /* temporary store of current list link of a route */
extern AS_t current_routing;
/*
 * Helping functions
 */
void surf_parse_error(const char *fmt, ...) {
  va_list va;
  va_start(va,fmt);
  char *msg = bvprintf(fmt,va);
  va_end(va);
  xbt_die("Parse error at %s:%d: %s", surf_parsed_filename, surf_parse_lineno, msg);
}
void surf_parse_warn(const char *fmt, ...) {
  va_list va;
  va_start(va,fmt);
  char *msg = bvprintf(fmt,va);
  va_end(va);
    XBT_WARN("%s:%d: %s", surf_parsed_filename, surf_parse_lineno, msg);
    free(msg);
}

double surf_parse_get_double(const char *string) {
  double res;
  int ret = sscanf(string, "%lg", &res);
  if (ret != 1)
    surf_parse_error("%s is not a double", string);
  //printf("Parsed double [%lg] %s\n", res, string);  
  return res;
}

int surf_parse_get_int(const char *string) {
  int res;
  int ret = sscanf(string, "%d", &res);
  if (ret != 1)
    surf_parse_error("%s is not an integer", string);
  return res;
}

struct unit_scale {
  const char *unit;
  double scale;
};

/* Note: field `unit' for the last element of parametre `units' should be
 * NULL. */
static double surf_parse_get_value_with_unit(const char *string,
                                             const struct unit_scale *units)
{
  char* ptr;
  double res;
  int i;
  errno = 0;
  res = strtod(string, &ptr);
  if (errno == ERANGE)
    surf_parse_error("value out of range: %s", string);
  if (ptr == string)
    surf_parse_error("cannot parse number: %s", string);
  for (i = 0; units[i].unit != NULL && strcmp(ptr, units[i].unit) != 0; i++) {
  }
  if (units[i].unit != NULL)
    res *= units[i].scale;
  else
    surf_parse_error("unknown unit: %s", ptr);
  return res;
}

double surf_parse_get_time(const char *string)
{
  const struct unit_scale units[] = {
    { "w",  7 * 24 * 60 * 60 },
    { "d",  24 * 60 * 60 },
    { "h",  60 * 60 },
    { "m",  60 },
    { "s",  1.0 },
    { "",   1.0 },              /* default unit is seconds */
    { "ms", 1e-3 },
    { "us", 1e-6 },
    { "ns", 1e-9 },
    { "ps", 1e-12 },
    { NULL, 0 }
  };
  return surf_parse_get_value_with_unit(string, units);
}

double surf_parse_get_bandwidth(const char *string)
{
  const struct unit_scale units[] = {
    { "TBps",  1e12 },
    { "GBps",  1e9 },
    { "MBps",  1e6 },
    { "KBps",  1e3 },
    { "Bps",   1.0 },
    { "",      1.0 },           /* default unit is bytes ber second */
    { "tbps",  0.125 * 1e12 },
    { "gbps",  0.125 * 1e9 },
    { "mbps",  0.125 * 1e6 },
    { "kbps",  0.125 * 1e3 },
    { "bps",   0.125 },
    { NULL,    0 }
  };
  return surf_parse_get_value_with_unit(string, units);
}

double surf_parse_get_power(const char *string)
{
  const struct unit_scale units[] = {
    { "yottaflops", 1e24 },
    { "yf",         1e24 },
    { "zettaflops", 1e21 },
    { "zf",         1e21 },
    { "exaflops",   1e18 },
    { "ef",         1e18 },
    { "petaflops",  1e15 },
    { "pf",         1e15 },
    { "teraflops",  1e12 },
    { "tf",         1e12 },
    { "gigaflops",  1e9 },
    { "gf",         1e9 },
    { "megaflops",  1e6 },
    { "mf",         1e6 },
    { "kiloflops",  1e3 },
    { "kf",         1e3 },
    { "flops",      1.0 },
    { "f",          1.0 },
    { "",           1.0 },      /* default unit is flops */
    { NULL,         0 }
  };
  return surf_parse_get_value_with_unit(string, units);
}

/*
 * All the callback lists that can be overridden anywhere.
 * (this list should probably be reduced to the bare minimum to allow the models to work)
 */

/* make sure these symbols are defined as strong ones in this file so that the linker can resolve them */

/* The default current property receiver. Setup in the corresponding opening callbacks. */
xbt_dict_t current_property_set = NULL;
xbt_dict_t as_current_property_set = NULL;
int AS_TAG = 0;
char* as_name_tab[1024];
void* as_dict_tab[1024];
int as_prop_nb = 0;


/* dictionary of random generator data */
xbt_dict_t random_data_list = NULL;

YY_BUFFER_STATE surf_input_buffer;
FILE *surf_file_to_parse = NULL;

static void init_randomness(void);
static void add_randomness(void);

/*
 * Stuff relative to storage
 */
void STag_surfxml_storage(void)
{
  AS_TAG = 0;
  XBT_DEBUG("STag_surfxml_storage");
  xbt_assert(current_property_set == NULL, "Someone forgot to reset the property set to NULL in its closing tag (or XML malformed)");
}
void ETag_surfxml_storage(void)
{
  s_sg_platf_storage_cbarg_t storage;
  memset(&storage,0,sizeof(storage));

  storage.id = A_surfxml_storage_id;
  storage.type_id = A_surfxml_storage_typeId;
  storage.content = A_surfxml_storage_content;
  storage.properties = current_property_set;
  sg_platf_new_storage(&storage);
  current_property_set = NULL;
}
void STag_surfxml_storage___type(void)
{
  AS_TAG = 0;
  XBT_DEBUG("STag_surfxml_storage___type");
  xbt_assert(current_property_set == NULL, "Someone forgot to reset the property set to NULL in its closing tag (or XML malformed)");
}
void ETag_surfxml_storage___type(void)
{
  s_sg_platf_storage_type_cbarg_t storage_type;
  memset(&storage_type,0,sizeof(storage_type));

  storage_type.content = A_surfxml_storage___type_content;
  storage_type.id = A_surfxml_storage___type_id;
  storage_type.model = A_surfxml_storage___type_model;
  storage_type.properties = current_property_set;
  storage_type.size = surf_parse_get_int(A_surfxml_storage___type_size);
  sg_platf_new_storage_type(&storage_type);
  current_property_set = NULL;
}
void STag_surfxml_mstorage(void)
{
  XBT_DEBUG("STag_surfxml_mstorage");
}
void ETag_surfxml_mstorage(void)
{
  s_sg_platf_mstorage_cbarg_t mstorage;
  memset(&mstorage,0,sizeof(mstorage));

  mstorage.name = A_surfxml_mstorage_name;
  mstorage.type_id = A_surfxml_mstorage_typeId;
  sg_platf_new_mstorage(&mstorage);
}
void STag_surfxml_mount(void)
{
  XBT_DEBUG("STag_surfxml_mount");
}
void ETag_surfxml_mount(void)
{
  s_sg_platf_mount_cbarg_t mount;
  memset(&mount,0,sizeof(mount));

  mount.name = A_surfxml_mount_name;
  mount.id = A_surfxml_mount_id;
  sg_platf_new_mount(&mount);
}

/*
 * Stuff relative to the <include> tag
 */
static xbt_dynar_t surf_input_buffer_stack = NULL;
static xbt_dynar_t surf_file_to_parse_stack = NULL;
static xbt_dynar_t surf_parsed_filename_stack = NULL;

void STag_surfxml_include(void)
{
  XBT_DEBUG("STag_surfxml_include '%s'",A_surfxml_include_file);
  xbt_dynar_push(surf_parsed_filename_stack,&surf_parsed_filename); // save old file name
  surf_parsed_filename = xbt_strdup(A_surfxml_include_file);

  xbt_dynar_push(surf_file_to_parse_stack, &surf_file_to_parse); //save old file descriptor

  surf_file_to_parse = surf_fopen(A_surfxml_include_file, "r"); // read new file descriptor
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
 * on a line. One day maybe, if the maya allow it.
 */
int ETag_surfxml_include_state(void)
{
  fflush(NULL);
  XBT_DEBUG("ETag_surfxml_include_state '%s'",A_surfxml_include_file);

  if(xbt_dynar_is_empty(surf_input_buffer_stack)) // nope, that's a true premature EOF. Let the parser die verbosely.
    return 0;

  // Yeah, we were in an <include> Restore state and proceed.
  fclose(surf_file_to_parse);
  xbt_dynar_pop(surf_file_to_parse_stack, &surf_file_to_parse);
  surf_parse_pop_buffer_state();
  xbt_dynar_pop(surf_input_buffer_stack,&surf_input_buffer);

  // Restore the filename for error messages
  free(surf_parsed_filename);
  xbt_dynar_pop(surf_parsed_filename_stack,&surf_parsed_filename);

  return 1;
}


void surf_parse_init_callbacks(void)
{
    sg_platf_init();
}

void surf_parse_reset_callbacks(void)
{
  surf_parse_free_callbacks();
  surf_parse_init_callbacks();
}

void surf_parse_free_callbacks(void)
{
  sg_platf_exit();
}

/* Stag and Etag parse functions */

void STag_surfxml_platform(void) {
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

  sg_platf_begin();
}
void ETag_surfxml_platform(void){
  sg_platf_end();
}

void STag_surfxml_host(void){
  AS_TAG = 0;
  xbt_assert(current_property_set == NULL, "Someone forgot to reset the property set to NULL in its closing tag (or XML malformed)");
}

void STag_surfxml_prop(void)
{
  if(AS_TAG){
    if (!as_current_property_set){
      xbt_assert(as_prop_nb < 1024, "Number of AS property reach the limit!!!");
      as_current_property_set = xbt_dict_new_homogeneous(xbt_free_f); // Maybe, it should raise an error
      as_name_tab[as_prop_nb] = xbt_strdup(A_surfxml_AS_id);
      as_dict_tab[as_prop_nb] = as_current_property_set;
      XBT_DEBUG("PUSH prop %p for AS '%s'",as_dict_tab[as_prop_nb],as_name_tab[as_prop_nb]);
      as_prop_nb++;
    }
    xbt_dict_set(as_current_property_set, A_surfxml_prop_id, xbt_strdup(A_surfxml_prop_value), NULL);
  }
  else{
    if (!current_property_set)
      current_property_set = xbt_dict_new_homogeneous(xbt_free_f); // Maybe, it should raise an error
    xbt_dict_set(current_property_set, A_surfxml_prop_id, xbt_strdup(A_surfxml_prop_value), NULL);
  }
}

void ETag_surfxml_host(void)    {
  s_sg_platf_host_cbarg_t host;
  memset(&host,0,sizeof(host));

  host.properties = current_property_set;

  host.id = A_surfxml_host_id;
  host.power_peak = get_cpu_power(A_surfxml_host_power);
  host.power_scale = surf_parse_get_double( A_surfxml_host_availability);
  host.core_amount = surf_parse_get_int(A_surfxml_host_core);
  host.power_trace = tmgr_trace_new_from_file(A_surfxml_host_availability___file);
  host.state_trace = tmgr_trace_new_from_file(A_surfxml_host_state___file);
  xbt_assert((A_surfxml_host_state == A_surfxml_host_state_ON) ||
        (A_surfxml_host_state == A_surfxml_host_state_OFF), "Invalid state");
  if (A_surfxml_host_state == A_surfxml_host_state_ON)
    host.initial_state = SURF_RESOURCE_ON;
  if (A_surfxml_host_state == A_surfxml_host_state_OFF)
    host.initial_state = SURF_RESOURCE_OFF;
  host.coord = A_surfxml_host_coordinates;

  sg_platf_new_host(&host);
  current_property_set = NULL;
}

void STag_surfxml_host___link(void){
  XBT_DEBUG("Create a Host_link for %s",A_surfxml_host___link_id);
  s_sg_platf_host_link_cbarg_t host_link;
  memset(&host_link,0,sizeof(host_link));

  host_link.id = A_surfxml_host___link_id;
  host_link.link_up = A_surfxml_host___link_up;
  host_link.link_down = A_surfxml_host___link_down;
  sg_platf_new_host_link(&host_link);
}

void STag_surfxml_router(void){
  s_sg_platf_router_cbarg_t router;
  memset(&router, 0, sizeof(router));

  router.id = A_surfxml_router_id;
  router.coord = A_surfxml_router_coordinates;

  sg_platf_new_router(&router);
}

void STag_surfxml_cluster(void){
  s_sg_platf_cluster_cbarg_t cluster;
  memset(&cluster,0,sizeof(cluster));
  cluster.id = A_surfxml_cluster_id;
  cluster.prefix = A_surfxml_cluster_prefix;
  cluster.suffix = A_surfxml_cluster_suffix;
  cluster.radical = A_surfxml_cluster_radical;
  cluster.power = surf_parse_get_power(A_surfxml_cluster_power);
  cluster.core_amount = surf_parse_get_int(A_surfxml_cluster_core);
  cluster.bw =   surf_parse_get_bandwidth(A_surfxml_cluster_bw);
  cluster.lat =  surf_parse_get_time(A_surfxml_cluster_lat);
  if(strcmp(A_surfxml_cluster_bb___bw,""))
    cluster.bb_bw = surf_parse_get_bandwidth(A_surfxml_cluster_bb___bw);
  if(strcmp(A_surfxml_cluster_bb___lat,""))
    cluster.bb_lat = surf_parse_get_time(A_surfxml_cluster_bb___lat);
  if(strcmp(A_surfxml_cluster_limiter___link,""))
    cluster.limiter_link = surf_parse_get_double(A_surfxml_cluster_limiter___link);
  if(strcmp(A_surfxml_cluster_loopback___bw,""))
    cluster.loopback_bw = surf_parse_get_bandwidth(A_surfxml_cluster_loopback___bw);
  if(strcmp(A_surfxml_cluster_loopback___lat,""))
    cluster.loopback_lat = surf_parse_get_time(A_surfxml_cluster_loopback___lat);
  cluster.router_id = A_surfxml_cluster_router___id;

  switch (AX_surfxml_cluster_sharing___policy) {
  case A_surfxml_cluster_sharing___policy_SHARED:
    cluster.sharing_policy = SURF_LINK_SHARED;
    break;
  case A_surfxml_cluster_sharing___policy_FULLDUPLEX:
    cluster.sharing_policy = SURF_LINK_FULLDUPLEX;
    break;
  case A_surfxml_cluster_sharing___policy_FATPIPE:
    cluster.sharing_policy = SURF_LINK_FATPIPE;
    break;
  default:
    surf_parse_error("Invalid cluster sharing policy for cluster %s",
                     cluster.id);
    break;
  }
  switch (AX_surfxml_cluster_bb___sharing___policy) {
  case A_surfxml_cluster_bb___sharing___policy_FATPIPE:
    cluster.bb_sharing_policy = SURF_LINK_FATPIPE;
    break;
  case A_surfxml_cluster_bb___sharing___policy_SHARED:
    cluster.bb_sharing_policy = SURF_LINK_SHARED;
    break;
  default:
    surf_parse_error("Invalid bb sharing policy in cluster %s",
                     cluster.id);
    break;
  }

  cluster.availability_trace = A_surfxml_cluster_availability___file;
  cluster.state_trace = A_surfxml_cluster_state___file;
  sg_platf_new_cluster(&cluster);
}

void STag_surfxml_cabinet(void){
  s_sg_platf_cabinet_cbarg_t cabinet;
  memset(&cabinet,0,sizeof(cabinet));
  cabinet.id = A_surfxml_cabinet_id;
  cabinet.prefix = A_surfxml_cabinet_prefix;
  cabinet.suffix = A_surfxml_cabinet_suffix;
  cabinet.power = surf_parse_get_power(A_surfxml_cabinet_power);
  cabinet.bw = surf_parse_get_bandwidth(A_surfxml_cabinet_bw);
  cabinet.lat = surf_parse_get_time(A_surfxml_cabinet_lat);
  cabinet.radical = A_surfxml_cabinet_radical;

  sg_platf_new_cabinet(&cabinet);
}

void STag_surfxml_peer(void){
  s_sg_platf_peer_cbarg_t peer;
  memset(&peer,0,sizeof(peer));
  peer.id = A_surfxml_peer_id;
  peer.power = surf_parse_get_power(A_surfxml_peer_power);
  peer.bw_in = surf_parse_get_bandwidth(A_surfxml_peer_bw___in);
  peer.bw_out = surf_parse_get_bandwidth(A_surfxml_peer_bw___out);
  peer.lat = surf_parse_get_time(A_surfxml_peer_lat);
  peer.coord = A_surfxml_peer_coordinates;
  peer.availability_trace = tmgr_trace_new_from_file(A_surfxml_peer_availability___file);
  peer.state_trace = tmgr_trace_new_from_file(A_surfxml_peer_state___file);

  sg_platf_new_peer(&peer);
}

void STag_surfxml_link(void){
  AS_TAG = 0;
  xbt_assert(current_property_set == NULL, "Someone forgot to reset the property set to NULL in its closing tag (or XML malformed)");
}

void ETag_surfxml_link(void){
  s_sg_platf_link_cbarg_t link;
  memset(&link,0,sizeof(link));

  link.properties = current_property_set;

  link.id = A_surfxml_link_id;
  link.bandwidth = surf_parse_get_bandwidth(A_surfxml_link_bandwidth);
  //printf("Link bandwidth [%lg]\n", link.bandwidth);  
  link.bandwidth_trace = tmgr_trace_new_from_file(A_surfxml_link_bandwidth___file);
  link.latency = surf_parse_get_time(A_surfxml_link_latency);
  //printf("Link latency [%lg]\n", link.latency);  
  link.latency_trace = tmgr_trace_new_from_file(A_surfxml_link_latency___file);

  switch (A_surfxml_link_state) {
  case A_surfxml_link_state_ON:
    link.state = SURF_RESOURCE_ON;
    break;
  case A_surfxml_link_state_OFF:
    link.state = SURF_RESOURCE_OFF;
    break;
  default:
    surf_parse_error("invalid state for link %s", link.id);
    break;
  }
  link.state_trace = tmgr_trace_new_from_file(A_surfxml_link_state___file);

  switch (A_surfxml_link_sharing___policy) {
  case A_surfxml_link_sharing___policy_SHARED:
    link.policy = SURF_LINK_SHARED;
    break;
  case A_surfxml_link_sharing___policy_FATPIPE:
     link.policy = SURF_LINK_FATPIPE;
     break;
  case A_surfxml_link_sharing___policy_FULLDUPLEX:
     link.policy = SURF_LINK_FULLDUPLEX;
     break;
  default:
    surf_parse_error("Invalid sharing policy in link %s", link.id);
    break;
  }

  sg_platf_new_link(&link);

  current_property_set = NULL;
}

void STag_surfxml_link___ctn(void){

  char *link_id;
  switch (A_surfxml_link___ctn_direction) {
  case AU_surfxml_link___ctn_direction:
  case A_surfxml_link___ctn_direction_NONE:
    link_id = xbt_strdup(A_surfxml_link___ctn_id);
    break;
  case A_surfxml_link___ctn_direction_UP:
    link_id = bprintf("%s_UP", A_surfxml_link___ctn_id);
    break;
  case A_surfxml_link___ctn_direction_DOWN:
    link_id = bprintf("%s_DOWN", A_surfxml_link___ctn_id);
    break;
  }

  // FIXME we should push the surf link object but it don't
  // work because of model rulebased
  xbt_dynar_push(parsed_link_list, &link_id);
}

void ETag_surfxml_backbone(void){
  s_sg_platf_link_cbarg_t link;
  memset(&link,0,sizeof(link));

  link.properties = NULL;

  link.id = A_surfxml_backbone_id;
  link.bandwidth = surf_parse_get_bandwidth(A_surfxml_backbone_bandwidth);
  link.latency = surf_parse_get_time(A_surfxml_backbone_latency);
  link.state = SURF_RESOURCE_ON;
  link.policy = SURF_LINK_SHARED;

  sg_platf_new_link(&link);
  routing_cluster_add_backbone(xbt_lib_get_or_null(link_lib, A_surfxml_backbone_id, SURF_LINK_LEVEL));
}

void STag_surfxml_route(void){
  xbt_assert(strlen(A_surfxml_route_src) > 0 || strlen(A_surfxml_route_dst) > 0,
      "Missing end-points while defining route \"%s\"->\"%s\"",
      A_surfxml_route_src, A_surfxml_route_dst);
  parsed_link_list = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
}

void STag_surfxml_ASroute(void){
  xbt_assert(strlen(A_surfxml_ASroute_src) > 0
             || strlen(A_surfxml_ASroute_dst) > 0
             || strlen(A_surfxml_ASroute_gw___src) > 0
             || strlen(A_surfxml_ASroute_gw___dst) > 0,
             "Missing end-points while defining route \"%s\"->\"%s\" (with %s and %s as gateways)",
             A_surfxml_ASroute_src, A_surfxml_ASroute_dst,
             A_surfxml_ASroute_gw___src, A_surfxml_ASroute_gw___dst);
  parsed_link_list = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
}

void STag_surfxml_bypassRoute(void){
  xbt_assert(strlen(A_surfxml_bypassRoute_src) > 0
             || strlen(A_surfxml_bypassRoute_dst) > 0,
             "Missing end-points while defining bupass route \"%s\"->\"%s\"",
             A_surfxml_bypassRoute_src, A_surfxml_bypassRoute_dst);
  parsed_link_list = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
}

void STag_surfxml_bypassASroute(void){
  xbt_assert(strlen(A_surfxml_bypassASroute_src) > 0
             || strlen(A_surfxml_bypassASroute_dst) > 0
             || strlen(A_surfxml_bypassASroute_gw___src) > 0
             || strlen(A_surfxml_bypassASroute_gw___dst) > 0,
             "Missing end-points while defining route \"%s\"->\"%s\" (with %s and %s as gateways)",
             A_surfxml_bypassASroute_src, A_surfxml_bypassASroute_dst,
             A_surfxml_bypassASroute_gw___src,A_surfxml_bypassASroute_gw___dst);
  parsed_link_list = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
}

void ETag_surfxml_route(void){
  s_sg_platf_route_cbarg_t route;
  memset(&route,0,sizeof(route));

  route.src = A_surfxml_route_src;
  route.dst = A_surfxml_route_dst;
  route.gw_src = NULL;
  route.gw_dst = NULL;
  route.link_list = parsed_link_list;

  switch (A_surfxml_route_symmetrical) {
  case AU_surfxml_route_symmetrical:
  case A_surfxml_route_symmetrical_YES:
    route.symmetrical = TRUE;
    break;
  case A_surfxml_route_symmetrical_NO:
    route.symmetrical = FALSE;;
    break;
  }

  sg_platf_new_route(&route);
  parsed_link_list = NULL;
}

void ETag_surfxml_ASroute(void){
  s_sg_platf_route_cbarg_t ASroute;
  memset(&ASroute,0,sizeof(ASroute));

  ASroute.src = A_surfxml_ASroute_src;
  ASroute.dst = A_surfxml_ASroute_dst;

  if (!strcmp(current_routing->model_desc->name,"RuleBased")) {
    // DIRTY PERL HACK AHEAD: with the rulebased routing, the {src,dst}_gateway fields
    // store the provided name instead of the entity directly (model_rulebased_parse_ASroute knows)
    //
    // This is because the user will provide something like "^AS_(.*)$" instead of the proper name of a given entity
    ASroute.gw_src = (sg_routing_edge_t) A_surfxml_ASroute_gw___src;
    ASroute.gw_dst = (sg_routing_edge_t) A_surfxml_ASroute_gw___dst;
  } else {
    ASroute.gw_src = sg_routing_edge_by_name_or_null(A_surfxml_ASroute_gw___src);
    ASroute.gw_dst = sg_routing_edge_by_name_or_null(A_surfxml_ASroute_gw___dst);
  }

  ASroute.link_list = parsed_link_list;

  switch (A_surfxml_ASroute_symmetrical) {
  case AU_surfxml_ASroute_symmetrical:
  case A_surfxml_ASroute_symmetrical_YES:
    ASroute.symmetrical = TRUE;
    break;
  case A_surfxml_ASroute_symmetrical_NO:
    ASroute.symmetrical = FALSE;
    break;
  }

  sg_platf_new_ASroute(&ASroute);
  parsed_link_list = NULL;
}

void ETag_surfxml_bypassRoute(void){
  s_sg_platf_route_cbarg_t route;
  memset(&route,0,sizeof(route));

  route.src = A_surfxml_bypassRoute_src;
  route.dst = A_surfxml_bypassRoute_dst;
  route.gw_src = NULL;
  route.gw_dst = NULL;
  route.link_list = parsed_link_list;
  route.symmetrical = FALSE;

  sg_platf_new_bypassRoute(&route);
  parsed_link_list = NULL;
}

void ETag_surfxml_bypassASroute(void){
  s_sg_platf_route_cbarg_t ASroute;
  memset(&ASroute,0,sizeof(ASroute));

  ASroute.src = A_surfxml_bypassASroute_src;
  ASroute.dst = A_surfxml_bypassASroute_dst;
  ASroute.link_list = parsed_link_list;
  ASroute.symmetrical = FALSE;

  if (!strcmp(current_routing->model_desc->name,"RuleBased")) {
    // DIRTY PERL HACK AHEAD: with the rulebased routing, the {src,dst}_gateway fields
    // store the provided name instead of the entity directly (model_rulebased_parse_ASroute knows)
    //
    // This is because the user will provide something like "^AS_(.*)$" instead of the proper name of a given entity
    ASroute.gw_src = (sg_routing_edge_t) A_surfxml_bypassASroute_gw___src;
    ASroute.gw_dst = (sg_routing_edge_t) A_surfxml_bypassASroute_gw___dst;
  } else {
    ASroute.gw_src = sg_routing_edge_by_name_or_null(A_surfxml_bypassASroute_gw___src);
    ASroute.gw_dst = sg_routing_edge_by_name_or_null(A_surfxml_bypassASroute_gw___dst);
  }

  sg_platf_new_bypassASroute(&ASroute);
  parsed_link_list = NULL;
}

void ETag_surfxml_trace(void){
  s_sg_platf_trace_cbarg_t trace;
  memset(&trace,0,sizeof(trace));

  trace.id = A_surfxml_trace_id;
  trace.file = A_surfxml_trace_file;
  trace.periodicity = surf_parse_get_double(A_surfxml_trace_periodicity);
  trace.pc_data = surfxml_pcdata;

  sg_platf_new_trace(&trace);
}

void STag_surfxml_trace___connect(void){
  s_sg_platf_trace_connect_cbarg_t trace_connect;
  memset(&trace_connect,0,sizeof(trace_connect));

  trace_connect.element = A_surfxml_trace___connect_element;
  trace_connect.trace = A_surfxml_trace___connect_trace;

  switch (A_surfxml_trace___connect_kind) {
  case AU_surfxml_trace___connect_kind:
  case A_surfxml_trace___connect_kind_POWER:
    trace_connect.kind =  SURF_TRACE_CONNECT_KIND_POWER;
    break;
  case A_surfxml_trace___connect_kind_BANDWIDTH:
    trace_connect.kind =  SURF_TRACE_CONNECT_KIND_BANDWIDTH;
    break;
  case A_surfxml_trace___connect_kind_HOST___AVAIL:
    trace_connect.kind =  SURF_TRACE_CONNECT_KIND_HOST_AVAIL;
    break;
  case A_surfxml_trace___connect_kind_LATENCY:
    trace_connect.kind =  SURF_TRACE_CONNECT_KIND_LATENCY;
    break;
  case A_surfxml_trace___connect_kind_LINK___AVAIL:
    trace_connect.kind =  SURF_TRACE_CONNECT_KIND_LINK_AVAIL;
    break;
  }
  sg_platf_trace_connect(&trace_connect);
}

void STag_surfxml_AS(void){
  AS_TAG = 1;
  s_sg_platf_AS_cbarg_t AS = SG_PLATF_AS_INITIALIZER;
  AS.id = A_surfxml_AS_id;
  AS.routing = (int)A_surfxml_AS_routing;

  as_current_property_set = NULL;

  sg_platf_new_AS_begin(&AS);
}
void ETag_surfxml_AS(void){
  if(as_prop_nb){
    char *name = as_name_tab[as_prop_nb-1];
    xbt_dict_t dict = as_dict_tab[as_prop_nb-1];
    as_prop_nb--;
    XBT_DEBUG("POP prop %p for AS '%s'",dict,name);
    xbt_lib_set(as_router_lib,
        name,
      ROUTING_PROP_ASR_LEVEL,
      dict);
    xbt_free(name);
  }
  sg_platf_new_AS_end();
}

extern int _sg_init_status; /* FIXME: find a proper way to export this at some point */

void STag_surfxml_config(void){
  AS_TAG = 0;
  xbt_assert(current_property_set == NULL, "Someone forgot to reset the property set to NULL in its closing tag (or XML malformed)");
  XBT_DEBUG("START configuration name = %s",A_surfxml_config_id);
  if (_sg_init_status == 2) {
    surf_parse_error("All <config> tags must be given before any platform elements (such as <AS>, <host>, <cluster>, <link>, etc).");
  }
}
void ETag_surfxml_config(void){
  xbt_dict_cursor_t cursor = NULL;
  char *key;
  char *elem;
  char *cfg;
  xbt_dict_foreach(current_property_set, cursor, key, elem) {
    cfg = bprintf("%s:%s",key,elem);
    if(xbt_cfg_is_default_value(_sg_cfg_set, key))
      xbt_cfg_set_parse(_sg_cfg_set, cfg);
    else
      XBT_INFO("The custom configuration '%s' is already defined by user!",key);
    free(cfg);
  }
  XBT_DEBUG("End configuration name = %s",A_surfxml_config_id);
  xbt_dict_free(&current_property_set);
  current_property_set = NULL;
}

static int argc;
static char **argv;

void STag_surfxml_process(void){
  AS_TAG = 0;
  argc = 1;
  argv = xbt_new(char *, 1);
  argv[0] = xbt_strdup(A_surfxml_process_function);
  xbt_assert(current_property_set == NULL, "Someone forgot to reset the property set to NULL in its closing tag (or XML malformed)");
}

void ETag_surfxml_process(void){
  s_sg_platf_process_cbarg_t process;
  memset(&process,0,sizeof(process));

  process.argc = argc;
  process.argv = (const char **)argv;
  process.properties = current_property_set;
  process.host = A_surfxml_process_host;
  process.function = A_surfxml_process_function;
  process.start_time = surf_parse_get_double(A_surfxml_process_start___time);
  process.kill_time = surf_parse_get_double(A_surfxml_process_kill___time);

  switch (A_surfxml_process_on___failure) {
  case AU_surfxml_process_on___failure:
  case A_surfxml_process_on___failure_DIE:
    process.on_failure =  SURF_PROCESS_ON_FAILURE_DIE;
    break;
  case A_surfxml_process_on___failure_RESTART:
    process.on_failure =  SURF_PROCESS_ON_FAILURE_RESTART;
    break;
  }

  sg_platf_new_process(&process);
  current_property_set = NULL;
}

void STag_surfxml_argument(void){
  argc++;
  argv = xbt_realloc(argv, (argc) * sizeof(char *));
  argv[(argc) - 1] = xbt_strdup(A_surfxml_argument_value);
}

/* ***************************************** */
/* TUTORIAL: New TAG                         */
void STag_surfxml_gpu(void)
{
  XBT_DEBUG("STag_surfxml_gpu");
}
void ETag_surfxml_gpu(void)
{
  s_sg_platf_gpu_cbarg_t gpu;
  memset(&gpu,0,sizeof(gpu));

  gpu.name = A_surfxml_gpu_name;

  sg_platf_new_gpu(&gpu);
}
/* ***************************************** */

/* nothing to do in those functions */
void ETag_surfxml_prop(void){}
void STag_surfxml_random(void){}
void ETag_surfxml_random(void){}
void ETag_surfxml_trace___connect(void){}
void STag_surfxml_trace(void){}
void ETag_surfxml_router(void){}
void ETag_surfxml_host___link(void){}
void ETag_surfxml_cluster(void){}
void ETag_surfxml_cabinet(void){}
void ETag_surfxml_peer(void){}
void STag_surfxml_backbone(void){}
void ETag_surfxml_link___ctn(void){}
void ETag_surfxml_argument(void){}

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

  if (!surf_parsed_filename_stack)
    surf_parsed_filename_stack = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
  surf_parsed_filename = xbt_strdup(file);

  surf_file_to_parse = surf_fopen(file, "r");
  xbt_assert((surf_file_to_parse), "Unable to open \"%s\"\n", file);
  surf_input_buffer = surf_parse__create_buffer(surf_file_to_parse, YY_BUF_SIZE);
  surf_parse__switch_to_buffer(surf_input_buffer);
  surf_parse_lineno = 1;
}

void surf_parse_close(void)
{
  xbt_dynar_free(&surf_input_buffer_stack);
  xbt_dynar_free(&surf_file_to_parse_stack);
  xbt_dynar_free(&surf_parsed_filename_stack);

  free(surf_parsed_filename);
  surf_parsed_filename = NULL;

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


/* Prop tag functions */

/**
 * With XML parser
 */

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
    power_scale = surf_parse_get_power(power);
  }
  return power_scale;
}

double random_min, random_max, random_mean, random_std_deviation;
e_random_generator_t random_generator;
char *random_id;

static void init_randomness(void)
{
  random_id = A_surfxml_random_id;
  random_min = surf_parse_get_double(A_surfxml_random_min);
  random_max = surf_parse_get_double(A_surfxml_random_max);
  random_mean = surf_parse_get_double(A_surfxml_random_mean);
  random_std_deviation = surf_parse_get_double(A_surfxml_random_std___deviation);
  switch (A_surfxml_random_generator) {
  case AU_surfxml_random_generator:
  case A_surfxml_random_generator_NONE:
    random_generator = NONE;
    break;
  case A_surfxml_random_generator_DRAND48:
    random_generator = DRAND48;
    break;
  case A_surfxml_random_generator_RAND:
    random_generator = RAND;
    break;
  case A_surfxml_random_generator_RNGSTREAM:
    random_generator = RNGSTREAM;
    break;
  default:
    surf_parse_error("Invalid random generator");
    break;
  }
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


xbt_dict_t get_as_router_properties(const char* name)
{
  return xbt_lib_get_or_null(as_router_lib, name, ROUTING_PROP_ASR_LEVEL);
}

