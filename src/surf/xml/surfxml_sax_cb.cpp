/* Copyright (c) 2006-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Engine.hpp"
#include "simgrid/sg_config.h"
#include "src/kernel/routing/NetPoint.hpp"
#include "src/surf/network_interface.hpp"
#include "xbt/file.h"

#include "src/surf/xml/platf_private.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <string>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_parse, surf, "Logging specific to the SURF parsing module");

SG_BEGIN_DECL()

int ETag_surfxml_include_state();

#include "simgrid_dtd.c"

char* surf_parsed_filename = nullptr; // to locate parse error messages

std::vector<simgrid::surf::LinkImpl*> parsed_link_list; /* temporary store of current list link of a route */

/*
 * Helping functions
 */
void surf_parse_assert(bool cond, std::string msg)
{
  if (not cond) {
    int lineno = surf_parse_lineno;
    cleanup();
    XBT_ERROR("Parse error at %s:%d: %s", surf_parsed_filename, lineno, msg.c_str());
    surf_exit();
    xbt_die("Exiting now");
  }
}
void surf_parse_error(std::string msg)
{
  int lineno = surf_parse_lineno;
  cleanup();
  XBT_ERROR("Parse error at %s:%d: %s", surf_parsed_filename, lineno, msg.c_str());
  surf_exit();
  xbt_die("Exiting now");
}

void surf_parse_assert_netpoint(char* hostname, const char* pre, const char* post)
{
  if (sg_netpoint_by_name_or_null(hostname) != nullptr) // found
    return;

  std::string msg = std::string(pre);
  msg += hostname;
  msg += post;
  msg += " Existing netpoints: \n";

  std::vector<simgrid::kernel::routing::NetPoint*> list;
  simgrid::s4u::Engine::getInstance()->getNetpointList(&list);
  std::sort(list.begin(), list.end(),
      [](simgrid::kernel::routing::NetPoint* a, simgrid::kernel::routing::NetPoint* b) {
      return a->name() < b->name();
  });
  bool first = true;
  for (auto np : list) {
    if (np->isNetZone())
      continue;

    if (not first)
      msg += ",";
    first = false;
    msg += "'" + np->name() + "'";
    if (msg.length() > 4096) {
      msg.pop_back(); // remove trailing quote
      msg += "...(list truncated)......";
      break;
    }
  }
  surf_parse_error(msg);
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
    surf_parse_error(std::string(string) + " is not a double");
  return res;
}

int surf_parse_get_int(const char *string) {
  int res;
  int ret = sscanf(string, "%d", &res);
  if (ret != 1)
    surf_parse_error(std::string(string) + " is not an integer");
  return res;
}

/* Turn something like "1-4,6,9-11" into the vector {1,2,3,4,6,9,10,11} */
static std::vector<int>* explodesRadical(const char* radicals)
{
  std::vector<int>* exploded = new std::vector<int>();

  // Make all hosts
  std::vector<std::string> radical_elements;
  boost::split(radical_elements, radicals, boost::is_any_of(","));
  for (auto group : radical_elements) {
    std::vector<std::string> radical_ends;
    boost::split(radical_ends, group, boost::is_any_of("-"));
    int start                = surf_parse_get_int((radical_ends.front()).c_str());
    int end                  = 0;

    switch (radical_ends.size()) {
      case 1:
        end = start;
        break;
      case 2:
        end = surf_parse_get_int((radical_ends.back()).c_str());
        break;
      default:
        surf_parse_error(std::string("Malformed radical: ") + group);
        break;
    }
    for (int i = start; i <= end; i++)
      exploded->push_back(i);
  }

  return exploded;
}

struct unit_scale {
  const char *unit;
  double scale;
};

/* Note: field `unit' for the last element of parameter `units' should be nullptr. */
static double surf_parse_get_value_with_unit(const char *string, const struct unit_scale *units,
    const char *entity_kind, const char *name, const char *error_msg, const char *default_unit)
{
  char* ptr;
  int i;
  errno = 0;
  double res   = strtod(string, &ptr);
  if (errno == ERANGE)
    surf_parse_error(std::string("value out of range: ") + string);
  if (ptr == string)
    surf_parse_error(std::string("cannot parse number:") + string);
  if (ptr[0] == '\0') {
    if (res == 0)
      return res; // Ok, 0 can be unit-less

    XBT_WARN("Deprecated unit-less value '%s' for %s %s. %s",string, entity_kind, name, error_msg);
    ptr = (char*)default_unit;
  }
  for (i = 0; units[i].unit != nullptr && strcmp(ptr, units[i].unit) != 0; i++);

  if (units[i].unit != nullptr)
    res *= units[i].scale;
  else
    surf_parse_error(std::string("unknown unit: ") + ptr);
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
    { "EiB", pow(1024, 6) },
    { "PiB", pow(1024, 5) },
    { "TiB", pow(1024, 4) },
    { "GiB", pow(1024, 3) },
    { "MiB", pow(1024, 2) },
    { "KiB", 1024 },
    { "EB",  1e18 },
    { "PB",  1e15 },
    { "TB",  1e12 },
    { "GB",  1e9 },
    { "MB",  1e6 },
    { "kB",  1e3 },
    { "B",   1.0 },
    { "Eib", 0.125 * pow(1024, 6) },
    { "Pib", 0.125 * pow(1024, 5) },
    { "Tib", 0.125 * pow(1024, 4) },
    { "Gib", 0.125 * pow(1024, 3) },
    { "Mib", 0.125 * pow(1024, 2) },
    { "Kib", 0.125 * 1024 },
    { "Eb",  0.125 * 1e18 },
    { "Pb",  0.125 * 1e15 },
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
    { "EiBps", pow(1024, 6) },
    { "PiBps", pow(1024, 5) },
    { "TiBps", pow(1024, 4) },
    { "GiBps", pow(1024, 3) },
    { "MiBps", pow(1024, 2) },
    { "KiBps", 1024 },
    { "EBps",  1e18 },
    { "PBps",  1e15 },
    { "TBps",  1e12 },
    { "GBps",  1e9 },
    { "MBps",  1e6 },
    { "kBps",  1e3 },
    { "Bps",   1.0 },
    { "Eibps", 0.125 * pow(1024, 6) },
    { "Pibps", 0.125 * pow(1024, 5) },
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

static std::vector<double> surf_parse_get_all_speeds(char* speeds, const char* entity_kind, const char* id){

  std::vector<double> speed_per_pstate;

  if (strchr(speeds, ',') == nullptr){
    double speed = surf_parse_get_speed(speeds, entity_kind, id);
    speed_per_pstate.push_back(speed);
  } else {
    std::vector<std::string> pstate_list;
    boost::split(pstate_list, speeds, boost::is_any_of(","));
    for (auto speed_str : pstate_list) {
      boost::trim(speed_str);
      double speed = surf_parse_get_speed(speed_str.c_str(), entity_kind, id);
      speed_per_pstate.push_back(speed);
      XBT_DEBUG("Speed value: %f", speed);
    }
  }
  return speed_per_pstate;
}

/*
 * All the callback lists that can be overridden anywhere.
 * (this list should probably be reduced to the bare minimum to allow the models to work)
 */

/* make sure these symbols are defined as strong ones in this file so that the linker can resolve them */

/* The default current property receiver. Setup in the corresponding opening callbacks. */
std::map<std::string, std::string>* current_property_set       = nullptr;
std::map<std::string, std::string>* current_model_property_set = nullptr;
int ZONE_TAG                            = 0; // Whether we just opened a zone tag (to see what to do with the properties)

YY_BUFFER_STATE surf_input_buffer;
FILE *surf_file_to_parse = nullptr;

/* Stuff relative to storage */
void STag_surfxml_storage()
{
  ZONE_TAG = 0;
  XBT_DEBUG("STag_surfxml_storage");
  xbt_assert(current_property_set == nullptr, "Someone forgot to reset the property set to nullptr in its closing tag (or XML malformed)");
}

void ETag_surfxml_storage()
{
  StorageCreationArgs storage;

  storage.properties   = current_property_set;
  current_property_set = nullptr;

  storage.id           = A_surfxml_storage_id;
  storage.type_id      = A_surfxml_storage_typeId;
  storage.content      = A_surfxml_storage_content;
  storage.attach       = A_surfxml_storage_attach;

  sg_platf_new_storage(&storage);
}
void STag_surfxml_storage___type()
{
  ZONE_TAG = 0;
  XBT_DEBUG("STag_surfxml_storage___type");
  xbt_assert(current_property_set == nullptr, "Someone forgot to reset the property set to nullptr in its closing tag (or XML malformed)");
  xbt_assert(current_model_property_set == nullptr, "Someone forgot to reset the model property set to nullptr in its closing tag (or XML malformed)");
}
void ETag_surfxml_storage___type()
{
  StorageTypeCreationArgs storage_type;

  storage_type.properties = current_property_set;
  current_property_set    = nullptr;

  storage_type.model_properties = current_model_property_set;
  current_model_property_set    = nullptr;

  storage_type.content = A_surfxml_storage___type_content;
  storage_type.id      = A_surfxml_storage___type_id;
  storage_type.model   = A_surfxml_storage___type_model;
  storage_type.size =
      surf_parse_get_size(A_surfxml_storage___type_size, "size of storage type", storage_type.id.c_str());
  sg_platf_new_storage_type(&storage_type);
}

void STag_surfxml_mount()
{
  XBT_DEBUG("STag_surfxml_mount");
}

void ETag_surfxml_mount()
{
  MountCreationArgs mount;

  mount.name      = A_surfxml_mount_name;
  mount.storageId = A_surfxml_mount_storageId;
  sg_platf_new_mount(&mount);
}

/*
 * Stuff relative to the <include> tag
 */
static std::vector<YY_BUFFER_STATE> surf_input_buffer_stack;
static std::vector<FILE*> surf_file_to_parse_stack;
static std::vector<char*> surf_parsed_filename_stack;

void STag_surfxml_include()
{
  parse_after_config();
  XBT_DEBUG("STag_surfxml_include '%s'",A_surfxml_include_file);
  surf_parsed_filename_stack.push_back(surf_parsed_filename); // save old file name
  surf_parsed_filename = xbt_strdup(A_surfxml_include_file);

  surf_file_to_parse_stack.push_back(surf_file_to_parse); // save old file descriptor

  surf_file_to_parse = surf_fopen(A_surfxml_include_file, "r"); // read new file descriptor
  xbt_assert((surf_file_to_parse), "Unable to open \"%s\"\n", A_surfxml_include_file);

  surf_input_buffer_stack.push_back(surf_input_buffer);
  surf_input_buffer = surf_parse__create_buffer(surf_file_to_parse, YY_BUF_SIZE);
  surf_parse_push_buffer_state(surf_input_buffer);

  fflush(nullptr);
}

void ETag_surfxml_include() {
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
 * directly: a command line flag could instruct it to do the correct thing when the include directive is encountered
 * on a line. One day maybe, if the maya allow it.
 */
int ETag_surfxml_include_state()
{
  fflush(nullptr);
  XBT_DEBUG("ETag_surfxml_include_state '%s'",A_surfxml_include_file);

  if (surf_input_buffer_stack.empty()) // nope, that's a true premature EOF. Let the parser die verbosely.
    return 0;

  // Yeah, we were in an <include> Restore state and proceed.
  fclose(surf_file_to_parse);
  surf_file_to_parse_stack.pop_back();
  surf_parse_pop_buffer_state();
  surf_input_buffer_stack.pop_back();

  // Restore the filename for error messages
  free(surf_parsed_filename);
  surf_parsed_filename_stack.pop_back();

  return 1;
}

/* Stag and Etag parse functions */

void STag_surfxml_platform() {
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
  xbt_assert((version >= 4.0),
             "******* FILE %s IS TOO OLD (v:%.1f) *********\n "
             "Changes introduced in SimGrid 3.13:\n"
             "  - 'power' attribute of hosts (and others) got renamed to 'speed'.\n"
             "  - In <trace_connect>, attribute kind=\"POWER\" is now kind=\"SPEED\".\n"
             "  - DOCTYPE now point to the rignt URL: http://simgrid.gforge.inria.fr/simgrid/simgrid.dtd\n"
             "  - speed, bandwidth and latency attributes now MUST have an explicit unit (f, Bps, s by default)"
             "\n\n"
             "Use simgrid_update_xml to update your file automatically. "
             "This program is installed automatically with SimGrid, or "
             "available in the tools/ directory of the source archive.",
             surf_parsed_filename, version);
  if (version < 4.1) {
    XBT_INFO("You're using a v%.1f XML file (%s) while the current standard is v4.1 "
             "That's fine, the new version is backward compatible. \n\n"
             "Use simgrid_update_xml to update your file automatically. "
             "This program is installed automatically with SimGrid, or "
             "available in the tools/ directory of the source archive.",
             version, surf_parsed_filename);
  }
  xbt_assert(version <= 4.1, "******* FILE %s COMES FROM THE FUTURE (v:%.1f) *********\n "
                             "The most recent formalism that this version of SimGrid understands is v4.1.\n"
                             "Please update your code, or use another, more adapted, file.",
             surf_parsed_filename, version);

  sg_platf_begin();
}
void ETag_surfxml_platform(){
  sg_platf_end();
}

void STag_surfxml_host(){
  ZONE_TAG = 0;
  xbt_assert(current_property_set == nullptr, "Someone forgot to reset the property set to nullptr in its closing tag (or XML malformed)");
}

void STag_surfxml_prop()
{
  if (ZONE_TAG) { // We need to retrieve the most recently opened zone
    XBT_DEBUG("Set zone property %s -> %s", A_surfxml_prop_id, A_surfxml_prop_value);
    simgrid::s4u::NetZone* netzone = simgrid::s4u::Engine::getInstance()->getNetzoneByNameOrNull(A_surfxml_zone_id);

    netzone->setProperty(A_surfxml_prop_id, A_surfxml_prop_value);
  } else {
    if (not current_property_set)
      current_property_set = new std::map<std::string, std::string>; // Maybe, it should raise an error
    current_property_set->insert({A_surfxml_prop_id, A_surfxml_prop_value});
    XBT_DEBUG("add prop %s=%s into current property set %p", A_surfxml_prop_id, A_surfxml_prop_value,
              current_property_set);
  }
}

void ETag_surfxml_host()    {
  s_sg_platf_host_cbarg_t host;
  memset(&host,0,sizeof(host));

  host.properties = current_property_set;
  current_property_set = nullptr;

  host.id = A_surfxml_host_id;

  host.speed_per_pstate = surf_parse_get_all_speeds(A_surfxml_host_speed, "speed of host", host.id);

  XBT_DEBUG("pstate: %s", A_surfxml_host_pstate);
  host.core_amount = surf_parse_get_int(A_surfxml_host_core);
  host.speed_trace = A_surfxml_host_availability___file[0] ? tmgr_trace_new_from_file(A_surfxml_host_availability___file) : nullptr;
  host.state_trace = A_surfxml_host_state___file[0] ? tmgr_trace_new_from_file(A_surfxml_host_state___file) : nullptr;
  host.pstate      = surf_parse_get_int(A_surfxml_host_pstate);
  host.coord       = A_surfxml_host_coordinates;

  sg_platf_new_host(&host);
}

void STag_surfxml_host___link(){
  XBT_DEBUG("Create a Host_link for %s",A_surfxml_host___link_id);
  s_sg_platf_host_link_cbarg_t host_link;
  memset(&host_link,0,sizeof(host_link));

  host_link.id        = A_surfxml_host___link_id;
  host_link.link_up   = A_surfxml_host___link_up;
  host_link.link_down = A_surfxml_host___link_down;
  sg_platf_new_hostlink(&host_link);
}

void STag_surfxml_router(){
  sg_platf_new_router(A_surfxml_router_id, A_surfxml_router_coordinates);
}

void ETag_surfxml_cluster(){
  s_sg_platf_cluster_cbarg_t cluster;
  memset(&cluster,0,sizeof(cluster));
  cluster.properties = current_property_set;
  current_property_set = nullptr;

  cluster.id          = A_surfxml_cluster_id;
  cluster.prefix      = A_surfxml_cluster_prefix;
  cluster.suffix      = A_surfxml_cluster_suffix;
  cluster.radicals    = explodesRadical(A_surfxml_cluster_radical);
  cluster.speeds      = surf_parse_get_all_speeds(A_surfxml_cluster_speed, "speed of cluster", cluster.id);
  cluster.core_amount = surf_parse_get_int(A_surfxml_cluster_core);
  cluster.bw          = surf_parse_get_bandwidth(A_surfxml_cluster_bw, "bw of cluster", cluster.id);
  cluster.lat         = surf_parse_get_time(A_surfxml_cluster_lat, "lat of cluster", cluster.id);
  if(strcmp(A_surfxml_cluster_bb___bw,""))
    cluster.bb_bw = surf_parse_get_bandwidth(A_surfxml_cluster_bb___bw, "bb_bw of cluster", cluster.id);
  if(strcmp(A_surfxml_cluster_bb___lat,""))
    cluster.bb_lat = surf_parse_get_time(A_surfxml_cluster_bb___lat, "bb_lat of cluster", cluster.id);
  if(strcmp(A_surfxml_cluster_limiter___link,""))
    cluster.limiter_link = surf_parse_get_bandwidth(A_surfxml_cluster_limiter___link, "limiter_link of cluster", cluster.id);
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
  case A_surfxml_cluster_topology_DRAGONFLY:
    cluster.topology= SURF_CLUSTER_DRAGONFLY ;
    break;
  default:
    surf_parse_error(std::string("Invalid cluster topology for cluster ") + cluster.id);
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
    surf_parse_error(std::string("Invalid cluster sharing policy for cluster ") + cluster.id);
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
    surf_parse_error(std::string("Invalid bb sharing policy in cluster ") + cluster.id);
    break;
  }

  sg_platf_new_cluster(&cluster);
}

void STag_surfxml_cluster(){
  ZONE_TAG = 0;
  parse_after_config();
  xbt_assert(current_property_set == nullptr, "Someone forgot to reset the property set to nullptr in its closing tag (or XML malformed)");
}

void STag_surfxml_cabinet(){
  parse_after_config();
  CabinetCreationArgs cabinet;
  cabinet.id      = A_surfxml_cabinet_id;
  cabinet.prefix  = A_surfxml_cabinet_prefix;
  cabinet.suffix  = A_surfxml_cabinet_suffix;
  cabinet.speed    = surf_parse_get_speed(A_surfxml_cabinet_speed, "speed of cabinet", cabinet.id.c_str());
  cabinet.bw       = surf_parse_get_bandwidth(A_surfxml_cabinet_bw, "bw of cabinet", cabinet.id.c_str());
  cabinet.lat      = surf_parse_get_time(A_surfxml_cabinet_lat, "lat of cabinet", cabinet.id.c_str());
  cabinet.radicals = explodesRadical(A_surfxml_cabinet_radical);

  sg_platf_new_cabinet(&cabinet);
}

void STag_surfxml_peer(){
  parse_after_config();
  PeerCreationArgs peer;

  peer.id          = std::string(A_surfxml_peer_id);
  peer.speed       = surf_parse_get_speed(A_surfxml_peer_speed, "speed of peer", peer.id.c_str());
  peer.bw_in       = surf_parse_get_bandwidth(A_surfxml_peer_bw___in, "bw_in of peer", peer.id.c_str());
  peer.bw_out      = surf_parse_get_bandwidth(A_surfxml_peer_bw___out, "bw_out of peer", peer.id.c_str());
  peer.coord       = A_surfxml_peer_coordinates;
  peer.speed_trace = A_surfxml_peer_availability___file[0] ? tmgr_trace_new_from_file(A_surfxml_peer_availability___file) : nullptr;
  peer.state_trace = A_surfxml_peer_state___file[0] ? tmgr_trace_new_from_file(A_surfxml_peer_state___file) : nullptr;

  if (A_surfxml_peer_lat[0] != '\0')
    XBT_WARN("The latency parameter in <peer> is now deprecated. Use the z coordinate instead of '%s'.",
             A_surfxml_peer_lat);

  sg_platf_new_peer(&peer);
}

void STag_surfxml_link(){
  ZONE_TAG = 0;
  xbt_assert(current_property_set == nullptr, "Someone forgot to reset the property set to nullptr in its closing tag (or XML malformed)");
}

void ETag_surfxml_link(){
  LinkCreationArgs link;

  link.properties          = current_property_set;
  current_property_set     = nullptr;

  link.id                  = std::string(A_surfxml_link_id);
  link.bandwidth           = surf_parse_get_bandwidth(A_surfxml_link_bandwidth, "bandwidth of link", link.id.c_str());
  link.bandwidth_trace     = A_surfxml_link_bandwidth___file[0] ? tmgr_trace_new_from_file(A_surfxml_link_bandwidth___file) : nullptr;
  link.latency             = surf_parse_get_time(A_surfxml_link_latency, "latency of link", link.id.c_str());
  link.latency_trace       = A_surfxml_link_latency___file[0] ? tmgr_trace_new_from_file(A_surfxml_link_latency___file) : nullptr;
  link.state_trace         = A_surfxml_link_state___file[0] ? tmgr_trace_new_from_file(A_surfxml_link_state___file):nullptr;

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
    surf_parse_error(std::string("Invalid sharing policy in link ") + link.id);
    break;
  }

  sg_platf_new_link(&link);
}

void STag_surfxml_link___ctn()
{
  simgrid::surf::LinkImpl* link = nullptr;
  switch (A_surfxml_link___ctn_direction) {
  case AU_surfxml_link___ctn_direction:
  case A_surfxml_link___ctn_direction_NONE:
    link = simgrid::surf::LinkImpl::byName(A_surfxml_link___ctn_id);
    break;
  case A_surfxml_link___ctn_direction_UP:
    link = simgrid::surf::LinkImpl::byName(std::string(A_surfxml_link___ctn_id) + "_UP");
    break;
  case A_surfxml_link___ctn_direction_DOWN:
    link = simgrid::surf::LinkImpl::byName(std::string(A_surfxml_link___ctn_id) + "_DOWN");
    break;
  default:
    surf_parse_error(std::string("Invalid direction for link ") + A_surfxml_link___ctn_id);
    break;
  }

  const char* dirname = "";
  switch (A_surfxml_link___ctn_direction) {
    case A_surfxml_link___ctn_direction_UP:
      dirname = " (upward)";
      break;
    case A_surfxml_link___ctn_direction_DOWN:
      dirname = " (downward)";
      break;
    default:
      dirname = "";
  }
  surf_parse_assert(link != nullptr, std::string("No such link: '") + A_surfxml_link___ctn_id + "'" + dirname);
  parsed_link_list.push_back(link);
}

void ETag_surfxml_backbone(){
  LinkCreationArgs link;

  link.properties = nullptr;
  link.id = std::string(A_surfxml_backbone_id);
  link.bandwidth = surf_parse_get_bandwidth(A_surfxml_backbone_bandwidth, "bandwidth of backbone", link.id.c_str());
  link.latency = surf_parse_get_time(A_surfxml_backbone_latency, "latency of backbone", link.id.c_str());
  link.policy = SURF_LINK_SHARED;

  sg_platf_new_link(&link);
  routing_cluster_add_backbone(simgrid::surf::LinkImpl::byName(A_surfxml_backbone_id));
}

void STag_surfxml_route(){
  surf_parse_assert_netpoint(A_surfxml_route_src, "Route src='", "' does name a node.");
  surf_parse_assert_netpoint(A_surfxml_route_dst, "Route dst='", "' does name a node.");
}

void STag_surfxml_ASroute(){
  surf_parse_assert_netpoint(A_surfxml_ASroute_src, "ASroute src='", "' does name a node.");
  surf_parse_assert_netpoint(A_surfxml_ASroute_dst, "ASroute dst='", "' does name a node.");

  surf_parse_assert_netpoint(A_surfxml_ASroute_gw___src, "ASroute gw_src='", "' does name a node.");
  surf_parse_assert_netpoint(A_surfxml_ASroute_gw___dst, "ASroute gw_dst='", "' does name a node.");
}
void STag_surfxml_zoneRoute(){
  surf_parse_assert_netpoint(A_surfxml_zoneRoute_src, "zoneRoute src='", "' does name a node.");
  surf_parse_assert_netpoint(A_surfxml_zoneRoute_dst, "zoneRoute dst='", "' does name a node.");
  surf_parse_assert_netpoint(A_surfxml_zoneRoute_gw___src, "zoneRoute gw_src='", "' does name a node.");
  surf_parse_assert_netpoint(A_surfxml_zoneRoute_gw___dst, "zoneRoute gw_dst='", "' does name a node.");
}

void STag_surfxml_bypassRoute(){
  surf_parse_assert_netpoint(A_surfxml_bypassRoute_src, "bypassRoute src='", "' does name a node.");
  surf_parse_assert_netpoint(A_surfxml_bypassRoute_dst, "bypassRoute dst='", "' does name a node.");
}

void STag_surfxml_bypassASroute(){
  surf_parse_assert_netpoint(A_surfxml_bypassASroute_src, "bypassASroute src='", "' does name a node.");
  surf_parse_assert_netpoint(A_surfxml_bypassASroute_dst, "bypassASroute dst='", "' does name a node.");
  surf_parse_assert_netpoint(A_surfxml_bypassASroute_gw___src, "bypassASroute gw_src='", "' does name a node.");
  surf_parse_assert_netpoint(A_surfxml_bypassASroute_gw___dst, "bypassASroute gw_dst='", "' does name a node.");
}
void STag_surfxml_bypassZoneRoute(){
  surf_parse_assert_netpoint(A_surfxml_bypassZoneRoute_src, "bypassZoneRoute src='", "' does name a node.");
  surf_parse_assert_netpoint(A_surfxml_bypassZoneRoute_dst, "bypassZoneRoute dst='", "' does name a node.");
  surf_parse_assert_netpoint(A_surfxml_bypassZoneRoute_gw___src, "bypassZoneRoute gw_src='", "' does name a node.");
  surf_parse_assert_netpoint(A_surfxml_bypassZoneRoute_gw___dst, "bypassZoneRoute gw_dst='", "' does name a node.");
}

void ETag_surfxml_route(){
  s_sg_platf_route_cbarg_t route;
  memset(&route,0,sizeof(route));

  route.src         = sg_netpoint_by_name_or_null(A_surfxml_route_src); // tested to not be nullptr in start tag
  route.dst         = sg_netpoint_by_name_or_null(A_surfxml_route_dst); // tested to not be nullptr in start tag
  route.gw_src    = nullptr;
  route.gw_dst    = nullptr;
  route.link_list   = new std::vector<simgrid::surf::LinkImpl*>();
  route.symmetrical = (A_surfxml_route_symmetrical == A_surfxml_route_symmetrical_YES);

  for (auto link: parsed_link_list)
    route.link_list->push_back(link);
  parsed_link_list.clear();

  sg_platf_new_route(&route);
  delete route.link_list;
}

void ETag_surfxml_ASroute()
{
  AX_surfxml_zoneRoute_src = AX_surfxml_ASroute_src;
  AX_surfxml_zoneRoute_dst = AX_surfxml_ASroute_dst;
  AX_surfxml_zoneRoute_gw___src = AX_surfxml_ASroute_gw___src;
  AX_surfxml_zoneRoute_gw___dst = AX_surfxml_ASroute_gw___dst;
  AX_surfxml_zoneRoute_symmetrical = (AT_surfxml_zoneRoute_symmetrical)AX_surfxml_ASroute_symmetrical;
  ETag_surfxml_zoneRoute();
}
void ETag_surfxml_zoneRoute()
{
  s_sg_platf_route_cbarg_t ASroute;
  memset(&ASroute,0,sizeof(ASroute));

  ASroute.src = sg_netpoint_by_name_or_null(A_surfxml_zoneRoute_src); // tested to not be nullptr in start tag
  ASroute.dst = sg_netpoint_by_name_or_null(A_surfxml_zoneRoute_dst); // tested to not be nullptr in start tag

  ASroute.gw_src = sg_netpoint_by_name_or_null(A_surfxml_zoneRoute_gw___src); // tested to not be nullptr in start tag
  ASroute.gw_dst = sg_netpoint_by_name_or_null(A_surfxml_zoneRoute_gw___dst); // tested to not be nullptr in start tag

  ASroute.link_list = new std::vector<simgrid::surf::LinkImpl*>();

  for (auto link: parsed_link_list)
    ASroute.link_list->push_back(link);
  parsed_link_list.clear();

  switch (A_surfxml_zoneRoute_symmetrical) {
  case AU_surfxml_zoneRoute_symmetrical:
  case A_surfxml_zoneRoute_symmetrical_YES:
    ASroute.symmetrical = true;
    break;
  case A_surfxml_zoneRoute_symmetrical_NO:
    ASroute.symmetrical = false;
    break;
  }

  sg_platf_new_route(&ASroute);
  delete ASroute.link_list;
}

void ETag_surfxml_bypassRoute(){
  s_sg_platf_route_cbarg_t route;
  memset(&route,0,sizeof(route));

  route.src         = sg_netpoint_by_name_or_null(A_surfxml_bypassRoute_src); // tested to not be nullptr in start tag
  route.dst         = sg_netpoint_by_name_or_null(A_surfxml_bypassRoute_dst); // tested to not be nullptr in start tag
  route.gw_src = nullptr;
  route.gw_dst = nullptr;
  route.symmetrical = false;
  route.link_list   = new std::vector<simgrid::surf::LinkImpl*>();

  for (auto link: parsed_link_list)
    route.link_list->push_back(link);
  parsed_link_list.clear();

  sg_platf_new_bypassRoute(&route);
  delete route.link_list;
}

void ETag_surfxml_bypassASroute()
{
  AX_surfxml_bypassZoneRoute_src = AX_surfxml_bypassASroute_src;
  AX_surfxml_bypassZoneRoute_dst = AX_surfxml_bypassASroute_dst;
  AX_surfxml_bypassZoneRoute_gw___src = AX_surfxml_bypassASroute_gw___src;
  AX_surfxml_bypassZoneRoute_gw___dst = AX_surfxml_bypassASroute_gw___dst;
  ETag_surfxml_bypassZoneRoute();
}
void ETag_surfxml_bypassZoneRoute()
{
  s_sg_platf_route_cbarg_t ASroute;
  memset(&ASroute,0,sizeof(ASroute));

  ASroute.src         = sg_netpoint_by_name_or_null(A_surfxml_bypassZoneRoute_src);
  ASroute.dst         = sg_netpoint_by_name_or_null(A_surfxml_bypassZoneRoute_dst);
  ASroute.link_list   = new std::vector<simgrid::surf::LinkImpl*>();
  for (auto link: parsed_link_list)
    ASroute.link_list->push_back(link);
  parsed_link_list.clear();

  ASroute.symmetrical = false;

  ASroute.gw_src = sg_netpoint_by_name_or_null(A_surfxml_bypassZoneRoute_gw___src);
  ASroute.gw_dst = sg_netpoint_by_name_or_null(A_surfxml_bypassZoneRoute_gw___dst);

  sg_platf_new_bypassRoute(&ASroute);
  delete ASroute.link_list;
}

void ETag_surfxml_trace(){
  TraceCreationArgs trace;

  trace.id = A_surfxml_trace_id;
  trace.file = A_surfxml_trace_file;
  trace.periodicity = surf_parse_get_double(A_surfxml_trace_periodicity);
  trace.pc_data = surfxml_pcdata;

  sg_platf_new_trace(&trace);
}

void STag_surfxml_trace___connect()
{
  parse_after_config();
  TraceConnectCreationArgs trace_connect;
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
  default:
    surf_parse_error("Invalid trace kind");
    break;
  }
  sg_platf_trace_connect(&trace_connect);
}

void STag_surfxml_AS()
{
  AX_surfxml_zone_id = AX_surfxml_AS_id;
  AX_surfxml_zone_routing = (AT_surfxml_zone_routing)AX_surfxml_AS_routing;
  STag_surfxml_zone();
}

void ETag_surfxml_AS()
{
  ETag_surfxml_zone();
}

void STag_surfxml_zone()
{
  parse_after_config();
  ZONE_TAG                 = 1;
  ZoneCreationArgs zone;
  zone.id      = A_surfxml_zone_id;
  zone.routing = static_cast<int>(A_surfxml_zone_routing);

  sg_platf_new_Zone_begin(&zone);
}

void ETag_surfxml_zone()
{
  sg_platf_new_Zone_seal();
}

void STag_surfxml_config()
{
  ZONE_TAG = 0;
  xbt_assert(current_property_set == nullptr, "Someone forgot to reset the property set to nullptr in its closing tag (or XML malformed)");
  XBT_DEBUG("START configuration name = %s",A_surfxml_config_id);
  if (_sg_cfg_init_status == 2) {
    surf_parse_error("All <config> tags must be given before any platform elements (such as <zone>, <host>, <cluster>, "
                     "<link>, etc).");
  }
}

void ETag_surfxml_config()
{
  for (auto elm : *current_property_set) {
    if (xbt_cfg_is_default_value(elm.first.c_str())) {
      std::string cfg = elm.first + ":" + elm.second;
      xbt_cfg_set_parse(cfg.c_str());
    } else
      XBT_INFO("The custom configuration '%s' is already defined by user!", elm.first.c_str());
  }
  XBT_DEBUG("End configuration name = %s",A_surfxml_config_id);

  delete current_property_set;
  current_property_set = nullptr;
}

static int argc;
static char **argv;

void STag_surfxml_process()
{
  AX_surfxml_actor_function = AX_surfxml_process_function;
  STag_surfxml_actor();
}
void STag_surfxml_actor()
{
  ZONE_TAG  = 0;
  argc    = 1;
  argv    = xbt_new(char *, 1);
  argv[0] = xbt_strdup(A_surfxml_actor_function);
  xbt_assert(current_property_set == nullptr, "Someone forgot to reset the property set to nullptr in its closing tag (or XML malformed)");
}

void ETag_surfxml_process()
{
  AX_surfxml_actor_host = AX_surfxml_process_host;
  AX_surfxml_actor_function = AX_surfxml_process_function;
  AX_surfxml_actor_start___time = AX_surfxml_process_start___time;
  AX_surfxml_actor_kill___time = AX_surfxml_process_kill___time;
  AX_surfxml_actor_on___failure = (AT_surfxml_actor_on___failure)AX_surfxml_process_on___failure;
  ETag_surfxml_actor();
}
void ETag_surfxml_actor()
{
  s_sg_platf_process_cbarg_t actor;
  memset(&actor,0,sizeof(actor));

  actor.argc       = argc;
  actor.argv       = (const char **)argv;
  actor.properties = current_property_set;
  actor.host       = A_surfxml_actor_host;
  actor.function   = A_surfxml_actor_function;
  actor.start_time = surf_parse_get_double(A_surfxml_actor_start___time);
  actor.kill_time  = surf_parse_get_double(A_surfxml_actor_kill___time);

  switch (A_surfxml_actor_on___failure) {
  case AU_surfxml_actor_on___failure:
  case A_surfxml_actor_on___failure_DIE:
    actor.on_failure =  SURF_ACTOR_ON_FAILURE_DIE;
    break;
  case A_surfxml_actor_on___failure_RESTART:
    actor.on_failure =  SURF_ACTOR_ON_FAILURE_RESTART;
    break;
  default:
    surf_parse_error("Invalid on failure behavior");
    break;
  }

  sg_platf_new_process(&actor);

  for (int i = 0; i != argc; ++i)
    xbt_free(argv[i]);
  xbt_free(argv);
  argv = nullptr;

  current_property_set = nullptr;
}

void STag_surfxml_argument(){
  argc++;
  argv = (char**)xbt_realloc(argv, (argc) * sizeof(char **));
  argv[(argc) - 1] = xbt_strdup(A_surfxml_argument_value);
}

void STag_surfxml_model___prop(){
  if (not current_model_property_set)
    current_model_property_set = new std::map<std::string, std::string>();

  current_model_property_set->insert(
      {std::string(A_surfxml_model___prop_id), std::string(A_surfxml_model___prop_value)});
}

void ETag_surfxml_prop(){/* Nothing to do */}
void STag_surfxml_random(){/* Nothing to do */}
void ETag_surfxml_random(){/* Nothing to do */}
void ETag_surfxml_trace___connect(){/* Nothing to do */}
void STag_surfxml_trace(){parse_after_config();}
void ETag_surfxml_router(){/*Nothing to do*/}
void ETag_surfxml_host___link(){/* Nothing to do */}
void ETag_surfxml_cabinet(){/* Nothing to do */}
void ETag_surfxml_peer(){/* Nothing to do */}
void STag_surfxml_backbone(){/* Nothing to do */}
void ETag_surfxml_link___ctn(){/* Nothing to do */}
void ETag_surfxml_argument(){/* Nothing to do */}
void ETag_surfxml_model___prop(){/* Nothing to do */}

/* Open and Close parse file */
void surf_parse_open(const char *file)
{
  xbt_assert(file, "Cannot parse the nullptr file. Bypassing the parser is strongly deprecated nowadays.");

  surf_parsed_filename = xbt_strdup(file);
  char* dir            = xbt_dirname(file);
  surf_path.push_back(std::string(dir));
  xbt_free(dir);

  surf_file_to_parse = surf_fopen(file, "r");
  if (surf_file_to_parse == nullptr)
    xbt_die("Unable to open '%s'\n", file);
  surf_input_buffer = surf_parse__create_buffer(surf_file_to_parse, YY_BUF_SIZE);
  surf_parse__switch_to_buffer(surf_input_buffer);
  surf_parse_lineno = 1;
}

void surf_parse_close()
{
  if (surf_parsed_filename) {
    surf_path.pop_back();
  }

  free(surf_parsed_filename);
  surf_parsed_filename = nullptr;

  if (surf_file_to_parse) {
    surf_parse__delete_buffer(surf_input_buffer);
    fclose(surf_file_to_parse);
    surf_file_to_parse = nullptr; //Must be reset for Bypass
  }
}

/* Call the lexer to parse the currently opened file */
int surf_parse()
{
  return surf_parse_lex();
}

SG_END_DECL()
