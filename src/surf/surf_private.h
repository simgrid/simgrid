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
#include "surf/surfxml_parse.h"
#include "surf/random_mgr.h"
#include "instr/instr_private.h"
#include "surf/surfxml_parse_values.h"

#define NO_MAX_DURATION -1.0

extern xbt_dict_t watched_hosts_lib;

extern const char *surf_action_state_names[6];

typedef enum {
  UM_FULL,
  UM_LAZY,
  UM_UNDEFINED
} e_UM_t;

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

  lmm_system_t maxmin_system;
  e_UM_t update_mechanism;
  xbt_swag_t modified_set;
  xbt_heap_t action_heap;
  int selective_update;

} s_surf_model_private_t;

double generic_maxmin_share_resources(xbt_swag_t running_actions,
                                      size_t offset,
                                      lmm_system_t sys,
                                      void (*solve) (lmm_system_t));
double generic_share_resources_lazy(double now, surf_model_t model);

/* Generic functions common to all models */
void surf_action_init(void);
void surf_action_exit(void);
e_surf_action_state_t surf_action_state_get(surf_action_t action);      /* cannot declare inline since we use a pointer to it */
double surf_action_get_start_time(surf_action_t action);        /* cannot declare inline since we use a pointer to it */
double surf_action_get_finish_time(surf_action_t action);       /* cannot declare inline since we use a pointer to it */
void surf_action_free(surf_action_t * action);
void surf_action_state_set(surf_action_t action,
                           e_surf_action_state_t state);
void surf_action_data_set(surf_action_t action, void *data);    /* cannot declare inline since we use a pointer to it */

void surf_action_lmm_update_index_heap(void *action, int i); /* callback for heap management shared by cpu and net models */
void surf_action_lmm_heap_insert(xbt_heap_t heap, surf_action_lmm_t action,
    double key, enum heap_action_type hat);
void surf_action_lmm_heap_remove(xbt_heap_t heap,surf_action_lmm_t action);

void surf_action_cancel(surf_action_t action);
int surf_action_unref(surf_action_t action);
void surf_action_suspend(surf_action_t action);
void surf_action_resume(surf_action_t action);
int surf_action_is_suspended(surf_action_t action);
void surf_action_set_max_duration(surf_action_t action, double duration);
void surf_action_set_priority(surf_action_t action, double priority);
#ifdef HAVE_TRACING
void surf_action_set_category(surf_action_t action,
                                    const char *category);
#endif
double surf_action_get_remains(surf_action_t action);
void generic_update_action_remaining_lazy( surf_action_lmm_t action, double now);
void generic_update_actions_state_lazy(double now, double delta, surf_model_t model);
void generic_update_actions_state_full(double now, double delta, surf_model_t model);

FILE *surf_fopen(const char *name, const char *mode);

extern tmgr_history_t history;

//void surf_config_init(int *argc, char **argv);
//void surf_config_finalize(void);
//void surf_config(const char *name, va_list pa);

void net_action_recycle(surf_action_t action);
#ifdef HAVE_LATENCY_BOUND_TRACKING
int net_get_link_latency_limited(surf_action_t action);
#endif

/* The __surf_is_absolute_file_path() returns 1 if
 * file_path is a absolute file path, in the other
 * case the function returns 0.
 */
int __surf_is_absolute_file_path(const char *file_path);

typedef struct s_routing_edge {
  AS_t rc_component;
  e_surf_network_element_type_t rc_type;
  int id;
  char *name;
} s_routing_edge_t;

/*
 * Link of lenght 1, alongside with its source and destination. This is mainly usefull in the bindings to gtnets and ns3
 */
typedef struct s_onelink {
  sg_routing_edge_t src;
  sg_routing_edge_t dst;
  void *link_ptr;
} s_onelink_t, *onelink_t;

/**
 * Routing logic
 */

typedef struct s_model_type {
  const char *name;
  const char *desc;
  AS_t (*create) ();
  void (*end) (AS_t as);
} s_routing_model_description_t, *routing_model_description_t;

/* This enum used in the routing structure helps knowing in which situation we are. */
typedef enum {
  SURF_ROUTING_NULL = 0,   /**< Undefined type                                   */
  SURF_ROUTING_BASE,       /**< Base case: use simple link lists for routing     */
  SURF_ROUTING_RECURSIVE   /**< Recursive case: also return gateway informations */
} e_surf_routing_hierarchy_t;

typedef struct s_as {
  xbt_dynar_t index_network_elm;
  xbt_dict_t bypassRoutes;    /* store bypass routes */
  routing_model_description_t model_desc;
  e_surf_routing_hierarchy_t hierarchy;
  char *name;
  struct s_as *routing_father;
  xbt_dict_t routing_sons;
  sg_routing_edge_t net_elem;
  xbt_dynar_t link_up_down_list;

  void (*get_route_and_latency) (AS_t as, sg_routing_edge_t src, sg_routing_edge_t dst, sg_platf_route_cbarg_t into, double *latency);

  xbt_dynar_t(*get_onelink_routes) (AS_t as);
  void (*get_graph) (xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges, AS_t rc);
  sg_platf_route_cbarg_t(*get_bypass_route) (AS_t as, sg_routing_edge_t src, sg_routing_edge_t dst, double *lat);
  void (*finalize) (AS_t as);


  /* The parser calls the following functions to inform the routing models
   * that a new element is added to the AS currently built.
   *
   * Of course, only the routing model of this AS is informed, not every ones */
  int (*parse_PU) (AS_t as, sg_routing_edge_t elm); /* A host or a router, whatever */
  int (*parse_AS) (AS_t as, sg_routing_edge_t elm);
  void (*parse_route) (AS_t as, sg_platf_route_cbarg_t route);
  void (*parse_ASroute) (AS_t as, sg_platf_route_cbarg_t route);
  void (*parse_bypassroute) (AS_t as, sg_platf_route_cbarg_t e_route);

} s_as_t;

struct s_routing_platf {
  AS_t root;
  void *loopback;
  xbt_dynar_t last_route;
  xbt_dynar_t(*get_onelink_routes) (void);
};

XBT_PUBLIC(void) routing_model_create(void *loopback);
XBT_PUBLIC(void) routing_exit(void);
XBT_PUBLIC(void) storage_register_callbacks(void);
/* ***************************************** */
/* TUTORIAL: New TAG                         */
XBT_PUBLIC(void) gpu_register_callbacks(void);
/* ***************************************** */

XBT_PUBLIC(void) routing_register_callbacks(void);
XBT_PUBLIC(void) generic_free_route(sg_platf_route_cbarg_t route); // FIXME rename to routing_route_free
 // FIXME: make previous function private to routing again?


XBT_PUBLIC(void) routing_get_route_and_latency(sg_routing_edge_t src, sg_routing_edge_t dst,
                              xbt_dynar_t * route, double *latency);

XBT_PUBLIC(void) generic_get_graph(xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges, AS_t rc);
/**
 * Resource protected methods
 */
XBT_PUBLIC(void) surfxml_bufferstack_push(int new);
XBT_PUBLIC(void) surfxml_bufferstack_pop(int new);

XBT_PUBLIC_DATA(int) surfxml_bufferstack_size;

/********** Tracing **********/
/* from surf_instr.c */
void TRACE_surf_host_set_power(double date, const char *resource, double power);
void TRACE_surf_link_set_bandwidth(double date, const char *resource, double bandwidth);


#endif                          /* _SURF_SURF_PRIVATE_H */
