/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_SURF_PRIVATE_H
#define _SURF_SURF_PRIVATE_H

#include "surf/surf.h"
#include "surf/maxmin.h"
#include "surf/trace_mgr.h"
#include "cpu_private.h"

/* Generic functions common to all ressources */
e_surf_action_state_t surf_action_get_state(surf_action_t action);
void surf_action_free(surf_action_t * action);
void surf_action_change_state(surf_action_t action, e_surf_action_state_t state);



#endif				/* _SURF_SURF_PRIVATE_H */
