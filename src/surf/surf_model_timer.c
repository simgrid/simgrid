/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "surf_private.h"
#include "xbt/dict.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_timer, surf,
                                "Logging specific to SURF (timer)");

typedef struct surf_action_timer {
  s_surf_action_t generic_action;
} s_surf_action_timer_t, *surf_action_timer_t;

typedef struct command {
  s_surf_resource_t generic_resource;   /* Must remain first, since we add this to a trace */
  void *function;
  void *args;
  s_xbt_swag_hookup_t command_set_hookup;
  surf_action_timer_t action;
} s_command_t, *command_t;


surf_model_t surf_timer_model = NULL;
static tmgr_trace_t empty_trace = NULL;
static xbt_swag_t command_pending = NULL;
static xbt_swag_t command_to_run = NULL;
static xbt_heap_t timer_heap = NULL;

static command_t command_new(void *fun, void *args)
{
  command_t command = xbt_new0(s_command_t, 1);

  command->generic_resource.model = surf_timer_model;
  command->function = fun;
  command->args = args;
  xbt_swag_insert(command, command_pending);
  command->action =
    surf_action_new(sizeof(s_surf_action_timer_t), 0, surf_timer_model, 0);
  return command;
}

static void command_free(command_t command)
{
  if (xbt_swag_belongs(command, command_to_run)) {
    xbt_swag_remove(command, command_to_run);
  } else if (xbt_swag_belongs(command, command_pending)) {
    xbt_swag_remove(command, command_pending);
  }
  xbt_swag_remove(command->action,
                  surf_timer_model->states.running_action_set);
  xbt_free(command->action);
  surf_resource_free((surf_resource_t) command);
  return;
}

static void parse_timer(void)
{
}

static void parse_file(const char *file)
{
}

static int resource_used(void *resource_id)
{
  return 1;
}

static void timer_action_state_set(surf_action_t action,
                                   e_surf_action_state_t state)
{
  DIE_IMPOSSIBLE;
  return;
}

static double share_resources(double now)
{
  if (xbt_heap_size(timer_heap)) {
    DEBUG1("Share resoure returning %lf", xbt_heap_maxkey(timer_heap));
    return (xbt_heap_maxkey(timer_heap));
  } else
    return -1.0;
}

static void update_actions_state(double now, double delta)
{
  while ((xbt_heap_size(timer_heap)) > 0
         && (xbt_heap_maxkey(timer_heap) <= now + delta))
    xbt_heap_pop(timer_heap);
  return;
}

static void update_resource_state(void *id,
                                  tmgr_trace_event_t event_type,
                                  double value, double date)
{
  command_t command = id;

  /* Move this command to the list of commands to execute */
  xbt_swag_remove(command, command_pending);
  xbt_swag_insert(command, command_to_run);
  tmgr_trace_event_free(event_type);

  DEBUG1("Insert command on date %lf", date);

  return;
}

static void set(double date, void *function, void *arg)
{
  command_t command = NULL;

  command = command_new(function, arg);

  tmgr_history_add_trace(history, empty_trace, date, 0, command);
  xbt_heap_push(timer_heap, NULL, date);

  DEBUG1("Putting value %lf on heap", date);
}


static int get(void **function, void **arg)
{
  command_t command = NULL;

  command = xbt_swag_extract(command_to_run);
  if (command) {
    *function = command->function;
    *arg = command->args;
    command_free(command);
    return 1;
  } else {
    return 0;
  }
}

static void action_suspend(surf_action_t action)
{
  DIE_IMPOSSIBLE;
}

static void action_resume(surf_action_t action)
{
  DIE_IMPOSSIBLE;
}

static int action_is_suspended(surf_action_t action)
{
  DIE_IMPOSSIBLE;
  return 0;
}

static void finalize(void)
{
  xbt_heap_free(timer_heap);
  timer_heap = NULL;

  tmgr_trace_free(empty_trace);
  empty_trace = NULL;

  xbt_swag_free(command_pending);
  xbt_swag_free(command_to_run);

  surf_model_exit(surf_timer_model);
  surf_timer_model = NULL;
}

static void surf_timer_model_init_internal(void)
{
  surf_timer_model = surf_model_init();

  surf_timer_model->name = "TIMER";
  surf_timer_model->action_state_set = timer_action_state_set;

  surf_timer_model->model_private->resource_used = resource_used;
  surf_timer_model->model_private->share_resources = share_resources;
  surf_timer_model->model_private->update_actions_state =
    update_actions_state;
  surf_timer_model->model_private->update_resource_state =
    update_resource_state;
  surf_timer_model->model_private->finalize = finalize;

  surf_timer_model->suspend = action_suspend;
  surf_timer_model->resume = action_resume;
  surf_timer_model->is_suspended = action_is_suspended;

  surf_timer_model->extension.timer.set = set;
  surf_timer_model->extension.timer.get = get;

  {
    s_command_t var;
    command_pending = xbt_swag_new(xbt_swag_offset(var, command_set_hookup));
    command_to_run = xbt_swag_new(xbt_swag_offset(var, command_set_hookup));
  }

  empty_trace = tmgr_empty_trace_new();
  timer_heap = xbt_heap_new(8, NULL);
}

void surf_timer_model_init(const char *filename)
{
  if (surf_timer_model)
    return;
  surf_timer_model_init_internal();
  xbt_dynar_push(model_list, &surf_timer_model);
}
