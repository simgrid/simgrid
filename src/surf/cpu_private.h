/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_CPU_PRIVATE_H
#define _SURF_CPU_PRIVATE_H

#include "surf/surf.h"
#include "surf/surf_private.h"

typedef struct surf_action_cpu {
  s_surf_action_t generic_action;
  lmm_variable_t variable;
} s_surf_action_cpu_t, *surf_action_cpu_t;

surf_cpu_resource_t surf_cpu_resource_init(void);

#endif				/* _SURF_SURF_PRIVATE_H */
