/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_NETWORK_PRIVATE_H
#define _SURF_NETWORK_PRIVATE_H

#include "surf_private.h"
#include "xbt/dict.h"

typedef enum {
  SURF_NETWORK_LINK_ON = 1,	/* Ready        */
  SURF_NETWORK_LINK_OFF = 0	/* Running      */
} e_surf_network_link_state_t;

typedef struct network_link {
  surf_resource_t resource;	/* Any such object, added in a trace
				   should start by this field!!! */
  /* Using this object with the public part of
     resource does not make sense */
  const char *name;
  double bw_current;
  tmgr_trace_event_t bw_event;
  double lat_current;
  tmgr_trace_event_t lat_event;
  e_surf_network_link_state_t state_current;
  tmgr_trace_event_t state_event;
  lmm_constraint_t constraint;
} s_network_link_t, *network_link_t;


typedef struct network_card {
  const char *name;
  int id;
} s_network_card_t, *network_card_t;

typedef struct surf_action_network {
  s_surf_action_t generic_action;
  double latency;
  double lat_current;
  lmm_variable_t variable;
  network_card_t src;
  network_card_t dst;
} s_surf_action_network_t, *surf_action_network_t;

extern xbt_dict_t network_card_set;

#endif				/* _SURF_NETWORK_PRIVATE_H */
