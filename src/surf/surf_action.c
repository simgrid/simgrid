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
  action_mallocator_allocated_size = sizeof(s_surf_action_network_CM02_t);
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
 * Maxmin action
 */

/* added to manage the communication action's heap */
void surf_action_lmm_update_index_heap(void *action, int i) {
  surf_action_lmm_t a = action;
  a->index_heap = i;
}
/* insert action on heap using a given key and a hat (heap_action_type)
 * a hat can be of three types for communications:
 *
 * NORMAL = this is a normal heap entry stating the date to finish transmitting
 * LATENCY = this is a heap entry to warn us when the latency is payed
 * MAX_DURATION =this is a heap entry to warn us when the max_duration limit is reached
 */
void surf_action_lmm_heap_insert(xbt_heap_t heap, surf_action_lmm_t action, double key,
    enum heap_action_type hat)
{
  action->hat = hat;
  xbt_heap_push(heap, action, key);
}

void surf_action_lmm_heap_remove(xbt_heap_t heap, surf_action_lmm_t action)
{
  action->hat = NOTSET;
  if (action->index_heap >= 0) {
    xbt_heap_remove(heap, action->index_heap);
  }
}

void surf_action_cancel(surf_action_t action)
{
  surf_model_t model = action->model_type;
  surf_action_state_set(action, SURF_ACTION_FAILED);
  if (model->model_private->update_mechanism == UM_LAZY) {
    xbt_swag_remove(action, model->model_private->modified_set);
    surf_action_lmm_heap_remove(model->model_private->action_heap,(surf_action_lmm_t)action);
  }
  return;
}

int surf_action_unref(surf_action_t action)
{
  surf_model_t model = action->model_type;
  action->refcount--;
  if (!action->refcount) {
    xbt_swag_remove(action, action->state_set);
    if (((surf_action_lmm_t) action)->variable)
      lmm_variable_free(model->model_private->maxmin_system,
                        ((surf_action_lmm_t) action)->variable);
    if (model->model_private->update_mechanism == UM_LAZY) {
      /* remove from heap */
      surf_action_lmm_heap_remove(model->model_private->action_heap,(surf_action_lmm_t)action);
      xbt_swag_remove(action, model->model_private->modified_set);
    }
#ifdef HAVE_TRACING
    xbt_free(action->category);
#endif
    surf_action_free(&action);
    return 1;
  }
  return 0;
}

void surf_action_suspend(surf_action_t action)
{
  surf_model_t model = action->model_type;
  XBT_IN("(%p)", action);
  if (((surf_action_lmm_t) action)->suspended != 2) {
    lmm_update_variable_weight(model->model_private->maxmin_system,
                               ((surf_action_lmm_t) action)->variable,
                               0.0);
    ((surf_action_lmm_t) action)->suspended = 1;
    if (model->model_private->update_mechanism == UM_LAZY)
      surf_action_lmm_heap_remove(model->model_private->action_heap,(surf_action_lmm_t)action);
  }
  XBT_OUT();
}

void surf_action_resume(surf_action_t action)
{
  surf_model_t model = action->model_type;
  XBT_IN("(%p)", action);
  if (((surf_action_lmm_t) action)->suspended != 2) {
    lmm_update_variable_weight(model->model_private->maxmin_system,
                               ((surf_action_lmm_t) action)->variable,
                               action->priority);
    ((surf_action_lmm_t) action)->suspended = 0;
    if (model->model_private->update_mechanism == UM_LAZY)
      surf_action_lmm_heap_remove(model->model_private->action_heap,(surf_action_lmm_t)action);
  }
  XBT_OUT();
}

int surf_action_is_suspended(surf_action_t action)
{
  return (((surf_action_lmm_t) action)->suspended == 1);
}

void surf_action_set_max_duration(surf_action_t action, double duration)
{
  surf_model_t model = action->model_type;
  XBT_IN("(%p,%g)", action, duration);
  action->max_duration = duration;
  if (model->model_private->update_mechanism == UM_LAZY)      // remove action from the heap
    surf_action_lmm_heap_remove(model->model_private->action_heap,(surf_action_lmm_t)action);
  XBT_OUT();
}

void surf_action_set_priority(surf_action_t action, double priority)
{
  surf_model_t model = action->model_type;
  XBT_IN("(%p,%g)", action, priority);
  action->priority = priority;
  lmm_update_variable_weight(model->model_private->maxmin_system,
                             ((surf_action_lmm_t) action)->variable,
                             priority);

  if (model->model_private->update_mechanism == UM_LAZY)
    surf_action_lmm_heap_remove(model->model_private->action_heap,(surf_action_lmm_t)action);
  XBT_OUT();
}

#ifdef HAVE_TRACING
void surf_action_set_category(surf_action_t action,
                                    const char *category)
{
  XBT_IN("(%p,%s)", action, category);
  action->category = xbt_strdup(category);
  XBT_OUT();
}
#endif

double surf_action_get_remains(surf_action_t action)
{
  XBT_IN("(%p)", action);
  surf_model_t model = action->model_type;
  /* update remains before return it */
  if (model->model_private->update_mechanism == UM_LAZY)      /* update remains before return it */
    generic_update_action_remaining_lazy(action, surf_get_clock());
  XBT_OUT();
  return action->remains;
}
