/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_private.h"
#include "network_private.h"
#include "xbt/mallocator.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_kernel);

/*
 * Generic action
 */

const char *surf_action_state_names[6] = {
  "SURF_ACTION_READY",
  "SURF_ACTION_RUNNING",
  "SURF_ACTION_FAILED",
  "SURF_ACTION_DONE",
  "SURF_ACTION_TO_FREE",
  "SURF_ACTION_NOT_IN_THE_SYSTEM"
};

/* Surf actions mallocator */
static xbt_mallocator_t action_mallocator = NULL;
static int action_mallocator_allocated_size = 0;
static void* surf_action_mallocator_new_f(void);
#define surf_action_mallocator_free_f xbt_free_f
static void surf_action_mallocator_reset_f(void* action);

/**
 * \brief Initializes the action module of Surf.
 */
void surf_action_init(void) {

  /* the action mallocator will always provide actions of the following size,
   * so this size should be set to the maximum size of the surf action structures
   */
  action_mallocator_allocated_size = sizeof(s_surf_action_network_CM02_im_t);
  action_mallocator = xbt_mallocator_new(65536, surf_action_mallocator_new_f,
      surf_action_mallocator_free_f, surf_action_mallocator_reset_f);
}

/**
 * \brief Uninitializes the action module of Surf.
 */
void surf_action_exit(void) {

  xbt_mallocator_free(action_mallocator);
}

static void* surf_action_mallocator_new_f(void) {
  return xbt_malloc(action_mallocator_allocated_size);
}

static void surf_action_mallocator_reset_f(void* action) {
  memset(action, 0, action_mallocator_allocated_size);
}

void *surf_action_new(size_t size, double cost, surf_model_t model,
                      int failed)
{
  xbt_assert(size <= action_mallocator_allocated_size,
      "Cannot create a surf action of size %zu: the mallocator only provides actions of size %d",
      size, action_mallocator_allocated_size);

  surf_action_t action = xbt_mallocator_get(action_mallocator);
  action->refcount = 1;
  action->cost = cost;
  action->remains = cost;
  action->priority = 1.0;
  action->max_duration = NO_MAX_DURATION;
  action->start = surf_get_clock();
  action->finish = -1.0;
  action->model_type = model;
#ifdef HAVE_TRACING
  action->category = NULL;
#endif

  if (failed)
    action->state_set = model->states.failed_action_set;
  else
    action->state_set = model->states.running_action_set;

  xbt_swag_insert(action, action->state_set);

  return action;
}

e_surf_action_state_t surf_action_state_get(surf_action_t action)
{
  surf_action_state_t action_state = &(action->model_type->states);

  if (action->state_set == action_state->ready_action_set)
    return SURF_ACTION_READY;
  if (action->state_set == action_state->running_action_set)
    return SURF_ACTION_RUNNING;
  if (action->state_set == action_state->failed_action_set)
    return SURF_ACTION_FAILED;
  if (action->state_set == action_state->done_action_set)
    return SURF_ACTION_DONE;
  return SURF_ACTION_NOT_IN_THE_SYSTEM;
}

double surf_action_get_start_time(surf_action_t action)
{
  return action->start;
}

double surf_action_get_finish_time(surf_action_t action)
{
  /* keep the function behavior, some models (cpu_ti) change the finish time before the action end */
  return action->remains == 0 ? action->finish : -1;
}

XBT_INLINE void surf_action_free(surf_action_t * action)
{
  xbt_mallocator_release(action_mallocator, *action);
  *action = NULL;
}

void surf_action_state_set(surf_action_t action,
                           e_surf_action_state_t state)
{
  surf_action_state_t action_state = &(action->model_type->states);
  XBT_IN("(%p,%s)", action, surf_action_state_names[state]);
  xbt_swag_remove(action, action->state_set);

  if (state == SURF_ACTION_READY)
    action->state_set = action_state->ready_action_set;
  else if (state == SURF_ACTION_RUNNING)
    action->state_set = action_state->running_action_set;
  else if (state == SURF_ACTION_FAILED)
    action->state_set = action_state->failed_action_set;
  else if (state == SURF_ACTION_DONE)
    action->state_set = action_state->done_action_set;
  else
    action->state_set = NULL;

  if (action->state_set)
    xbt_swag_insert(action, action->state_set);
  XBT_OUT();
}

void surf_action_data_set(surf_action_t action, void *data)
{
  action->data = data;
}

XBT_INLINE void surf_action_ref(surf_action_t action)
{
  action->refcount++;
}

/*
void surf_action_suspend(surf_action_t action)
{
  action->suspended = 1;
}*/

/*
 * Maxmin action
 */
