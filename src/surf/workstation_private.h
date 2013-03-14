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

  /* common with vm */
  s_ws_params_t params;

} s_workstation_CLM03_t, *workstation_CLM03_t;

int ws_action_unref(surf_action_t action);

int ws_resource_used(void *resource_id);
double ws_share_resources(surf_model_t workstation_model, double now);
void ws_update_actions_state(surf_model_t workstation_model, double now, double delta);
void ws_update_resource_state(void *id, tmgr_trace_event_t event_type, double value, double date);
void ws_finalize(surf_model_t workstation_model);

void ws_action_set_priority(surf_action_t action, double priority);

surf_action_t ws_execute(void *workstation, double size);
surf_action_t ws_action_sleep(void *workstation, double duration);
void ws_action_suspend(surf_action_t action);
void ws_action_resume(surf_action_t action);
void ws_action_cancel(surf_action_t action);
e_surf_resource_state_t ws_get_state(void *workstation);
double ws_action_get_remains(surf_action_t action);

void ws_get_params(void *ws, ws_params_t params);
void ws_set_params(void *ws, ws_params_t params);
#endif /* WS_PRIVATE_H_ */
