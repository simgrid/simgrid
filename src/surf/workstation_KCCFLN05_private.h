/* 	$Id$	 */

/* Copyright (c) 2005 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_WORKSTATION_KCCFLN05_PRIVATE_H
#define _SURF_WORKSTATION_KCCFLN05_PRIVATE_H

#include "surf_private.h"
#include "network_private.h"

/**************************************/
/********* workstation object *********/
/**************************************/
typedef struct workstation_KCCFLN05 {
  surf_resource_t resource;	/* Any such object, added in a trace
				   should start by this field!!! */
  char *name;
  double power_scale;
  double power_current;
  double interference_send;
  double interference_recv;
  double interference_send_recv;
  tmgr_trace_event_t power_event;
  e_surf_cpu_state_t state_current;
  tmgr_trace_event_t state_event;
  lmm_constraint_t constraint;
  int id; /* cpu and network card are a single object... */
  xbt_dynar_t incomming_communications;
  xbt_dynar_t outgoing_communications;
} s_workstation_KCCFLN05_t, *workstation_KCCFLN05_t;

/**************************************/
/*********** network object ***********/
/**************************************/

typedef struct network_link_KCCFLN05 {
  surf_resource_t resource;	/* Any such object, added in a trace
				   should start by this field!!! */
  char *name;
  double bw_current;
  tmgr_trace_event_t bw_event;
  e_surf_network_link_state_t state_current;
  tmgr_trace_event_t state_event;
  lmm_constraint_t constraint;
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

typedef struct surf_action_cpu_KCCFLN05 {
  s_surf_action_t generic_action;
  lmm_variable_t variable;
} s_surf_action_cpu_KCCFLN05_t, *surf_action_cpu_KCCFLN05_t;

typedef struct surf_action_network_KCCFLN05 {
  s_surf_action_t generic_action;
  lmm_variable_t variable;
  workstation_KCCFLN05_t src;
  workstation_KCCFLN05_t dst;
} s_surf_action_network_KCCFLN05_t, *surf_action_network_KCCFLN05_t;


#endif				/* _SURF_WORKSTATION_KCCFLN05_PRIVATE_H */
