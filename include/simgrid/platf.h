/* platf.h - Public interface to the SimGrid platforms                      */

/* Copyright (c) 2004-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SG_PLATF_H
#define SG_PLATF_H

#include <xbt.h>
#include <simgrid/host.h>
#include "forward.h"

SG_BEGIN_DECL()

static inline char* sg_storage_name(sg_storage_t storage) {
  return storage->key;
}

XBT_PUBLIC(sg_netcard_t) sg_netcard_by_name_or_null(const char *name);

XBT_PUBLIC(tmgr_trace_t) tmgr_trace_new_from_file(const char *filename);
XBT_PUBLIC(tmgr_trace_t) tmgr_trace_new_from_string(const char *id,
                                                    const char *input,
                                                    double periodicity);
typedef enum {
  SURF_CLUSTER_FAT_TREE=2,
  SURF_CLUSTER_FLAT = 1,
  SURF_CLUSTER_TORUS = 0
} e_surf_cluster_topology_t;


typedef struct s_model_type {
  AS_t (*create) ();
} s_routing_model_description_t, *routing_model_description_t;

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
  xbt_dynar_t speed_peak;
  int pstate;
  int core_amount;
  double speed_scale;
  tmgr_trace_t speed_trace;
  int initiallyOn;
  tmgr_trace_t state_trace;
  const char* coord;
  xbt_dict_t properties;
} s_sg_platf_host_cbarg_t, *sg_platf_host_cbarg_t;

#define SG_PLATF_HOST_INITIALIZER { \
    NULL, 0, 1, 1, 1., NULL, 1/*ON*/, NULL, \
    NULL, NULL \
}

typedef struct {
  const char* id;
  const char* link_up;
  const char* link_down;
} s_sg_platf_host_link_cbarg_t, *sg_platf_host_link_cbarg_t;

#define SG_PLATF_HOST_LINK_INITIALIZER {NULL,NULL,NULL}

typedef struct {
  const char* id;
  const char* coord;
} s_sg_platf_router_cbarg_t, *sg_platf_router_cbarg_t;

#define SG_PLATF_ROUTER_INITIALIZER {NULL,NULL}

typedef struct {
  const char* id;
  double bandwidth;
  tmgr_trace_t bandwidth_trace;
  double latency;
  tmgr_trace_t latency_trace;
  int initiallyOn;
  tmgr_trace_t state_trace;
  e_surf_link_sharing_policy_t policy;
  xbt_dict_t properties;
} s_sg_platf_link_cbarg_t, *sg_platf_link_cbarg_t;

#define SG_PLATF_LINK_INITIALIZER {\
  NULL, 0., NULL, 0., NULL, 1/*ON*/, \
  NULL, SURF_LINK_SHARED, NULL \
}

typedef struct s_sg_platf_peer_cbarg *sg_platf_peer_cbarg_t;
typedef struct s_sg_platf_peer_cbarg {
  const char* id;
  double speed;
  double bw_in;
  double bw_out;
  double lat;
  const char* coord;
  tmgr_trace_t availability_trace;
  tmgr_trace_t state_trace;
} s_sg_platf_peer_cbarg_t;

#define SG_PLATF_PEER_INITIALIZER {NULL,0.0,0.0,0.0,0.0,NULL,NULL,NULL}

typedef struct s_sg_platf_route_cbarg *sg_platf_route_cbarg_t;
typedef struct s_sg_platf_route_cbarg {
  int symmetrical;
  const char *src;
  const char *dst;
  sg_netcard_t gw_src;
  sg_netcard_t gw_dst;
  xbt_dynar_t link_list;
} s_sg_platf_route_cbarg_t;

#define SG_PLATF_ROUTE_INITIALIZER {1,NULL,NULL,NULL,NULL,NULL}

typedef struct s_sg_platf_cluster_cbarg *sg_platf_cluster_cbarg_t;
typedef struct s_sg_platf_cluster_cbarg {
  const char* id;
  const char* prefix;
  const char* suffix;
  const char* radical;
  double speed;
  int core_amount;
  double bw;
  double lat;
  double bb_bw;
  double bb_lat;
  double loopback_bw;
  double loopback_lat;
  double limiter_link;
  e_surf_cluster_topology_t topology;
  const char* topo_parameters;
  xbt_dict_t properties;
  const char* router_id;
  e_surf_link_sharing_policy_t sharing_policy;
  e_surf_link_sharing_policy_t bb_sharing_policy;
  const char* availability_trace; //don't convert to tmgr_trace_t since there is a trace per host and some rewriting is needed
  const char* state_trace;
} s_sg_platf_cluster_cbarg_t;

#define SG_PLATF_CLUSTER_INITIALIZER {NULL,NULL,NULL,NULL,0.0,1 \
  ,1.,1.,0.,0.,0.,0.,0. \
  ,SURF_CLUSTER_FLAT,NULL,NULL,NULL, \
  SURF_LINK_SHARED,SURF_LINK_SHARED,NULL \
  ,NULL}

typedef struct s_sg_platf_cabinet_cbarg *sg_platf_cabinet_cbarg_t;
typedef struct s_sg_platf_cabinet_cbarg {
  const char* id;
  const char* prefix;
  const char* suffix;
  const char* radical;
  double speed;
  double bw;
  double lat;
} s_sg_platf_cabinet_cbarg_t;

#define SG_PLATF_CABINET_INITIALIZER {NULL,NULL,NULL,NULL,0.0,0.0,0.0}

typedef struct {
  const char* id;
  const char* type_id;
  const char* content;
  const char* content_type;
  xbt_dict_t properties;
  const char* attach;
} s_sg_platf_storage_cbarg_t, *sg_platf_storage_cbarg_t;

#define SG_PLATF_STORAGE_INITIALIZER {NULL,NULL,NULL,NULL,NULL,NULL}

typedef struct {
  const char* id;
  const char* model;
  const char* content;
  const char* content_type;
  xbt_dict_t properties;
  xbt_dict_t model_properties;
  sg_size_t size;
} s_sg_platf_storage_type_cbarg_t, *sg_platf_storage_type_cbarg_t;

#define SG_PLATF_STORAGE_TYPE_INITIALIZER {NULL,NULL,NULL,NULL,NULL,NULL,0}

typedef struct {
  const char* type_id;
  const char* name;
} s_sg_platf_mstorage_cbarg_t, *sg_platf_mstorage_cbarg_t;

#define SG_PLATF_MSTORAGE_INITIALIZER {NULL,NULL}

typedef struct {
  const char* storageId;
  const char* name;
} s_sg_platf_mount_cbarg_t, *sg_platf_mount_cbarg_t;

#define SG_PLATF_MOUNT_INITIALIZER {NULL,NULL}

typedef struct s_sg_platf_prop_cbarg *sg_platf_prop_cbarg_t;
typedef struct s_sg_platf_prop_cbarg {
  const char *id;
  const char *value;
} s_sg_platf_prop_cbarg_t;

#define SG_PLATF_PROP_INITIALIZER {NULL,NULL}

typedef struct s_sg_platf_trace_cbarg *sg_platf_trace_cbarg_t;
typedef struct s_sg_platf_trace_cbarg {
  const char *id;
  const char *file;
  double periodicity;
  const char *pc_data;
} s_sg_platf_trace_cbarg_t;

#define SG_PLATF_TRACE_INITIALIZER {NULL,NULL,0.0,NULL}

typedef struct s_sg_platf_trace_connect_cbarg *sg_platf_trace_connect_cbarg_t;
typedef struct s_sg_platf_trace_connect_cbarg {
  e_surf_trace_connect_kind_t kind;
  const char *trace;
  const char *element;
} s_sg_platf_trace_connect_cbarg_t;

#define SG_PLATF_TRACE_CONNECT_INITIALIZER {SURF_TRACE_CONNECT_KIND_LATENCY,NULL,NULL}

typedef struct s_sg_platf_process_cbarg *sg_platf_process_cbarg_t;
typedef struct s_sg_platf_process_cbarg {
  const char **argv;
  int argc;
  xbt_dict_t properties;
  const char *host;
  const char *function;
  double start_time;
  double kill_time;
  e_surf_process_on_failure_t on_failure;
} s_sg_platf_process_cbarg_t;

#define SG_PLATF_PROCESS_INITIALIZER {NULL,0,NULL,NULL,NULL,-1.0,-1.0,SURF_PROCESS_ON_FAILURE_DIE}

typedef struct s_sg_platf_AS_cbarg *sg_platf_AS_cbarg_t;
typedef struct s_sg_platf_AS_cbarg {
  const char *id;
  int routing;
} s_sg_platf_AS_cbarg_t;

#define SG_PLATF_AS_INITIALIZER {NULL,0}

/** opaque structure defining a event generator for availability based on a probability distribution */
typedef struct probabilist_event_generator *probabilist_event_generator_t;

/*** END of the parsing cruft ***/

XBT_PUBLIC(void) sg_platf_begin(void);  // Start a new platform
XBT_PUBLIC(void) sg_platf_end(void); // Finish the creation of the platform

XBT_PUBLIC(void) sg_platf_new_AS_begin(sg_platf_AS_cbarg_t AS); // Begin description of new AS
XBT_PUBLIC(void) sg_platf_new_AS_end(void);                            // That AS is fully described

XBT_PUBLIC(void) sg_platf_new_host   (sg_platf_host_cbarg_t   host);   // Add an host   to the currently described AS
XBT_PUBLIC(void) sg_platf_new_hostlink(sg_platf_host_link_cbarg_t h); // Add an host_link to the currently described AS
XBT_PUBLIC(void) sg_platf_new_router (sg_platf_router_cbarg_t router); // Add a router  to the currently described AS
XBT_PUBLIC(void) sg_platf_new_link   (sg_platf_link_cbarg_t link);     // Add a link    to the currently described AS
XBT_PUBLIC(void) sg_platf_new_peer   (sg_platf_peer_cbarg_t peer);     // Add a peer    to the currently described AS
XBT_PUBLIC(void) sg_platf_new_cluster(sg_platf_cluster_cbarg_t clust); // Add a cluster to the currently described AS
XBT_PUBLIC(void) sg_platf_new_cabinet(sg_platf_cabinet_cbarg_t cabinet); // Add a cabinet to the currently described AS

XBT_PUBLIC(void) sg_platf_new_route (sg_platf_route_cbarg_t route); // Add a route
XBT_PUBLIC(void) sg_platf_new_ASroute (sg_platf_route_cbarg_t ASroute); // Add an ASroute
XBT_PUBLIC(void) sg_platf_new_bypassRoute (sg_platf_route_cbarg_t bypassroute); // Add a bypassRoute
XBT_PUBLIC(void) sg_platf_new_bypassASroute (sg_platf_route_cbarg_t bypassASroute); // Add an bypassASroute

XBT_PUBLIC(void) sg_platf_new_trace(sg_platf_trace_cbarg_t trace);

XBT_PUBLIC(void) sg_platf_new_storage(sg_platf_storage_cbarg_t storage); // Add a storage to the currently described AS
XBT_PUBLIC(void) sg_platf_new_mstorage(sg_platf_mstorage_cbarg_t mstorage);
XBT_PUBLIC(void) sg_platf_new_storage_type(sg_platf_storage_type_cbarg_t storage_type);
XBT_PUBLIC(void) sg_platf_new_mount(sg_platf_mount_cbarg_t mount);

XBT_PUBLIC(void) sg_platf_new_process(sg_platf_process_cbarg_t process);

// Add route and Asroute without xml file with those functions
XBT_PUBLIC(void) sg_platf_route_begin (sg_platf_route_cbarg_t route); // Initialize route
XBT_PUBLIC(void) sg_platf_route_end (sg_platf_route_cbarg_t route); // Finalize and add a route

XBT_PUBLIC(void) sg_platf_ASroute_begin (sg_platf_route_cbarg_t ASroute); // Initialize ASroute
XBT_PUBLIC(void) sg_platf_ASroute_end (sg_platf_route_cbarg_t ASroute); // Finalize and add a ASroute

XBT_PUBLIC(void) sg_platf_route_add_link (const char* link_id, sg_platf_route_cbarg_t route); // Add a link to link list
XBT_PUBLIC(void) sg_platf_ASroute_add_link (const char* link_id, sg_platf_route_cbarg_t ASroute); // Add a link to link list

SG_END_DECL()

#endif                          /* SG_PLATF_H */
