/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#ifndef _SURF_SURF_PRIVATE_H
#define _SURF_SURF_PRIVATE_H

#include "surf/surf.h"
#include "surf/maxmin.h"
#include "surf/trace_mgr.h"
#include "xbt/log.h"
#include "surf/surfxml_parse_private.h"
#include "surf/random_mgr.h"
#include "instr/private.h"

#define NO_MAX_DURATION -1.0

/* user-visible parameters */
extern double sg_tcp_gamma;
extern double sg_latency_factor;
extern double sg_bandwidth_factor;
extern double sg_weight_S_parameter;
extern int sg_maxmin_selective_update;
extern int sg_network_fullduplex;
#ifdef HAVE_GTNETS
extern double sg_gtnets_jitter;
extern int sg_gtnets_jitter_seed;
#endif


extern const char *surf_action_state_names[6];

typedef struct surf_model_private {
  int (*resource_used) (void *resource_id);
  /* Share the resources to the actions and return in how much time
     the next action may terminate */
  double (*share_resources) (double now);
  /* Update the actions' state */
  void (*update_actions_state) (double now, double delta);
  void (*update_resource_state) (void *id, tmgr_trace_event_t event_type,
                                 double value, double time);
  void (*finalize) (void);
} s_surf_model_private_t;

double generic_maxmin_share_resources(xbt_swag_t running_actions,
                                      size_t offset,
                                      lmm_system_t sys,
                                      void (*solve) (lmm_system_t));

/* Generic functions common to all models */
e_surf_action_state_t surf_action_state_get(surf_action_t action);      /* cannot declare inline since we use a pointer to it */
double surf_action_get_start_time(surf_action_t action);        /* cannot declare inline since we use a pointer to it */
double surf_action_get_finish_time(surf_action_t action);       /* cannot declare inline since we use a pointer to it */
void surf_action_free(surf_action_t * action);
void surf_action_state_set(surf_action_t action,
                           e_surf_action_state_t state);
void surf_action_data_set(surf_action_t action, void *data);    /* cannot declare inline since we use a pointer to it */
FILE *surf_fopen(const char *name, const char *mode);

extern tmgr_history_t history;
extern xbt_dynar_t surf_path;

void surf_config_init(int *argc, char **argv);
void surf_config_finalize(void);
void surf_config(const char *name, va_list pa);

void net_action_recycle(surf_action_t action);
double net_action_get_remains(surf_action_t action);
#ifdef HAVE_LATENCY_BOUND_TRACKING
int net_get_link_latency_limited(surf_action_t action);
#endif
void net_action_set_max_duration(surf_action_t action, double duration);
/*
 * Returns the initial path. On Windows the initial path is
 * the current directory for the current process in the other
 * case the function returns "./" that represents the current
 * directory on Unix/Linux platforms.
 */
const char *__surf_get_initial_path(void);

/* The __surf_is_absolute_file_path() returns 1 if
 * file_path is a absolute file path, in the other
 * case the function returns 0.
 */
int __surf_is_absolute_file_path(const char *file_path);

/*
 * One link routing list
 */
typedef struct s_onelink {
  char *src;
  char *dst;
  void *link_ptr;
} s_onelink_t, *onelink_t;

/**
 * Routing logic
 */

typedef struct s_model_type {
  const char *name;
  const char *desc;
  void *(*create) ();
  void (*load) ();
  void (*unload) ();
  void (*end) ();
} s_model_type_t, *model_type_t;

typedef struct s_route {
  xbt_dynar_t link_list;
} s_route_t, *route_t;

typedef struct s_route_limits {
  char *src_gateway;
  char *dst_gateway;
} s_route_limits_t, *route_limits_t;

typedef struct s_route_extended {
  s_route_t generic_route;
  char *src_gateway;
  char *dst_gateway;
} s_route_extended_t, *route_extended_t;

/* This enum used in the routing structure helps knowing in which situation we are. */
typedef enum {
  SURF_ROUTING_NULL = 0,   /**< Indefined type                                   */
  SURF_ROUTING_BASE,       /**< Base case: use simple link lists for routing     */
  SURF_ROUTING_RECURSIVE   /**< Recursive case: also return gateway informations */
} e_surf_routing_hierarchy_t;

typedef enum {
  SURF_NETWORK_ELEMENT_NULL = 0,        /* NULL */
  SURF_NETWORK_ELEMENT_HOST,    /* host type */
  SURF_NETWORK_ELEMENT_ROUTER,  /* router type */
  SURF_NETWORK_ELEMENT_AS,      /* AS type */
} e_surf_network_element_type_t;

typedef struct s_routing_component *routing_component_t;
typedef struct s_routing_component {
  xbt_dict_t to_index;			/* char* -> network_element_t */
  xbt_dict_t parse_routes;      /* store data during the parse process */
  xbt_dict_t bypassRoutes;		/* store bypass routes */
  model_type_t routing;
  e_surf_routing_hierarchy_t hierarchy;
  char *name;
  struct s_routing_component *routing_father;
  xbt_dict_t routing_sons;
   route_extended_t(*get_route) (routing_component_t rc, const char *src,
                                 const char *dst);
   xbt_dynar_t(*get_onelink_routes) (routing_component_t rc);
   e_surf_network_element_type_t(*get_network_element_type) (const char
                                                             *name);
   route_extended_t(*get_bypass_route) (routing_component_t rc,
                                        const char *src, const char *dst);
  void (*finalize) (routing_component_t rc);
  void (*set_processing_unit) (routing_component_t rc, const char *name);
  void (*set_autonomous_system) (routing_component_t rc, const char *name);
  void (*set_route) (routing_component_t rc, const char *src,
                     const char *dst, route_t route);
  void (*set_ASroute) (routing_component_t rc, const char *src,
                       const char *dst, route_extended_t route);
  void (*set_bypassroute) (routing_component_t rc, const char *src,
                           const char *dst, route_extended_t e_route);
} s_routing_component_t;

typedef struct s_network_element_info {
  routing_component_t rc_component;
  e_surf_network_element_type_t rc_type;
} s_network_element_info_t, *network_element_info_t;

typedef int *network_element_t;

struct s_routing_global {
  routing_component_t root;
  xbt_dict_t where_network_elements;    /* char* -> network_element_info_t */
  void *loopback;
  size_t size_of_link;
   const xbt_dynar_t(*get_route) (const char *src, const char *dst);
   xbt_dynar_t(*get_route_no_cleanup) (const char *src, const char *dst);
   xbt_dynar_t(*get_onelink_routes) (void);
   e_surf_network_element_type_t(*get_network_element_type) (const char
                                                             *name);
  void (*finalize) (void);
  xbt_dynar_t last_route;
};

XBT_PUBLIC(void) routing_model_create(size_t size_of_link, void *loopback);

/**
 * Resource protected methods
 */
xbt_dict_t surf_resource_properties(const void *resource);

XBT_PUBLIC(void) surfxml_bufferstack_push(int new);
XBT_PUBLIC(void) surfxml_bufferstack_pop(int new);

XBT_PUBLIC_DATA(int) surfxml_bufferstack_size;

#endif                          /* _SURF_SURF_PRIVATE_H */
