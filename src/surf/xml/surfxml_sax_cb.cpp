/* Copyright (c) 2006-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <errno.h>
#include <math.h>
#include <stdarg.h> /* va_arg */

#include "xbt/misc.h"
#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/file.h"
#include "xbt/dict.h"
#include "src/surf/surf_private.h"
#include "src/surf/network_interface.hpp"
#include "simgrid/sg_config.h"
#include "simgrid/link.h"

#include "src/surf/xml/platf_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_parse, surf, "Logging specific to the SURF parsing module");
#undef CLEANUP
int ETag_surfxml_include_state(void);

#include "simgrid_dtd.c"

char* surf_parsed_filename = nullptr; // to locate parse error messages

xbt_dynar_t parsed_link_list = nullptr;   /* temporary store of current list link of a route */
/*
 * Helping functions
 */
void surf_parse_assert(bool cond, const char *fmt, ...) {
  if (!cond ) {
    va_list va;
    va_start(va,fmt);
    int lineno = surf_parse_lineno;
    char *msg = bvprintf(fmt,va);
    va_end(va);
    cleanup();
    XBT_ERROR("Parse error at %s:%d: %s", surf_parsed_filename, lineno, msg);
    surf_exit();
    xbt_die("Exiting now");
  }
}
void surf_parse_error(const char *fmt, ...) {
  va_list va;
  va_start(va,fmt);
  int lineno = surf_parse_lineno;
  char *msg = bvprintf(fmt,va);
  va_end(va);
  cleanup();
  XBT_ERROR("Parse error at %s:%d: %s", surf_parsed_filename, lineno, msg);
  surf_exit();
  xbt_die("Exiting now");
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

/* Note: field `unit' for the last element of parameter `units' should be nullptr. */
static double surf_parse_get_value_with_unit(const char *string, const struct unit_scale *units,
    const char *entity_kind, const char *name,
    const char *error_msg, const char *default_unit)
{
  char* ptr;
  double res;
  int i;
  errno = 0;
  res   = strtod(string, &ptr);
  if (errno == ERANGE)
    surf_parse_error("value out of range: %s", string);
  if (ptr == string)
    surf_parse_error("cannot parse number: %s", string);
  if (ptr[0] == '\0') {
    if (res == 0)
      return res; // Ok, 0 can be unit-less

    XBT_WARN("Deprecated unit-less value '%s' for %s %s. %s",string, entity_kind, name, error_msg);
    ptr = (char*)default_unit;
  }
  for (i = 0; units[i].unit != nullptr && strcmp(ptr, units[i].unit) != 0; i++) {
  }
  if (units[i].unit != nullptr)
    res *= units[i].scale;
  else
    surf_parse_error("unknown unit: %s", ptr);
  return res;
}

double surf_parse_get_time(const char *string, const char *entity_kind, const char *name)
{
  const struct unit_scale units[] = {
    { "w",  7 * 24 * 60 * 60 },
    { "d",  24 * 60 * 60 },
    { "h",  60 * 60 },
    { "m",  60 },
    { "s",  1.0 },
    { "ms", 1e-3 },
    { "us", 1e-6 },
    { "ns", 1e-9 },
    { "ps", 1e-12 },
    { nullptr, 0 }
  };
  return surf_parse_get_value_with_unit(string, units, entity_kind, name,
      "Append 's' to your time to get seconds", "s");
}

double surf_parse_get_size(const char *string, const char *entity_kind, const char *name)
{
  const struct unit_scale units[] = {
    { "TiB", pow(1024, 4) },
    { "GiB", pow(1024, 3) },
    { "MiB", pow(1024, 2) },
    { "KiB", 1024 },
    { "TB",  1e12 },
    { "GB",  1e9 },
    { "MB",  1e6 },
    { "kB",  1e3 },
    { "B",   1.0 },
    { "Tib", 0.125 * pow(1024, 4) },
    { "Gib", 0.125 * pow(1024, 3) },
    { "Mib", 0.125 * pow(1024, 2) },
    { "Kib", 0.125 * 1024 },
    { "Tb",  0.125 * 1e12 },
    { "Gb",  0.125 * 1e9 },
    { "Mb",  0.125 * 1e6 },
    { "kb",  0.125 * 1e3 },
    { "b",   0.125 },
    { nullptr,    0 }
  };
  return surf_parse_get_value_with_unit(string, units, entity_kind, name,
      "Append 'B' to get bytes (or 'b' for bits but 1B = 8b).", "B");
}

double surf_parse_get_bandwidth(const char *string, const char *entity_kind, const char *name)
{
  const struct unit_scale units[] = {
    { "TiBps", pow(1024, 4) },
    { "GiBps", pow(1024, 3) },
    { "MiBps", pow(1024, 2) },
    { "KiBps", 1024 },
    { "TBps",  1e12 },
    { "GBps",  1e9 },
    { "MBps",  1e6 },
    { "kBps",  1e3 },
    { "Bps",   1.0 },
    { "Tibps", 0.125 * pow(1024, 4) },
    { "Gibps", 0.125 * pow(1024, 3) },
    { "Mibps", 0.125 * pow(1024, 2) },
    { "Kibps", 0.125 * 1024 },
    { "Tbps",  0.125 * 1e12 },
    { "Gbps",  0.125 * 1e9 },
    { "Mbps",  0.125 * 1e6 },
    { "kbps",  0.125 * 1e3 },
    { "bps",   0.125 },
    { nullptr,    0 }
  };
  return surf_parse_get_value_with_unit(string, units, entity_kind, name,
      "Append 'Bps' to get bytes per second (or 'bps' for bits but 1Bps = 8bps)", "Bps");
}

double surf_parse_get_speed(const char *string, const char *entity_kind, const char *name)
{
  const struct unit_scale units[] = {
    { "yottaflops", 1e24 },
    { "Yf",         1e24 },
    { "zettaflops", 1e21 },
    { "Zf",         1e21 },
    { "exaflops",   1e18 },
    { "Ef",         1e18 },
    { "petaflops",  1e15 },
    { "Pf",         1e15 },
    { "teraflops",  1e12 },
    { "Tf",         1e12 },
    { "gigaflops",  1e9 },
    { "Gf",         1e9 },
    { "megaflops",  1e6 },
    { "Mf",         1e6 },
    { "kiloflops",  1e3 },
    { "kf",         1e3 },
    { "flops",      1.0 },
    { "f",          1.0 },
    { nullptr,         0 }
  };
  return surf_parse_get_value_with_unit(string, units, entity_kind, name,
      "Append 'f' or 'flops' to your speed to get flop per second", "f");
}

/*
 * All the callback lists that can be overridden anywhere.
 * (this list should probably be reduced to the bare minimum to allow the models to work)
 */

/* make sure these symbols are defined as strong ones in this file so that the linker can resolve them */

/* The default current property receiver. Setup in the corresponding opening callbacks. */
xbt_dict_t current_property_set = nullptr;
xbt_dict_t current_model_property_set = nullptr;
xbt_dict_t as_current_property_set = nullptr;
int AS_TAG = 0;
char* as_name_tab[1024];
void* as_dict_tab[1024];
int as_prop_nb = 0;


/* dictionary of random generator data */
xbt_dict_t random_data_list = nullptr;

YY_BUFFER_STATE surf_input_buffer;
FILE *surf_file_to_parse = nullptr;

/*
 * Stuff relative to storage
 */
void STag_surfxml_storage(void)
{
  AS_TAG = 0;
  XBT_DEBUG("STag_surfxml_storage");
  xbt_assert(current_property_set == nullptr, "Someone forgot to reset the property set to nullptr in its closing tag (or XML malformed)");
}
void ETag_surfxml_storage(void)
{
  s_sg_platf_storage_cbarg_t storage = SG_PLATF_STORAGE_INITIALIZER;
  memset(&storage,0,sizeof(storage));

  storage.id           = A_surfxml_storage_id;
  storage.type_id      = A_surfxml_storage_typeId;
  storage.content      = A_surfxml_storage_content;
  storage.content_type = A_surfxml_storage_content___type;
  storage.properties   = current_property_set;
  storage.attach       = A_surfxml_storage_attach;
  sg_platf_new_storage(&storage);
  current_property_set = nullptr;
}
void STag_surfxml_storage___type(void)
{
  AS_TAG = 0;
  XBT_DEBUG("STag_surfxml_storage___type");
  xbt_assert(current_property_set == nullptr, "Someone forgot to reset the property set to nullptr in its closing tag (or XML malformed)");
  xbt_assert(current_model_property_set == nullptr, "Someone forgot to reset the model property set to nullptr in its closing tag (or XML malformed)");
}
void ETag_surfxml_storage___type(void)
{
  s_sg_platf_storage_type_cbarg_t storage_type = SG_PLATF_STORAGE_TYPE_INITIALIZER;
  memset(&storage_type,0,sizeof(storage_type));

  storage_type.content          = A_surfxml_storage___type_content;
  storage_type.content_type     = A_surfxml_storage___type_content___type;
  storage_type.id               = A_surfxml_storage___type_id;
  storage_type.model            = A_surfxml_storage___type_model;
  storage_type.properties       = current_property_set;
  storage_type.model_properties = current_model_property_set;
  storage_type.size             = surf_parse_get_size(A_surfxml_storage___type_size,
        "size of storage type", storage_type.id);
  sg_platf_new_storage_type(&storage_type);
  current_property_set       = nullptr;
  current_model_property_set = nullptr;
}
void STag_surfxml_mstorage(void)
{
  XBT_DEBUG("STag_surfxml_mstorage");
}
void ETag_surfxml_mstorage(void)
{
  s_sg_platf_mstorage_cbarg_t mstorage = SG_PLATF_MSTORAGE_INITIALIZER;
  memset(&mstorage,0,sizeof(mstorage));

  mstorage.name    = A_surfxml_mstorage_name;
  mstorage.type_id = A_surfxml_mstorage_typeId;
  sg_platf_new_mstorage(&mstorage);
}
void STag_surfxml_mount(void)
{
  XBT_DEBUG("STag_surfxml_mount");
}
void ETag_surfxml_mount(void)
{
  s_sg_platf_mount_cbarg_t mount = SG_PLATF_MOUNT_INITIALIZER;
  memset(&mount,0,sizeof(mount));

  mount.name      = A_surfxml_mount_name;
  mount.storageId = A_surfxml_mount_storageId;
  sg_platf_new_mount(&mount);
}

/*
 * Stuff relative to the <include> tag
 */
static xbt_dynar_t surf_input_buffer_stack    = nullptr;
static xbt_dynar_t surf_file_to_parse_stack   = nullptr;
static xbt_dynar_t surf_parsed_filename_stack = nullptr;

void STag_surfxml_include(void)
{
  parse_after_config();
  XBT_DEBUG("STag_surfxml_include '%s'",A_surfxml_include_file);
  xbt_dynar_push(surf_parsed_filename_stack,&surf_parsed_filename); // save old file name
  surf_parsed_filename = xbt_strdup(A_surfxml_include_file);

  xbt_dynar_push(surf_file_to_parse_stack, &surf_file_to_parse); //save old file descriptor

  surf_file_to_parse = surf_fopen(A_surfxml_include_file, "r"); // read new file descriptor
  xbt_assert((surf_file_to_parse), "Unable to open \"%s\"\n", A_surfxml_include_file);

  xbt_dynar_push(surf_input_buffer_stack,&surf_input_buffer);
  surf_input_buffer = surf_parse__create_buffer(surf_file_to_parse, YY_BUF_SIZE);
  surf_parse_push_buffer_state(surf_input_buffer);

  fflush(nullptr);
}

void ETag_surfxml_include(void) {
/* Nothing to do when done with reading the include tag.
 * Instead, the handling should be deferred until the EOF of current buffer -- see below */
}

/** @brief When reaching EOF, check whether we are in an include tag, and behave accordingly if yes
 *
 * This function is called automatically by sedding the parser in tools/cmake/MaintainerMode.cmake
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
  fflush(nullptr);
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

/* Stag and Etag parse functions */

void STag_surfxml_platform(void) {
  XBT_ATTRIB_UNUSED double version = surf_parse_get_double(A_surfxml_platform_version);

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
  xbt_assert((version >= 4.0), "******* FILE %s IS TOO OLD (v:%.1f) *********\n "
      "Changes introduced in SimGrid 3.13:\n"
      "  - 'power' attribute of hosts (and others) got renamed to 'speed'.\n"
      "  - In <trace_connect>, attribute kind=\"POWER\" is now kind=\"SPEED\".\n"
      "  - DOCTYPE now point to the rignt URL: http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\n"
      "  - speed, bandwidth and latency attributes now MUST have an explicit unit (f, Bps, s by default)"
      "\n\n"
      "Use simgrid_update_xml to update your file automatically. "
      "This program is installed automatically with SimGrid, or "
      "available in the tools/ directory of the source archive.",surf_parsed_filename, version);

  sg_platf_begin();
}
void ETag_surfxml_platform(void){
  sg_platf_end();
}

void STag_surfxml_host(void){
  AS_TAG = 0;
  xbt_assert(current_property_set == nullptr, "Someone forgot to reset the property set to nullptr in its closing tag (or XML malformed)");
}

void STag_surfxml_prop(void)
{
  if(AS_TAG){
    if (!as_current_property_set){
      xbt_assert(as_prop_nb < 1024, "Number of AS property reach the limit!!!");
      as_current_property_set = xbt_dict_new_homogeneous(xbt_free_f); // Maybe, it should raise an error
      as_name_tab[as_prop_nb] = xbt_strdup(A_surfxml_AS_id);
      as_dict_tab[as_prop_nb] = as_current_property_set;
      XBT_DEBUG("PUSH prop set %p for AS '%s'",as_dict_tab[as_prop_nb],as_name_tab[as_prop_nb]);
      as_prop_nb++;
    }
    XBT_DEBUG("add prop %s=%s into current AS property set", A_surfxml_prop_id, A_surfxml_prop_value);
    xbt_dict_set(as_current_property_set, A_surfxml_prop_id, xbt_strdup(A_surfxml_prop_value), nullptr);
  }
  else{
    if (!current_property_set)
       current_property_set = xbt_dict_new(); // Maybe, it should raise an error
    xbt_dict_set(current_property_set, A_surfxml_prop_id, xbt_strdup(A_surfxml_prop_value), xbt_free_f);
    XBT_DEBUG("add prop %s=%s into current property set", A_surfxml_prop_id, A_surfxml_prop_value);
  }
}

void ETag_surfxml_host(void)    {
  s_sg_platf_host_cbarg_t host = SG_PLATF_HOST_INITIALIZER;
  char* buf;
  memset(&host,0,sizeof(host));


  host.properties = current_property_set;

  host.id = A_surfxml_host_id;

  buf = A_surfxml_host_speed;
  XBT_DEBUG("Buffer: %s", buf);
  host.speed_peak = xbt_dynar_new(sizeof(double), nullptr);
  if (strchr(buf, ',') == nullptr){
    double speed = surf_parse_get_speed(A_surfxml_host_speed,"speed of host", host.id);
    xbt_dynar_push_as(host.speed_peak,double, speed);
  }
  else {
    xbt_dynar_t pstate_list = xbt_str_split(buf, ",");
    unsigned int i;
    for (i = 0; i < xbt_dynar_length(pstate_list); i++) {
      double speed;
      char* speed_str;

      xbt_dynar_get_cpy(pstate_list, i, &speed_str);
      xbt_str_trim(speed_str, nullptr);
      speed = surf_parse_get_speed(speed_str,"speed of host", host.id);
      xbt_dynar_push_as(host.speed_peak, double, speed);
      XBT_DEBUG("Speed value: %f", speed);
    }
    xbt_dynar_free(&pstate_list);
  }

  XBT_DEBUG("pstate: %s", A_surfxml_host_pstate);
  host.core_amount = surf_parse_get_int(A_surfxml_host_core);
  host.speed_trace = A_surfxml_host_availability___file[0] ? tmgr_trace_new_from_file(A_surfxml_host_availability___file) : NULL;
  host.state_trace = A_surfxml_host_state___file[0] ? tmgr_trace_new_from_file(A_surfxml_host_state___file) : NULL;
  host.pstate      = surf_parse_get_int(A_surfxml_host_pstate);
  host.coord       = A_surfxml_host_coordinates;

  sg_platf_new_host(&host);
  xbt_dynar_free(&host.speed_peak);
  current_property_set = nullptr;
}

void STag_surfxml_host___link(void){
  XBT_DEBUG("Create a Host_link for %s",A_surfxml_host___link_id);
  s_sg_platf_host_link_cbarg_t host_link = SG_PLATF_HOST_LINK_INITIALIZER;
  memset(&host_link,0,sizeof(host_link));

  host_link.id        = A_surfxml_host___link_id;
  host_link.link_up   = A_surfxml_host___link_up;
  host_link.link_down = A_surfxml_host___link_down;
  sg_platf_new_hostlink(&host_link);
}

void STag_surfxml_router(void){
  s_sg_platf_router_cbarg_t router = SG_PLATF_ROUTER_INITIALIZER;
  memset(&router, 0, sizeof(router));

  router.id    = A_surfxml_router_id;
  router.coord = A_surfxml_router_coordinates;

  sg_platf_new_router(&router);
}

void ETag_surfxml_cluster(void){
  s_sg_platf_cluster_cbarg_t cluster = SG_PLATF_CLUSTER_INITIALIZER;
  memset(&cluster,0,sizeof(cluster));
  cluster.properties = as_current_property_set;

  cluster.id          = A_surfxml_cluster_id;
  cluster.prefix      = A_surfxml_cluster_prefix;
  cluster.suffix      = A_surfxml_cluster_suffix;
  cluster.radical     = A_surfxml_cluster_radical;
  cluster.speed       = surf_parse_get_speed(A_surfxml_cluster_speed, "speed of cluster", cluster.id);
  cluster.core_amount = surf_parse_get_int(A_surfxml_cluster_core);
  cluster.bw          = surf_parse_get_bandwidth(A_surfxml_cluster_bw, "bw of cluster", cluster.id);
  cluster.lat         = surf_parse_get_time(A_surfxml_cluster_lat, "lat of cluster", cluster.id);
  if(strcmp(A_surfxml_cluster_bb___bw,""))
    cluster.bb_bw = surf_parse_get_bandwidth(A_surfxml_cluster_bb___bw, "bb_bw of cluster", cluster.id);
  if(strcmp(A_surfxml_cluster_bb___lat,""))
    cluster.bb_lat = surf_parse_get_time(A_surfxml_cluster_bb___lat, "bb_lat of cluster", cluster.id);
  if(strcmp(A_surfxml_cluster_limiter___link,""))
    cluster.limiter_link = surf_parse_get_double(A_surfxml_cluster_limiter___link);
  if(strcmp(A_surfxml_cluster_loopback___bw,""))
    cluster.loopback_bw = surf_parse_get_bandwidth(A_surfxml_cluster_loopback___bw, "loopback_bw of cluster", cluster.id);
  if(strcmp(A_surfxml_cluster_loopback___lat,""))
    cluster.loopback_lat = surf_parse_get_time(A_surfxml_cluster_loopback___lat, "loopback_lat of cluster", cluster.id);

  switch(AX_surfxml_cluster_topology){
  case A_surfxml_cluster_topology_FLAT:
    cluster.topology= SURF_CLUSTER_FLAT ;
    break;
  case A_surfxml_cluster_topology_TORUS:
    cluster.topology= SURF_CLUSTER_TORUS ;
    break;
  case A_surfxml_cluster_topology_FAT___TREE:
    cluster.topology = SURF_CLUSTER_FAT_TREE;
    break;
  default:
    surf_parse_error("Invalid cluster topology for cluster %s",
                     cluster.id);
    break;
  }
  cluster.topo_parameters = A_surfxml_cluster_topo___parameters;
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

  current_property_set = nullptr;
}

void STag_surfxml_cluster(void){
  parse_after_config();
  xbt_assert(current_property_set == nullptr, "Someone forgot to reset the property set to nullptr in its closing tag (or XML malformed)");
}

void STag_surfxml_cabinet(void){
  parse_after_config();
  s_sg_platf_cabinet_cbarg_t cabinet = SG_PLATF_CABINET_INITIALIZER;
  memset(&cabinet,0,sizeof(cabinet));
  cabinet.id      = A_surfxml_cabinet_id;
  cabinet.prefix  = A_surfxml_cabinet_prefix;
  cabinet.suffix  = A_surfxml_cabinet_suffix;
  cabinet.speed   = surf_parse_get_speed(A_surfxml_cabinet_speed, "speed of cabinet", cabinet.id);
  cabinet.bw      = surf_parse_get_bandwidth(A_surfxml_cabinet_bw, "bw of cabinet", cabinet.id);
  cabinet.lat     = surf_parse_get_time(A_surfxml_cabinet_lat, "lat of cabinet", cabinet.id);
  cabinet.radical = A_surfxml_cabinet_radical;

  sg_platf_new_cabinet(&cabinet);
}

void STag_surfxml_peer(void){
  parse_after_config();
  s_sg_platf_peer_cbarg_t peer = SG_PLATF_PEER_INITIALIZER;
  memset(&peer,0,sizeof(peer));
  peer.id                 = A_surfxml_peer_id;
  peer.speed              = surf_parse_get_speed(A_surfxml_peer_speed, "speed of peer", peer.id);
  peer.bw_in              = surf_parse_get_bandwidth(A_surfxml_peer_bw___in, "bw_in of peer", peer.id);
  peer.bw_out             = surf_parse_get_bandwidth(A_surfxml_peer_bw___out, "bw_out of peer", peer.id);
  peer.lat                = surf_parse_get_time(A_surfxml_peer_lat, "lat of peer", peer.id);
  peer.coord              = A_surfxml_peer_coordinates;
  peer.availability_trace = A_surfxml_peer_availability___file[0] ? tmgr_trace_new_from_file(A_surfxml_peer_availability___file) : NULL;
  peer.state_trace        = A_surfxml_peer_state___file[0] ? tmgr_trace_new_from_file(A_surfxml_peer_state___file) : NULL;

  sg_platf_new_peer(&peer);
}

void STag_surfxml_link(void){
  AS_TAG = 0;
  xbt_assert(current_property_set == nullptr, "Someone forgot to reset the property set to nullptr in its closing tag (or XML malformed)");
}

void ETag_surfxml_link(void){
  s_sg_platf_link_cbarg_t link = SG_PLATF_LINK_INITIALIZER;
  memset(&link,0,sizeof(link));

  link.properties          = current_property_set;
  link.id                  = A_surfxml_link_id;
  link.bandwidth           = surf_parse_get_bandwidth(A_surfxml_link_bandwidth, "bandwidth of link", link.id);
  link.bandwidth_trace     = A_surfxml_link_bandwidth___file[0] ? tmgr_trace_new_from_file(A_surfxml_link_bandwidth___file) : NULL;
  link.latency             = surf_parse_get_time(A_surfxml_link_latency, "latency of link", link.id);
  link.latency_trace       = A_surfxml_link_latency___file[0] ? tmgr_trace_new_from_file(A_surfxml_link_latency___file) : NULL;
  link.state_trace         = A_surfxml_link_state___file[0] ? tmgr_trace_new_from_file(A_surfxml_link_state___file):NULL;

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

  current_property_set = nullptr;
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
  s_sg_platf_link_cbarg_t link = SG_PLATF_LINK_INITIALIZER;
  memset(&link,0,sizeof(link));

  link.properties = nullptr;

  link.id = A_surfxml_backbone_id;
  link.bandwidth = surf_parse_get_bandwidth(A_surfxml_backbone_bandwidth, "bandwidth of backbone", link.id);
  link.latency = surf_parse_get_time(A_surfxml_backbone_latency, "latency of backbone", link.id);
  link.policy = SURF_LINK_SHARED;

  sg_platf_new_link(&link);
  routing_cluster_add_backbone(sg_link_by_name(A_surfxml_backbone_id));
}

void STag_surfxml_route(void){
  surf_parse_assert(sg_netcard_by_name_or_null(A_surfxml_route_src),
      "Route src='%s' does name a node.", A_surfxml_route_src);
  surf_parse_assert(sg_netcard_by_name_or_null(A_surfxml_route_dst),
      "Route dst='%s' does name a node.", A_surfxml_route_dst);
  parsed_link_list = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
}

void STag_surfxml_ASroute(void){
  surf_parse_assert(sg_netcard_by_name_or_null(A_surfxml_ASroute_src),
      "ASroute src='%s' does name a node.", A_surfxml_route_src);
  surf_parse_assert(sg_netcard_by_name_or_null(A_surfxml_ASroute_dst),
      "ASroute dst='%s' does name a node.", A_surfxml_route_dst);

  surf_parse_assert(sg_netcard_by_name_or_null(A_surfxml_ASroute_gw___src),
      "ASroute gw_src='%s' does name a node.", A_surfxml_ASroute_gw___src);
  surf_parse_assert(sg_netcard_by_name_or_null(A_surfxml_ASroute_gw___dst),
      "ASroute gw_dst='%s' does name a node.", A_surfxml_ASroute_gw___dst);

  parsed_link_list = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
}

void STag_surfxml_bypassRoute(void){
  surf_parse_assert(sg_netcard_by_name_or_null(A_surfxml_bypassRoute_src),
      "bypassRoute src='%s' does name a node.", A_surfxml_bypassRoute_src);
  surf_parse_assert(sg_netcard_by_name_or_null(A_surfxml_bypassRoute_dst),
      "bypassRoute dst='%s' does name a node.", A_surfxml_bypassRoute_dst);

  parsed_link_list = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
}

void STag_surfxml_bypassASroute(void){
  surf_parse_assert(sg_netcard_by_name_or_null(A_surfxml_bypassASroute_src),
      "bypassASroute src='%s' does name a node.", A_surfxml_bypassASroute_src);
  surf_parse_assert(sg_netcard_by_name_or_null(A_surfxml_bypassASroute_dst),
      "bypassASroute dst='%s' does name a node.", A_surfxml_bypassASroute_dst);
  surf_parse_assert(sg_netcard_by_name_or_null(A_surfxml_bypassASroute_gw___src),
      "bypassASroute gw_src='%s' does name a node.", A_surfxml_bypassASroute_gw___src);
  surf_parse_assert(sg_netcard_by_name_or_null(A_surfxml_bypassASroute_gw___dst),
      "bypassASroute gw_dst='%s' does name a node.", A_surfxml_bypassASroute_gw___dst);

  parsed_link_list = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
}

void ETag_surfxml_route(void){
  s_sg_platf_route_cbarg_t route;
  memset(&route,0,sizeof(route));

  route.src       = A_surfxml_route_src;
  route.dst       = A_surfxml_route_dst;
  route.gw_src    = nullptr;
  route.gw_dst    = nullptr;
  route.link_list = new std::vector<Link*>();
  route.symmetrical = (A_surfxml_route_symmetrical == A_surfxml_route_symmetrical_YES);

  unsigned int cpt;
  char *link_name;
  xbt_dynar_foreach(parsed_link_list, cpt, link_name) {
    simgrid::surf::Link *link = Link::byName(link_name);
    route.link_list->push_back(link);
  }



  sg_platf_new_route(&route);
  parsed_link_list = nullptr;
}

void ETag_surfxml_ASroute(void){
  s_sg_platf_route_cbarg_t ASroute;
  memset(&ASroute,0,sizeof(ASroute));

  ASroute.src = A_surfxml_ASroute_src;
  ASroute.dst = A_surfxml_ASroute_dst;

  ASroute.gw_src = sg_netcard_by_name_or_null(A_surfxml_ASroute_gw___src);
  ASroute.gw_dst = sg_netcard_by_name_or_null(A_surfxml_ASroute_gw___dst);

  if (A_surfxml_ASroute_gw___src && !ASroute.gw_src)
    surf_parse_error("gw_src=\"%s\" not found for ASroute from \"%s\" to \"%s\"",
                     A_surfxml_ASroute_gw___src, ASroute.src, ASroute.dst);
  if (A_surfxml_ASroute_gw___dst && !ASroute.gw_dst)
    surf_parse_error("gw_dst=\"%s\" not found for ASroute from \"%s\" to \"%s\"",
                     A_surfxml_ASroute_gw___dst, ASroute.src, ASroute.dst);

  ASroute.link_list =  new std::vector<Link*>();

  unsigned int cpt;
  char *link_name;
  xbt_dynar_foreach(parsed_link_list, cpt, link_name) {
    simgrid::surf::Link *link = Link::byName(link_name);
    ASroute.link_list->push_back(link);
  }

  switch (A_surfxml_ASroute_symmetrical) {
  case AU_surfxml_ASroute_symmetrical:
  case A_surfxml_ASroute_symmetrical_YES:
    ASroute.symmetrical = true;
    break;
  case A_surfxml_ASroute_symmetrical_NO:
    ASroute.symmetrical = false;
    break;
  }

  sg_platf_new_route(&ASroute);
  parsed_link_list = nullptr;
}

void ETag_surfxml_bypassRoute(void){
  s_sg_platf_route_cbarg_t route;
  memset(&route,0,sizeof(route));

  route.src = A_surfxml_bypassRoute_src;
  route.dst = A_surfxml_bypassRoute_dst;
  route.gw_src = nullptr;
  route.gw_dst = nullptr;
  route.symmetrical = false;
  route.link_list =  new std::vector<Link*>();

  unsigned int cpt;
  char *link_name;
  xbt_dynar_foreach(parsed_link_list, cpt, link_name) {
    simgrid::surf::Link *link = Link::byName(link_name);
    route.link_list->push_back(link);
  }
  xbt_dynar_free(&parsed_link_list);

  sg_platf_new_bypassRoute(&route);
}

void ETag_surfxml_bypassASroute(void){
  s_sg_platf_route_cbarg_t ASroute;
  memset(&ASroute,0,sizeof(ASroute));

  ASroute.src         = A_surfxml_bypassASroute_src;
  ASroute.dst         = A_surfxml_bypassASroute_dst;
  ASroute.link_list   =  new std::vector<Link*>();
  unsigned int cpt;
  char *link_name;
  xbt_dynar_foreach(parsed_link_list, cpt, link_name) {
    simgrid::surf::Link *link = Link::byName(link_name);
    ASroute.link_list->push_back(link);
  }
  xbt_dynar_free(&parsed_link_list);
  ASroute.symmetrical = false;

  ASroute.gw_src = sg_netcard_by_name_or_null(A_surfxml_bypassASroute_gw___src);
  ASroute.gw_dst = sg_netcard_by_name_or_null(A_surfxml_bypassASroute_gw___dst);

  sg_platf_new_bypassRoute(&ASroute);
}

void ETag_surfxml_trace(void){
  s_sg_platf_trace_cbarg_t trace = SG_PLATF_TRACE_INITIALIZER;
  memset(&trace,0,sizeof(trace));

  trace.id = A_surfxml_trace_id;
  trace.file = A_surfxml_trace_file;
  trace.periodicity = surf_parse_get_double(A_surfxml_trace_periodicity);
  trace.pc_data = surfxml_pcdata;

  sg_platf_new_trace(&trace);
}

void STag_surfxml_trace___connect(void){
  parse_after_config();
  s_sg_platf_trace_connect_cbarg_t trace_connect = SG_PLATF_TRACE_CONNECT_INITIALIZER;
  memset(&trace_connect,0,sizeof(trace_connect));

  trace_connect.element = A_surfxml_trace___connect_element;
  trace_connect.trace = A_surfxml_trace___connect_trace;

  switch (A_surfxml_trace___connect_kind) {
  case AU_surfxml_trace___connect_kind:
  case A_surfxml_trace___connect_kind_SPEED:
    trace_connect.kind =  SURF_TRACE_CONNECT_KIND_SPEED;
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
  parse_after_config();
  AS_TAG                   = 1;
  s_sg_platf_AS_cbarg_t AS = SG_PLATF_AS_INITIALIZER;
  AS.id                    = A_surfxml_AS_id;
  AS.routing               = (int)A_surfxml_AS_routing;

  as_current_property_set = nullptr;

  sg_platf_new_AS_begin(&AS);
}
void ETag_surfxml_AS(void){
  if(as_prop_nb){
    char *name      = as_name_tab[as_prop_nb-1];
    xbt_dict_t dict = (xbt_dict_t) as_dict_tab[as_prop_nb-1];
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

void STag_surfxml_config(void){
  AS_TAG = 0;
  xbt_assert(current_property_set == nullptr, "Someone forgot to reset the property set to nullptr in its closing tag (or XML malformed)");
  XBT_DEBUG("START configuration name = %s",A_surfxml_config_id);
  if (_sg_cfg_init_status == 2) {
    surf_parse_error("All <config> tags must be given before any platform elements (such as <AS>, <host>, <cluster>, <link>, etc).");
  }
}
void ETag_surfxml_config(void){
  xbt_dict_cursor_t cursor = nullptr;
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
  current_property_set = nullptr;
}

static int argc;
static char **argv;

void STag_surfxml_process(void){
  AS_TAG  = 0;
  argc    = 1;
  argv    = xbt_new(char *, 1);
  argv[0] = xbt_strdup(A_surfxml_process_function);
  xbt_assert(current_property_set == nullptr, "Someone forgot to reset the property set to nullptr in its closing tag (or XML malformed)");
}

void ETag_surfxml_process(void){
  s_sg_platf_process_cbarg_t process = SG_PLATF_PROCESS_INITIALIZER;
  memset(&process,0,sizeof(process));

  process.argc       = argc;
  process.argv       = (const char **)argv;
  process.properties = current_property_set;
  process.host       = A_surfxml_process_host;
  process.function   = A_surfxml_process_function;
  process.start_time = surf_parse_get_double(A_surfxml_process_start___time);
  process.kill_time  = surf_parse_get_double(A_surfxml_process_kill___time);

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
  current_property_set = nullptr;
}

void STag_surfxml_argument(void){
  argc++;
  argv = (char**)xbt_realloc(argv, (argc) * sizeof(char **));
  argv[(argc) - 1] = xbt_strdup(A_surfxml_argument_value);
}

void STag_surfxml_model___prop(void){
  if (!current_model_property_set)
    current_model_property_set = xbt_dict_new_homogeneous(xbt_free_f);

  xbt_dict_set(current_model_property_set, A_surfxml_model___prop_id, xbt_strdup(A_surfxml_model___prop_value), nullptr);
}

/* nothing to do in those functions */
void ETag_surfxml_prop(void){}
void STag_surfxml_random(void){}
void ETag_surfxml_random(void){}
void ETag_surfxml_trace___connect(void){}
void STag_surfxml_trace(void){parse_after_config();}
void ETag_surfxml_router(void){}
void ETag_surfxml_host___link(void){}
void ETag_surfxml_cabinet(void){}
void ETag_surfxml_peer(void){}
void STag_surfxml_backbone(void){}
void ETag_surfxml_link___ctn(void){}
void ETag_surfxml_argument(void){}
void ETag_surfxml_model___prop(void){}

/* Open and Close parse file */

void surf_parse_open(const char *file)
{
  xbt_assert(file, "Cannot parse the NULL file. Bypassing the parser is strongly deprecated nowadays.");

  if (!surf_input_buffer_stack)
    surf_input_buffer_stack = xbt_dynar_new(sizeof(YY_BUFFER_STATE), nullptr);
  if (!surf_file_to_parse_stack)
    surf_file_to_parse_stack = xbt_dynar_new(sizeof(FILE *), nullptr);

  if (!surf_parsed_filename_stack)
    surf_parsed_filename_stack = xbt_dynar_new(sizeof(char *), &xbt_free_ref);

  surf_parsed_filename = xbt_strdup(file);
  char *dir = xbt_dirname(file);
  xbt_dynar_push(surf_path, &dir);

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
  if (surf_parsed_filename) {
    char *dir = nullptr;
    xbt_dynar_pop(surf_path, &dir);
    free(dir);
  }

  free(surf_parsed_filename);
  surf_parsed_filename = nullptr;

  if (surf_file_to_parse) {
    surf_parse__delete_buffer(surf_input_buffer);
    fclose(surf_file_to_parse);
    surf_file_to_parse = nullptr; //Must be reset for Bypass
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
xbt_dict_t get_as_router_properties(const char* name)
{
  return (xbt_dict_t)xbt_lib_get_or_null(as_router_lib, name, ROUTING_PROP_ASR_LEVEL);
}

