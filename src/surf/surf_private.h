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
#include "surf/surfxml_parse_private.h"

#define NO_MAX_DURATION -1.0
#define SG_TCP_CTE_GAMMA 20000.0

extern const char *surf_action_state_names[6];

typedef enum {
  SURF_NETWORK_LINK_ON = 1,	/* Ready        */
  SURF_NETWORK_LINK_OFF = 0	/* Running      */
} e_surf_network_link_state_t;

typedef enum {
  SURF_NETWORK_LINK_SHARED = 1,
  SURF_NETWORK_LINK_FATPIPE = 0
} e_surf_network_link_sharing_policy_t;

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

XBT_PUBLIC double generic_maxmin_share_resources(xbt_swag_t running_actions,
				      size_t offset);
XBT_PUBLIC double generic_maxmin_share_resources2(xbt_swag_t running_actions,
				       size_t offset,
				       lmm_system_t sys);

/* Generic functions common to all ressources */
XBT_PUBLIC e_surf_action_state_t surf_action_get_state(surf_action_t action);
XBT_PUBLIC double surf_action_get_start_time(surf_action_t action);
XBT_PUBLIC double surf_action_get_finish_time(surf_action_t action);
XBT_PUBLIC void surf_action_free(surf_action_t * action);
XBT_PUBLIC void surf_action_change_state(surf_action_t action,
			      e_surf_action_state_t state);
XBT_PUBLIC void surf_action_set_data(surf_action_t action,
			  void *data);
XBT_PUBLIC FILE *surf_fopen(const char *name, const char *mode);

extern lmm_system_t maxmin_system;
extern tmgr_history_t history;
extern xbt_dynar_t surf_path;


/*
 * Returns the initial path. On Windows the initial path is
 * the current directory for the current process in the other
 * case the function returns "./" that represents the current
 * directory on Unix/Linux platforms.
 */
const char* __surf_get_initial_path(void);

/* The __surf_is_absolute_file_path() returns 1 if
 * file_path is a absolute file path, in the other
 * case the function returns 0.
 */
int __surf_is_absolute_file_path(const char* file_path);

#endif				/* _SURF_SURF_PRIVATE_H */
