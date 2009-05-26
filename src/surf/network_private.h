/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_NETWORK_PRIVATE_H
#define _SURF_NETWORK_PRIVATE_H

#include "surf_private.h"
#include "network_common.h"
#include "xbt/dict.h"

typedef struct network_link_CM02 {
  surf_model_t model;           /* Any such object, added in a trace
                                   should start by this field!!! */
  xbt_dict_t properties;
  /* Using this object with the public part of
     model does not make sense */
  char *name;
  double bw_current;
  tmgr_trace_event_t bw_event;
  double lat_current;
  tmgr_trace_event_t lat_event;
  e_surf_link_state_t state_current;
  tmgr_trace_event_t state_event;
  lmm_constraint_t constraint;
} s_link_CM02_t, *link_CM02_t;


typedef struct network_card_CM02 {
  char *name;
  int id;
} s_network_card_CM02_t, *network_card_CM02_t;

typedef struct surf_action_network_CM02 {
  s_surf_action_t generic_action;
  double latency;
  double lat_current;
  double weight;
  lmm_variable_t variable;
  double rate;
  int suspended;
  network_card_CM02_t src;
  network_card_CM02_t dst;
} s_surf_action_network_CM02_t, *surf_action_network_CM02_t;

extern int card_number;
extern link_CM02_t **routing_table;
extern int *routing_table_size;

#define ROUTE(i,j) routing_table[(i)+(j)*host_number]
#define ROUTE_SIZE(i,j) routing_table_size[(i)+(j)*host_number]

#endif /* _SURF_NETWORK_PRIVATE_H */
