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
#include "surf/random_mgr.h"

#define NO_MAX_DURATION -1.0
extern double sg_tcp_gamma;

extern const char *surf_action_state_names[6];

typedef enum {
  SURF_LINK_SHARED = 1,
  SURF_LINK_FATPIPE = 0
} e_surf_link_sharing_policy_t;

typedef struct surf_model_private {
  int (*resource_used) (void *resource_id);
  /* Share the resources to the actions and return in how much time
     the next action may terminate */
  double (*share_resources) (double now);
  /* Update the actions' state */
  void (*update_actions_state) (double now, double delta);
  void (*update_resource_state) (void *id, tmgr_trace_event_t event_type,
                                 double value, double time);
  void (*finalize) (void);
} s_surf_model_private_t;

double generic_maxmin_share_resources(xbt_swag_t running_actions,
                                      size_t offset,
                                      lmm_system_t sys,
                                      void (*solve) (lmm_system_t));

/* Generic functions common to all models */
e_surf_action_state_t surf_action_state_get(surf_action_t action);
double surf_action_get_start_time(surf_action_t action);
double surf_action_get_finish_time(surf_action_t action);
void surf_action_free(surf_action_t * action);
void surf_action_state_set(surf_action_t action,
                              e_surf_action_state_t state);
void surf_action_data_set(surf_action_t action, void *data);
FILE *surf_fopen(const char *name, const char *mode);

extern tmgr_history_t history;
extern xbt_dynar_t surf_path;

void surf_config_init(int *argc, char **argv);
void surf_config_finalize(void);
void surf_config(const char *name, va_list pa);


/*
 * Returns the initial path. On Windows the initial path is
 * the current directory for the current process in the other
 * case the function returns "./" that represents the current
 * directory on Unix/Linux platforms.
 */
const char *__surf_get_initial_path(void);

/* The __surf_is_absolute_file_path() returns 1 if
 * file_path is a absolute file path, in the other
 * case the function returns 0.
 */
int __surf_is_absolute_file_path(const char *file_path);

/*
 * Routing logic
 */
struct s_routing {
  const char *name;
  xbt_dict_t host_id; /* char* -> int* */

  xbt_dynar_t (*get_route)(int src, int dst);
  void (*finalize)(void);
  int host_count;
};
XBT_PUBLIC(void) routing_model_create(size_t size_of_link,void *loopback);


/*
 * Resource protected methods
 */
XBT_PUBLIC(xbt_dict_t) surf_resource_properties(const void *resource);

#endif /* _SURF_SURF_PRIVATE_H */
