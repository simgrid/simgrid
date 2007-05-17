/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_NETWORK_GTNETS_PRIVATE_H
#define _SURF_NETWORK_GTNETS_PRIVATE_H

#include "surf_private.h"
#include "xbt/dict.h"

typedef struct network_link_GTNETS {
  surf_resource_t resource;	/* Any such object, added in a trace
				   should start by this field!!! */
  /* Using this object with the public part of
     resource does not make sense */
  char *name;
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
  network_card_GTNETS_t src;
  network_card_GTNETS_t dst;
} s_surf_action_network_GTNETS_t, *surf_action_network_GTNETS_t;

extern xbt_dict_t network_card_set;

/* HC: I put this prototype here for now but it will have to go in 
   src/include/surf.h when it is functionnal. */
void surf_network_resource_init_GTNETS(const char *filename);


#endif				/* _SURF_NETWORK_PRIVATE_H */


