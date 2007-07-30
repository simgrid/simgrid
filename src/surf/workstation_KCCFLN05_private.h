/* 	$Id$	 */

/* Copyright (c) 2005 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_WORKSTATION_KCCFLN05_PRIVATE_H
#define _SURF_WORKSTATION_KCCFLN05_PRIVATE_H

#include "surf_private.h"

typedef enum {
  SURF_WORKSTATION_RESOURCE_CPU,
  SURF_WORKSTATION_RESOURCE_LINK,
} e_surf_workstation_resource_type_t;


/**************************************/
/********* router object **************/
/**************************************/
typedef struct router_KCCFLN05 {
  char *name;		
  unsigned int id;
} s_router_KCCFLN05_t, *router_KCCFLN05_t;


/**************************************/
/********* cpu object *****************/
/**************************************/
typedef struct cpu_KCCFLN05 {
  surf_resource_t resource;
  e_surf_workstation_resource_type_t type;	/* Do not move this field */
  char *name;					/* Do not move this field */
  lmm_constraint_t constraint;
  lmm_constraint_t bus;
  double power_scale;
  double power_current;
  double interference_send;
  double interference_recv;
  double interference_send_recv;
  tmgr_trace_event_t power_event;
  e_surf_cpu_state_t state_current;
  tmgr_trace_event_t state_event;
  int id;			/* cpu and network card are a single object... */
  xbt_dynar_t incomming_communications;
  xbt_dynar_t outgoing_communications;
} s_cpu_KCCFLN05_t, *cpu_KCCFLN05_t;

/**************************************/
/*********** network object ***********/
/**************************************/

typedef struct network_link_KCCFLN05 {
  surf_resource_t resource;
  e_surf_workstation_resource_type_t type;	/* Do not move this field */
  char *name;					/* Do not move this field */
  lmm_constraint_t constraint;
  double lat_current;
  tmgr_trace_event_t lat_event;
  double bw_current;
  tmgr_trace_event_t bw_event;
  e_surf_network_link_state_t state_current;
  tmgr_trace_event_t state_event;
} s_network_link_KCCFLN05_t, *network_link_KCCFLN05_t;


typedef struct s_route_KCCFLN05 {
  double impact_on_src;
  double impact_on_dst;
  double impact_on_src_with_other_recv;
  double impact_on_dst_with_other_send;
  network_link_KCCFLN05_t *links;
  int size;
} s_route_KCCFLN05_t, *route_KCCFLN05_t;

/**************************************/
/*************** actions **************/
/**************************************/
typedef struct surf_action_workstation_KCCFLN05 {
  s_surf_action_t generic_action;
  double latency;
  double lat_current;
  lmm_variable_t variable;
  double rate;
  int suspended;
  cpu_KCCFLN05_t src;		/* could be avoided */
  cpu_KCCFLN05_t dst;		/* could be avoided */
} s_surf_action_workstation_KCCFLN05_t,
  *surf_action_workstation_KCCFLN05_t;




#endif				/* _SURF_WORKSTATION_KCCFLN05_PRIVATE_H */
