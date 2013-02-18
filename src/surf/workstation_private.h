/* Copyright (c) 2009, 2013. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef WS_PRIVATE_H_
#define WS_PRIVATE_H_
typedef struct workstation_CLM03 {
  s_surf_resource_t generic_resource;   /* Must remain first to add this to a trace */
  void *net_elm;
  xbt_dynar_t storage;
} s_workstation_CLM03_t, *workstation_CLM03_t;

void __init_ws(workstation_CLM03_t ws,  const char *id, int level);

int ws_resource_used(void *resource_id);
double ws_share_resources(surf_model_t workstation_model, double now);
void ws_update_actions_state(surf_model_t workstation_model, double now, double delta);
void ws_update_resource_state(void *id, tmgr_trace_event_t event_type, double value, double date);
void ws_finalize(surf_model_t workstation_model);

#endif /* WS_PRIVATE_H_ */
