/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_HOST_PRIVATE_H
#define _SURF_HOST_PRIVATE_H

#include "surf/surf.h"
#include "surf/surf_private.h"

typedef struct surf_action_host {
  s_surf_action_t generic_action;
  lmm_variable_t variable;
} s_surf_action_host_t, *surf_action_host_t;

surf_host_resource_t surf_host_resource_init(void);

#endif				/* _SURF_SURF_PRIVATE_H */
