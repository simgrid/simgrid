/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_NETWORK_PRIVATE_H
#define _SURF_NETWORK_PRIVATE_H

#include "surf_private.h"

typedef struct surf_action_network {
  s_surf_action_t generic_action;
  lmm_variable_t variable;
} s_surf_action_network_t, *surf_action_network_t;

typedef struct network {
  surf_resource_t resource;   /* Any such object, added in a trace
				 should start by this field!!! */
  const char *name;
} s_network_t, *network_t;


#endif				/* _SURF_NETWORK_PRIVATE_H */
