/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_SURF_PRIVATE_H
#define _SURF_SURF_PRIVATE_H

#include "surf/surf.h"
#include "surf/maxmin.h"
#include "surf/trace_mgr.h"
#include "xbt/log.h"
#include "surf/surf_parse.h"

#define NO_MAX_DURATION -1.0

typedef struct surf_resource_private {
  int (*resource_used) (void *resource_id);
  /* Share the resources to the actions and return in hom much time
     the next action may terminate */
  double (*share_resources) (double now);
  /* Update the actions' state */
  void (*update_actions_state) (double now, double delta);
  void (*update_resource_state) (void *id, tmgr_trace_event_t event_type,
				 double value);
  void (*finalize) (void);
} s_surf_resource_private_t;

/* #define pub2priv(r) ((surf_resource_private_t) ((char *)(r) -(sizeof(struct surf_resource_private_part)))) */
/* #define priv2pub(r) ((void *) ((char *)(r) +(sizeof(struct surf_resource_private_part)))) */

double generic_maxmin_share_resources(xbt_swag_t running_actions,
				      size_t offset);
/* Generic functions common to all ressources */
e_surf_action_state_t surf_action_get_state(surf_action_t action);
void surf_action_free(surf_action_t * action);
void surf_action_change_state(surf_action_t action,
			      e_surf_action_state_t state);
void surf_action_set_data(surf_action_t action,
			  void *data);
FILE *surf_fopen(const char *name, const char *mode);

extern lmm_system_t maxmin_system;
extern tmgr_history_t history;
extern xbt_dynar_t surf_path;

#endif				/* _SURF_SURF_PRIVATE_H */
