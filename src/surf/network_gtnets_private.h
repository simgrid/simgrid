/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_NETWORK_GTNETS_PRIVATE_H
#define _SURF_NETWORK_GTNETS_PRIVATE_H

#include "surf_private.h"
#include "xbt/dict.h"

typedef struct network_link_GTNETS {
  s_surf_resource_t generic_resource; /* Must remain first to allow casting */
  /* Using this object with the public part of
     model does not make sense */
  double bw_current;
  double lat_current;
  int id;
} s_network_link_GTNETS_t, *network_link_GTNETS_t;

typedef struct network_card_GTNETS {
  char *name;
  int id;
} s_network_card_GTNETS_t, *network_card_GTNETS_t;

typedef struct surf_action_network_GTNETS {
  s_surf_action_t generic_action;
  double latency;
  double lat_current;
  lmm_variable_t variable;
  double rate;
  int suspended;
} s_surf_action_network_GTNETS_t, *surf_action_network_GTNETS_t;

//extern xbt_dict_t network_card_set;


#endif /* _SURF_NETWORK_PRIVATE_H */
