/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_SURF_PRIVATE_H
#define _SURF_SURF_PRIVATE_H

#include "surf/surf.h"
#include "surf/maxmin.h"
#include "surf/trace_mgr.h"
#include "xbt/log.h"

typedef struct surf_resource_private {
  /* Share the resources to the actions and return in hom much time
      the next action may terminate */
  xbt_heap_float_t(*share_resources) (xbt_heap_float_t now);	
  /* Update the actions' state */
  void (*update_actions_state) (xbt_heap_float_t now, xbt_heap_float_t delta);
  void (*update_resource_state) (void *id,tmgr_trace_event_t event_type, xbt_maxmin_float_t value);
  void (*finalize)(void);
} s_surf_resource_private_t;

/* #define pub2priv(r) ((surf_resource_private_t) ((char *)(r) -(sizeof(struct surf_resource_private_part)))) */
/* #define priv2pub(r) ((void *) ((char *)(r) +(sizeof(struct surf_resource_private_part)))) */

/* Generic functions common to all ressources */
e_surf_action_state_t surf_action_get_state(surf_action_t action);
void surf_action_free(surf_action_t * action);
void surf_action_change_state(surf_action_t action, e_surf_action_state_t state);

extern xbt_dynar_t resource_list;
extern lmm_system_t maxmin_system;
extern tmgr_history_t history;

#endif				/* _SURF_SURF_PRIVATE_H */
