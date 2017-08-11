/* platf_private.h - Interface to the SimGrid platforms which visibility should be limited to this directory */

/* Copyright (c) 2004-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SG_PLATF_H
#define SG_PLATF_H

#include "simgrid/host.h"
#include "src/surf/xml/platf.hpp"
#include <map>
#include <string>
#include <vector>

SG_BEGIN_DECL()
#include "src/surf/xml/simgrid_dtd.h"

#ifndef YY_TYPEDEF_YY_SIZE_T
#define YY_TYPEDEF_YY_SIZE_T
typedef size_t yy_size_t;
#endif

typedef enum {
  SURF_CLUSTER_DRAGONFLY=3,
  SURF_CLUSTER_FAT_TREE=2,
  SURF_CLUSTER_FLAT = 1,
  SURF_CLUSTER_TORUS = 0
} e_surf_cluster_topology_t;

/* ***************************************** */
/*
 * Platform creation functions. Instead of passing 123 arguments to the creation functions
 * (one for each possible XML attribute), we pass structures containing them all. It removes the
 * chances of switching arguments by error, and reduce the burden when we add a new attribute:
 * old models can just continue to ignore it without having to update their headers.
 *
 * It shouldn't be too costly at runtime, provided that structures living on the stack are
 * used, instead of malloced structures.
 */

typedef struct {
  const char* id;
  std::vector<double> speed_per_pstate;
  int pstate;
  int core_amount;
  tmgr_trace_t speed_trace;
  tmgr_trace_t state_trace;
  const char* coord;
  std::map<std::string, std::string>* properties;
} s_sg_platf_host_cbarg_t;
typedef s_sg_platf_host_cbarg_t* sg_platf_host_cbarg_t;

class HostLinkCreationArgs {
public:
  std::string id;
  std::string link_up;
  std::string link_down;
};

class LinkCreationArgs {
public:
  std::string id;
  double bandwidth                    = 0;
  tmgr_trace_t bandwidth_trace        = nullptr;
  double latency                      = 0;
  tmgr_trace_t latency_trace          = nullptr;
  tmgr_trace_t state_trace            = nullptr;
  e_surf_link_sharing_policy_t policy = SURF_LINK_FATPIPE;
  std::map<std::string, std::string>* properties = nullptr;
};

class PeerCreationArgs {
public:
  std::string id;
  double speed;
  double bw_in;
  double bw_out;
  std::string coord;
  tmgr_trace_t speed_trace;
  tmgr_trace_t state_trace;
};

typedef struct s_sg_platf_route_cbarg *sg_platf_route_cbarg_t;
typedef struct s_sg_platf_route_cbarg {
  bool symmetrical;
  sg_netpoint_t src;
  sg_netpoint_t dst;
  sg_netpoint_t gw_src;
  sg_netpoint_t gw_dst;
  std::vector<simgrid::surf::LinkImpl*>* link_list;
} s_sg_platf_route_cbarg_t;

class ClusterCreationArgs {
public:
  std::string id;
  std::string prefix;
  std::string suffix;
  std::vector<int>* radicals = nullptr;
  std::vector<double> speeds;
  int core_amount     = 0;
  double bw           = 0;
  double lat          = 0;
  double bb_bw        = 0;
  double bb_lat       = 0;
  double loopback_bw  = 0;
  double loopback_lat = 0;
  double limiter_link = 0;
  e_surf_cluster_topology_t topology;
  std::string topo_parameters;
  std::map<std::string, std::string>* properties;
  std::string router_id;
  e_surf_link_sharing_policy_t sharing_policy;
  e_surf_link_sharing_policy_t bb_sharing_policy;
};

class CabinetCreationArgs {
public:
  std::string id;
  std::string prefix;
  std::string suffix;
  std::vector<int>* radicals;
  double speed;
  double bw;
  double lat;
};

class StorageCreationArgs {
public:
  std::string id;
  std::string type_id;
  std::string content;
  std::map<std::string, std::string>* properties;
  std::string attach;
};

class StorageTypeCreationArgs {
public:
  std::string id;
  std::string model;
  std::string content;
  std::map<std::string, std::string>* properties;
  std::map<std::string, std::string>* model_properties;
  sg_size_t size;
};

class MountCreationArgs {
public:
  std::string storageId;
  std::string name;
};

typedef struct s_sg_platf_prop_cbarg *sg_platf_prop_cbarg_t;
typedef struct s_sg_platf_prop_cbarg {
  const char *id;
  const char *value;
} s_sg_platf_prop_cbarg_t;

class TraceCreationArgs {
public:
  std::string id;
  std::string file;
  double periodicity;
  std::string pc_data;
};

class TraceConnectCreationArgs {
public:
  e_surf_trace_connect_kind_t kind;
  std::string trace;
  std::string element;
};

typedef struct s_sg_platf_process_cbarg *sg_platf_process_cbarg_t;
typedef struct s_sg_platf_process_cbarg {
  const char **argv;
  int argc;
  std::map<std::string, std::string>* properties;
  const char *host;
  const char *function;
  double start_time;
  double kill_time;
  e_surf_process_on_failure_t on_failure;
} s_sg_platf_process_cbarg_t;

class ZoneCreationArgs {
public:
  std::string id;
  int routing;
};

/* The default current property receiver. Setup in the corresponding opening callbacks. */
extern std::map<std::string, std::string>* current_property_set;

/********** Routing **********/
void routing_cluster_add_backbone(simgrid::surf::LinkImpl* bb);
/*** END of the parsing cruft ***/

XBT_PUBLIC(void) sg_platf_begin();  // Start a new platform
XBT_PUBLIC(void) sg_platf_end(); // Finish the creation of the platform

XBT_PUBLIC(simgrid::s4u::NetZone*) sg_platf_new_Zone_begin(ZoneCreationArgs* zone); // Begin description of new Zone
XBT_PUBLIC(void) sg_platf_new_Zone_seal();                                          // That Zone is fully described

XBT_PUBLIC(void) sg_platf_new_host(sg_platf_host_cbarg_t host);        // Add a host      to the current Zone
XBT_PUBLIC(void) sg_platf_new_hostlink(HostLinkCreationArgs* h);       // Add a host_link to the current Zone
XBT_PUBLIC(void) sg_platf_new_link(LinkCreationArgs* link);            // Add a link      to the current Zone
XBT_PUBLIC(void) sg_platf_new_peer(PeerCreationArgs* peer);            // Add a peer      to the current Zone
XBT_PUBLIC(void) sg_platf_new_cluster(ClusterCreationArgs* clust);     // Add a cluster   to the current Zone
XBT_PUBLIC(void) sg_platf_new_cabinet(CabinetCreationArgs* cabinet);   // Add a cabinet   to the current Zone
XBT_PUBLIC(simgrid::kernel::routing::NetPoint*)                        // Add a router    to the current Zone
sg_platf_new_router(std::string, const char* coords);

XBT_PUBLIC(void) sg_platf_new_route (sg_platf_route_cbarg_t route); // Add a route
XBT_PUBLIC(void) sg_platf_new_bypassRoute (sg_platf_route_cbarg_t bypassroute); // Add a bypassRoute

XBT_PUBLIC(void) sg_platf_new_trace(TraceCreationArgs* trace);

XBT_PUBLIC(void) sg_platf_new_storage(StorageCreationArgs* storage); // Add a storage to the current Zone
XBT_PUBLIC(void) sg_platf_new_storage_type(StorageTypeCreationArgs* storage_type);
XBT_PUBLIC(void) sg_platf_new_mount(MountCreationArgs* mount);

XBT_PUBLIC(void) sg_platf_new_process(sg_platf_process_cbarg_t process);
XBT_PRIVATE void sg_platf_trace_connect(TraceConnectCreationArgs* trace_connect);

/* Prototypes of the functions offered by flex */
XBT_PUBLIC(int) surf_parse_lex();
XBT_PUBLIC(int) surf_parse_get_lineno();
XBT_PUBLIC(FILE *) surf_parse_get_in();
XBT_PUBLIC(FILE *) surf_parse_get_out();
XBT_PUBLIC(int) surf_parse_get_leng();
XBT_PUBLIC(char *) surf_parse_get_text();
XBT_PUBLIC(void) surf_parse_set_lineno(int line_number);
XBT_PUBLIC(void) surf_parse_set_in(FILE * in_str);
XBT_PUBLIC(void) surf_parse_set_out(FILE * out_str);
XBT_PUBLIC(int) surf_parse_get_debug();
XBT_PUBLIC(void) surf_parse_set_debug(int bdebug);
XBT_PUBLIC(int) surf_parse_lex_destroy();

XBT_PUBLIC(void) routing_route_free(sg_platf_route_cbarg_t route);

SG_END_DECL()

namespace simgrid {
namespace surf {

extern XBT_PRIVATE simgrid::xbt::signal<void(ClusterCreationArgs*)> on_cluster;
}
}

#endif                          /* SG_PLATF_H */
