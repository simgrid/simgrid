/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012. The SimGrid Team.
 * All rights reserved.                                                                 */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package.             */

/* ********************************************************************* */
/* TUTORIAL: New model                                                   */
/* ********************************************************************* */

#include "xbt/ex.h"
#include "xbt/dict.h"
#include "xbt/file_stat.h"
#include "portable.h"
#include "surf_private.h"
#include "new_model_private.h"
#include "surf/surf_resource.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_new_model, surf,
                                "Logging specific to the SURF new model module");

surf_model_t surf_new_model = NULL;
lmm_system_t new_model_maxmin_system = NULL;
static int new_model_selective_update = 0;
static xbt_swag_t
    new_model_running_action_set_that_does_not_need_being_checked = NULL;

#define GENERIC_LMM_ACTION(action) action->generic_lmm_action
#define GENERIC_ACTION(action) GENERIC_LMM_ACTION(action).generic_action

static void new_model_action_state_set(surf_action_t action, e_surf_action_state_t state);
static surf_action_t new_model_action_execute ();

static surf_action_t new_model_action_fct()
{
  surf_action_t action = new_model_action_execute();
  return action;
}

static surf_action_t new_model_action_execute ()
{
  THROW_UNIMPLEMENTED;
  return NULL;
}

static void* new_model_create_resource(const char* id, const char* model,const char* type_id,const char* content_name)
{
  THROW_UNIMPLEMENTED;
  return NULL;
}

static void new_model_finalize(void)
{
  THROW_UNIMPLEMENTED;
}

static void new_model_update_actions_state(double now, double delta)
{
  THROW_UNIMPLEMENTED;
}

static double new_model_share_resources(double NOW)
{
  THROW_UNIMPLEMENTED;
  return 0;
}

static int new_model_resource_used(void *resource_id)
{
  THROW_UNIMPLEMENTED;
  return 0;
}

static void new_model_resources_state(void *id, tmgr_trace_event_t event_type,
                                 double value, double time)
{
  THROW_UNIMPLEMENTED;
}

static int new_model_action_unref(surf_action_t action)
{
  THROW_UNIMPLEMENTED;
  return 0;
}

static void new_model_action_cancel(surf_action_t action)
{
  surf_action_state_set(action, SURF_ACTION_FAILED);
  return;
}

static void new_model_action_state_set(surf_action_t action, e_surf_action_state_t state)
{
  surf_action_state_set(action, state);
  return;
}

static void new_model_action_suspend(surf_action_t action)
{
  XBT_IN("(%p)", action);
  if (((surf_action_lmm_t) action)->suspended != 2) {
    lmm_update_variable_weight(new_model_maxmin_system,
                               ((surf_action_lmm_t) action)->variable,
                               0.0);
    ((surf_action_lmm_t) action)->suspended = 1;
  }
  XBT_OUT();
}

static void new_model_action_resume(surf_action_t action)
{
  THROW_UNIMPLEMENTED;
}

static int new_model_action_is_suspended(surf_action_t action)
{
  return (((surf_action_lmm_t) action)->suspended == 1);
}

static void new_model_action_set_max_duration(surf_action_t action, double duration)
{
  THROW_UNIMPLEMENTED;
}

static void new_model_action_set_priority(surf_action_t action, double priority)
{
  THROW_UNIMPLEMENTED;
}

static void new_model_define_callbacks()
{
}

static void surf_new_model_model_init_internal(void)
{
  s_surf_action_t action;

  XBT_DEBUG("surf_new_model_model_init_internal");
  surf_new_model = surf_model_init();

  new_model_running_action_set_that_does_not_need_being_checked =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));

  surf_new_model->name = "Storage";
  surf_new_model->action_unref = new_model_action_unref;
  surf_new_model->action_cancel = new_model_action_cancel;
  surf_new_model->action_state_set = new_model_action_state_set;

  surf_new_model->model_private->finalize = new_model_finalize;
  surf_new_model->model_private->update_actions_state = new_model_update_actions_state;
  surf_new_model->model_private->share_resources = new_model_share_resources;
  surf_new_model->model_private->resource_used = new_model_resource_used;
  surf_new_model->model_private->update_resource_state = new_model_resources_state;

  surf_new_model->suspend = new_model_action_suspend;
  surf_new_model->resume = new_model_action_resume;
  surf_new_model->is_suspended = new_model_action_is_suspended;
  surf_new_model->set_max_duration = new_model_action_set_max_duration;
  surf_new_model->set_priority = new_model_action_set_priority;

  surf_new_model->extension.new_model.fct = new_model_action_fct;
  surf_new_model->extension.new_model.create_resource = new_model_create_resource;

  if (!new_model_maxmin_system) {
    new_model_maxmin_system = lmm_system_new(new_model_selective_update);
  }

}

void surf_new_model_model_init_default(void)
{
  surf_new_model_model_init_internal();
  new_model_define_callbacks();

  xbt_dynar_push(model_list, &surf_new_model);
}
