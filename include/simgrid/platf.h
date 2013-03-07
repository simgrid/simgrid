/* platf.h - Public interface to the SimGrid platforms                      */

/* Copyright (c) 2004-2012. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SG_PLATF_H
#define SG_PLATF_H

#include <xbt.h>

typedef void *sg_routing_link_t; /* The actual type is model-dependent so use void* instead*/
typedef struct s_routing_edge *sg_routing_edge_t;

XBT_PUBLIC(sg_routing_edge_t) sg_routing_edge_by_name_or_null(const char *name);

/** Defines whether a given resource is working or not */
typedef enum {
  SURF_RESOURCE_ON = 1,                   /**< Up & ready        */
  SURF_RESOURCE_OFF = 0                   /**< Down & broken     */
} e_surf_resource_state_t;

typedef enum {
  SURF_LINK_FULLDUPLEX = 2,
  SURF_LINK_SHARED = 1,
  SURF_LINK_FATPIPE = 0
} e_surf_link_sharing_policy_t;

typedef enum {
  SURF_TRACE_CONNECT_KIND_HOST_AVAIL = 4,
  SURF_TRACE_CONNECT_KIND_POWER = 3,
  SURF_TRACE_CONNECT_KIND_LINK_AVAIL = 2,
  SURF_TRACE_CONNECT_KIND_BANDWIDTH = 1,
  SURF_TRACE_CONNECT_KIND_LATENCY = 0
} e_surf_trace_connect_kind_t;

typedef enum {
  SURF_PROCESS_ON_FAILURE_DIE = 1,
  SURF_PROCESS_ON_FAILURE_RESTART = 0
} e_surf_process_on_failure_t;


typedef struct tmgr_trace *tmgr_trace_t; /**< Opaque structure defining an availability trace */

/** opaque structure defining a event generator for availability based on a probability distribution */
typedef struct probabilist_event_generator *probabilist_event_generator_t;

XBT_PUBLIC(tmgr_trace_t) tmgr_trace_new_from_file(const char *filename);
XBT_PUBLIC(tmgr_trace_t) tmgr_trace_new_from_string(const char *id,
                                                    const char *input,
                                                    double periodicity);

XBT_PUBLIC(tmgr_trace_t) tmgr_trace_generator_value(const char *id,
                                              probabilist_event_generator_t date_generator,
                                              probabilist_event_generator_t value_generator);
XBT_PUBLIC(tmgr_trace_t) tmgr_trace_generator_state(const char *id,
                                              probabilist_event_generator_t date_generator,
                                              e_surf_resource_state_t first_event_value);
XBT_PUBLIC(tmgr_trace_t) tmgr_trace_generator_avail_unavail(const char *id,
                                              probabilist_event_generator_t avail_duration_generator,
                                              probabilist_event_generator_t unavail_duration_generator,
                                              e_surf_resource_state_t first_event_value);

XBT_PUBLIC(probabilist_event_generator_t) tmgr_event_generator_new_uniform(const char* id,
                                                                           double min,
                                                                           double max);
XBT_PUBLIC(probabilist_event_generator_t) tmgr_event_generator_new_exponential(const char* id,
                                                                           double rate);
XBT_PUBLIC(probabilist_event_generator_t) tmgr_event_generator_new_weibull(const char* id,
                                                                           double scale,
                                                                           double shape);
typedef xbt_dictelm_t sg_host_t;
static inline char* sg_host_name(sg_host_t host) {
  return host->key;
}


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
  double power_peak;
  int core_amount;
  double power_scale;
  tmgr_trace_t power_trace;
  e_surf_resource_state_t initial_state;
  tmgr_trace_t state_trace;
  const char* coord;
  xbt_dict_t properties;
} s_sg_platf_host_cbarg_t, *sg_platf_host_cbarg_t;

#define SG_PLATF_HOST_INITIALIZER { \
    .id = NULL,\
    .power_peak = 0,\
    .core_amount = 1.,\
    .power_scale = 1,\
    .initial_state = SURF_RESOURCE_ON,\
    .power_trace = NULL,\
    .state_trace = NULL,\
    .coord = NULL,\
    .properties = NULL\
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
  e_surf_resource_state_t state;
  tmgr_trace_t state_trace;
  e_surf_link_sharing_policy_t policy;
  xbt_dict_t properties;
} s_sg_platf_link_cbarg_t, *sg_platf_link_cbarg_t;

#define SG_PLATF_LINK_INITIALIZER {\
  .id = NULL,\
  .bandwidth = 0.,\
  .bandwidth_trace = NULL,\
  .latency = 0.,\
  .latency_trace = NULL,\
  .state = SURF_RESOURCE_ON,\
  .state_trace = NULL,\
  .policy = SURF_LINK_SHARED,\
  .properties = NULL\
}

typedef struct s_sg_platf_peer_cbarg *sg_platf_peer_cbarg_t;
typedef struct s_sg_platf_peer_cbarg {
  const char* id;
  double power;
  double bw_in;
  double bw_out;
  double lat;
  const char* coord;
  tmgr_trace_t availability_trace;
  tmgr_trace_t state_trace;
} s_sg_platf_peer_cbarg_t;

#define SG_PLATF_PEER_INITIALIZER {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL}

typedef struct s_sg_platf_route_cbarg *sg_platf_route_cbarg_t;
typedef struct s_sg_platf_route_cbarg {
  int symmetrical;
  const char *src;
  const char *dst;
  sg_routing_edge_t gw_src;
  sg_routing_edge_t gw_dst;
  xbt_dynar_t link_list;
} s_sg_platf_route_cbarg_t;

#define SG_PLATF_ROUTE_INITIALIZER {TRUE,NULL,NULL,NULL,NULL,NULL}

typedef struct s_sg_platf_cluster_cbarg *sg_platf_cluster_cbarg_t;
typedef struct s_sg_platf_cluster_cbarg {
  const char* id;
  const char* prefix;
  const char* suffix;
  const char* radical;
  double power;
  int core_amount;
  double bw;
  double lat;
  double bb_bw;
  double bb_lat;
  double limiter_link;
  const char* router_id;
  e_surf_link_sharing_policy_t sharing_policy;
  e_surf_link_sharing_policy_t bb_sharing_policy;
  const char* availability_trace; //don't convert to tmgr_trace_t since there is a trace per host and some rewriting is needed
  const char* state_trace;
} s_sg_platf_cluster_cbarg_t;

#define SG_PLATF_CLUSTER_INITIALIZER {NULL,NULL,NULL,NULL,NULL,NULL \
  ,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL}

typedef struct s_sg_platf_cabinet_cbarg *sg_platf_cabinet_cbarg_t;
typedef struct s_sg_platf_cabinet_cbarg {
  const char* id;
  const char* prefix;
  const char* suffix;
  const char* radical;
  double power;
  double bw;
  double lat;
} s_sg_platf_cabinet_cbarg_t;

#define SG_PLATF_CABINET_INITIALIZER {NULL,NULL,NULL,NULL,NULL,NULL,NULL}

typedef struct {
  const char* id;
  const char* type_id;
  const char* content;
  xbt_dict_t properties;
} s_sg_platf_storage_cbarg_t, *sg_platf_storage_cbarg_t;

#define SG_PLATF_STORAGE_INITIALIZER {NULL,NULL,NULL,NULL}

typedef struct {
  const char* id;
  const char* model;
  const char* content;
  xbt_dict_t properties;
  unsigned long size; /* size in Gbytes */
} s_sg_platf_storage_type_cbarg_t, *sg_platf_storage_type_cbarg_t;

#define SG_PLATF_STORAGE_TYPE_INITIALIZER {NULL,NULL,NULL,NULL,NULL}

typedef struct {
  const char* type_id;
  const char* name;
} s_sg_platf_mstorage_cbarg_t, *sg_platf_mstorage_cbarg_t;

#define SG_PLATF_MSTORAGE_INITIALIZER {NULL,NULL}

typedef struct {
  const char* id;
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

#define SG_PLATF_TRACE_INITIALIZER {NULL,NULL,NULL,NULL}

typedef struct s_sg_platf_trace_connect_cbarg *sg_platf_trace_connect_cbarg_t;
typedef struct s_sg_platf_trace_connect_cbarg {
  e_surf_trace_connect_kind_t kind;
  const char *trace;
  const char *element;
} s_sg_platf_trace_connect_cbarg_t;

#define SG_PLATF_TRACE_CONNECT_INITIALIZER {NULL,NULL,NULL}

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

#define SG_PLATF_PROCESS_INITIALIZER {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL}

typedef struct s_sg_platf_AS_cbarg *sg_platf_AS_cbarg_t;
typedef struct s_sg_platf_AS_cbarg {
  const char *id;
  int routing;
} s_sg_platf_AS_cbarg_t;

#define SG_PLATF_AS_INITIALIZER {NULL,0}

/* ***************************************** */
/* TUTORIAL: New TAG                         */

typedef struct s_sg_platf_gpu_cbarg *sg_platf_gpu_cbarg_t;
typedef struct s_sg_platf_gpu_cbarg {
  const char *name;
} s_sg_platf_gpu_cbarg_t;

#define SG_PLATF_GPU_INITIALIZER {NULL}

/* ***************************************** */

XBT_PUBLIC(void) sg_platf_begin(void);  // Start a new platform
XBT_PUBLIC(void) sg_platf_end(void); // Finish the creation of the platform

XBT_PUBLIC(void) sg_platf_new_AS_begin(sg_platf_AS_cbarg_t AS); // Begin description of new AS
XBT_PUBLIC(void) sg_platf_new_AS_end(void);                            // That AS is fully described

XBT_PUBLIC(void) sg_platf_new_host   (sg_platf_host_cbarg_t   host);   // Add an host   to the currently described AS
XBT_PUBLIC(void) sg_platf_new_host_link(sg_platf_host_link_cbarg_t h); // Add an host_link to the currently described AS
XBT_PUBLIC(void) sg_platf_new_router (sg_platf_router_cbarg_t router); // Add a router  to the currently described AS
XBT_PUBLIC(void) sg_platf_new_link   (sg_platf_link_cbarg_t link);     // Add a link    to the currently described AS
XBT_PUBLIC(void) sg_platf_new_peer   (sg_platf_peer_cbarg_t peer);     // Add a peer    to the currently described AS
XBT_PUBLIC(void) sg_platf_new_cluster(sg_platf_cluster_cbarg_t clust); // Add a cluster to the currently described AS
XBT_PUBLIC(void) sg_platf_new_cabinet(sg_platf_cabinet_cbarg_t cabinet); // Add a cabinet to the currently described AS

XBT_PUBLIC(void) sg_platf_new_route (sg_platf_route_cbarg_t route); // Add a route
XBT_PUBLIC(void) sg_platf_new_ASroute (sg_platf_route_cbarg_t ASroute); // Add an ASroute
XBT_PUBLIC(void) sg_platf_new_bypassRoute (sg_platf_route_cbarg_t bypassroute); // Add a bypassRoute
XBT_PUBLIC(void) sg_platf_new_bypassASroute (sg_platf_route_cbarg_t bypassASroute); // Add an bypassASroute
XBT_PUBLIC(void) sg_platf_new_prop (sg_platf_prop_cbarg_t prop); // Add a prop

XBT_PUBLIC(void) sg_platf_new_trace(sg_platf_trace_cbarg_t trace);
XBT_PUBLIC(void) sg_platf_trace_connect(sg_platf_trace_connect_cbarg_t trace_connect);

XBT_PUBLIC(void) sg_platf_new_storage(sg_platf_storage_cbarg_t storage); // Add a storage to the currently described AS
XBT_PUBLIC(void) sg_platf_new_storage(sg_platf_storage_cbarg_t storage); // Add a storage to the currently described AS
XBT_PUBLIC(void) sg_platf_new_mstorage(sg_platf_mstorage_cbarg_t mstorage);
XBT_PUBLIC(void) sg_platf_new_storage_type(sg_platf_storage_type_cbarg_t storage_type);
XBT_PUBLIC(void) sg_platf_new_mount(sg_platf_mount_cbarg_t mount);

XBT_PUBLIC(void) sg_platf_new_process(sg_platf_process_cbarg_t process);

/* ***************************************** */
/* TUTORIAL: New TAG                         */
XBT_PUBLIC(void) sg_platf_new_gpu(sg_platf_gpu_cbarg_t gpu);
/* ***************************************** */

// Add route and Asroute without xml file with those functions
XBT_PUBLIC(void) sg_platf_route_begin (sg_platf_route_cbarg_t route); // Initialize route
XBT_PUBLIC(void) sg_platf_route_end (sg_platf_route_cbarg_t route); // Finalize and add a route

XBT_PUBLIC(void) sg_platf_ASroute_begin (sg_platf_route_cbarg_t ASroute); // Initialize ASroute
XBT_PUBLIC(void) sg_platf_ASroute_end (sg_platf_route_cbarg_t ASroute); // Finalize and add a ASroute

XBT_PUBLIC(void) sg_platf_route_add_link (const char* link_id, sg_platf_route_cbarg_t route); // Add a link to link list
XBT_PUBLIC(void) sg_platf_ASroute_add_link (const char* link_id, sg_platf_route_cbarg_t ASroute); // Add a link to link list

typedef void (*sg_platf_process_cb_t)(sg_platf_process_cbarg_t);
XBT_PUBLIC(void) sg_platf_process_add_cb(sg_platf_process_cb_t fct);


#endif                          /* SG_PLATF_H */
