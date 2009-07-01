/* 	$Id$	 */

/* Copyright (c) 2007 Arnaud Legrand, Bruno Donnassolo.
   All rights reserved.                                          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/log.h"
#include "xbt/ex.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_action, simix,
                                "Logging specific to SIMIX (action)");

/************************************* Actions *********************************/
/** \brief Creates a new SIMIX action to communicate two hosts.
 *
 * 	This function creates a SURF action and allocates the data necessary to create the SIMIX action. It can raise a network_error exception if the host is unavailable.
 *	\param sender SIMIX host sender
 * 	\param receiver SIMIX host receiver
 * 	\param name Action name
 * 	\param size Communication size (in bytes)
 * 	\param rate Communication rate between hosts.
 * 	\return A new SIMIX action
 * */
smx_action_t SIMIX_action_communicate(smx_host_t sender,
                                      smx_host_t receiver, const char *name,
                                      double size, double rate)
{
  smx_action_t act;
  smx_simdata_action_t simdata;

  /* check if the host is active */
  if (surf_workstation_model->extension.
      workstation.get_state(sender->simdata->host) != SURF_CPU_ON) {
    THROW1(network_error, 0, "Host %s failed, you cannot call this function",
           sender->name);
  }
  if (surf_workstation_model->extension.
      workstation.get_state(receiver->simdata->host) != SURF_CPU_ON) {
    THROW1(network_error, 0, "Host %s failed, you cannot call this function",
           receiver->name);
  }

  /* alloc structures */
  act = xbt_new0(s_smx_action_t, 1);
  act->simdata = xbt_new0(s_smx_simdata_action_t, 1);
  simdata = act->simdata;
  act->cond_list = xbt_fifo_new();

  /* initialize them */
  act->name = xbt_strdup(name);
  simdata->source = sender;


  simdata->surf_action =
    surf_workstation_model->extension.workstation.
    communicate(sender->simdata->host, receiver->simdata->host, size, rate);
  surf_workstation_model->action_data_set(simdata->surf_action, act);

  DEBUG1("Create communicate action %p", act);
  return act;
}

/** \brief Creates a new SIMIX action to execute an action.
 *
 * 	This function creates a SURF action and allocates the data necessary to create the SIMIX action. It can raise a host_error exception if the host crashed.
 *	\param host SIMIX host where the action will be executed
 * 	\param name Action name
 * 	\param amount Task amount (in bytes)
 * 	\return A new SIMIX action
 * */
smx_action_t SIMIX_action_execute(smx_host_t host, const char *name,
                                  double amount)
{
  smx_action_t act;
  smx_simdata_action_t simdata;

  /* check if the host is active */
  if (surf_workstation_model->extension.
      workstation.get_state(host->simdata->host) != SURF_CPU_ON) {
    THROW1(host_error, 0, "Host %s failed, you cannot call this function",
           host->name);
  }

  /* alloc structures */
  act = xbt_new0(s_smx_action_t, 1);
  act->simdata = xbt_new0(s_smx_simdata_action_t, 1);
  simdata = act->simdata;
  act->cond_list = xbt_fifo_new();

  /* initialize them */
  simdata->source = host;
  act->name = xbt_strdup(name);

  /* set communication */
  simdata->surf_action =
    surf_workstation_model->extension.workstation.execute(host->simdata->host,
                                                          amount);

  surf_workstation_model->action_data_set(simdata->surf_action, act);

  DEBUG1("Create execute action %p", act);
  return act;
}

/** \brief Creates a new sleep SIMIX action.
 *
 * 	This function creates a SURF action and allocates the data necessary to create the SIMIX action. It can raise a host_error exception if the host crashed. The default SIMIX name of the action is "sleep".
 *	\param host SIMIX host where the sleep will run.
 * 	\param duration Time duration of the sleep.
 * 	\return A new SIMIX action
 * */
smx_action_t SIMIX_action_sleep(smx_host_t host, double duration)
{
  char name[] = "sleep";
  smx_simdata_action_t simdata;
  smx_action_t act;

  /* check if the host is active */
  if (surf_workstation_model->extension.
      workstation.get_state(host->simdata->host) != SURF_CPU_ON) {
    THROW1(host_error, 0, "Host %s failed, you cannot call this function",
           host->name);
  }

  /* alloc structures */
  act = xbt_new0(s_smx_action_t, 1);
  act->simdata = xbt_new0(s_smx_simdata_action_t, 1);
  simdata = act->simdata;
  act->cond_list = xbt_fifo_new();

  /* initialize them */
  simdata->source = host;
  act->name = xbt_strdup(name);

  simdata->surf_action =
    surf_workstation_model->extension.workstation.sleep(host->simdata->host,
                                                        duration);

  surf_workstation_model->action_data_set(simdata->surf_action, act);

  DEBUG1("Create sleep action %p", act);
  return act;
}

/**
 * 	\brief Cancels an action.
 *
 *	This functions stops the execution of an action. It calls a surf functions.
 *	\param action The SIMIX action
 */
void SIMIX_action_cancel(smx_action_t action)
{
  xbt_assert0((action != NULL), "Invalid parameter");

  DEBUG1("Cancel action %p", action);
  if (action->simdata->surf_action) {
    surf_workstation_model->action_cancel(action->simdata->surf_action);
  }
  return;
}

/**
 * 	\brief Changes the action's priority
 *
 *	This functions changes the priority only. It calls a surf functions.
 *	\param action The SIMIX action
 *	\param priority The new priority
 */
void SIMIX_action_set_priority(smx_action_t action, double priority)
{
  xbt_assert0((action != NULL)
              && (action->simdata != NULL), "Invalid parameter");

  surf_workstation_model->set_priority(action->simdata->surf_action,
                                       priority);
  return;
}

/**
 * 	\brief Destroys an action
 *
 *	Destroys an action, freing its memory. This function cannot be called if there are a conditional waiting for it.
 *	\param action The SIMIX action
 */
int SIMIX_action_destroy(smx_action_t action)
{
  XBT_IN3("(%p:'%s',%d)", action, action->name, action->refcount);
  xbt_assert0((action != NULL), "Invalid parameter");

  action->refcount--;
  if (action->refcount > 0)
    return 0;

  xbt_assert1((xbt_fifo_size(action->cond_list) == 0),
              "Conditional list not empty %d. There is a problem. Cannot destroy it now!",
              xbt_fifo_size(action->cond_list));

  DEBUG1("Destroy action %p", action);
  if (action->name)
    xbt_free(action->name);

  xbt_fifo_free(action->cond_list);

  if (action->simdata->surf_action)
    action->simdata->surf_action->model_type->action_unref(action->
                                                          simdata->surf_action);

  xbt_free(action->simdata);
  xbt_free(action);
  return 1;
}

/**
 * 	\brief Increase refcount of anan action
 *
 *	\param action The SIMIX action
 */
void SIMIX_action_use(smx_action_t action)
{
  XBT_IN3("(%p:'%s',%d)", action, action->name, action->refcount);
  xbt_assert0((action != NULL), "Invalid parameter");

  action->refcount++;

  return;
}

/**
 * 	\brief Decrease refcount of anan action
 *
 *	\param action The SIMIX action
 */
void SIMIX_action_release(smx_action_t action)
{
  xbt_assert0((action != NULL), "Invalid parameter");

  action->refcount--;

  return;
}

/**
 * 	\brief Set an action to a condition
 *
 * 	Creates the "link" between an action and a condition. You have to call this function when you create an action and want to wait its ending.
 *	\param action SIMIX action
 *	\param cond SIMIX cond
 */
void SIMIX_register_action_to_condition(smx_action_t action, smx_cond_t cond)
{
  xbt_assert0((action != NULL) && (cond != NULL), "Invalid parameters");

  DEBUG2("Register action %p to cond %p", action, cond);
  __SIMIX_cond_display_actions(cond);
  xbt_fifo_push(cond->actions, action);
  __SIMIX_cond_display_actions(cond);
  DEBUG2("Register condition %p to action %p", cond, action);
  __SIMIX_action_display_conditions(action);
  xbt_fifo_push(action->cond_list, cond);
  __SIMIX_action_display_conditions(action);
}

/**
 * 	\brief Unset an action to a condition.
 *
 * 	Destroys the "links" from the condition to this action.
 *	\param action SIMIX action
 *	\param cond SIMIX cond
 */
void SIMIX_unregister_action_to_condition(smx_action_t action,
                                          smx_cond_t cond)
{
  xbt_assert0((action != NULL) && (cond != NULL), "Invalid parameters");

  __SIMIX_cond_display_actions(cond);
  xbt_fifo_remove_all(cond->actions, action);
  __SIMIX_cond_display_actions(cond);
  __SIMIX_action_display_conditions(action);
  xbt_fifo_remove_all(action->cond_list, cond);
  __SIMIX_action_display_conditions(action);
}

/**
 * 	\brief Return how much remais to be done in the action.
 *
 * 	\param action The SIMIX action
 *	\return Remains cost
 */
double SIMIX_action_get_remains(smx_action_t action)
{
  xbt_assert0((action != NULL), "Invalid parameter");
  return action->simdata->surf_action->remains;
}

smx_action_t SIMIX_action_parallel_execute(char *name, int host_nb,
                                           smx_host_t * host_list,
                                           double *computation_amount,
                                           double *communication_amount,
                                           double amount, double rate)
{
  void **workstation_list = NULL;
  smx_simdata_action_t simdata;
  smx_action_t act;
  int i;

  /* alloc structures */
  act = xbt_new0(s_smx_action_t, 1);
  act->simdata = xbt_new0(s_smx_simdata_action_t, 1);
  simdata = act->simdata;
  act->cond_list = xbt_fifo_new();

  /* initialize them */
  act->name = xbt_strdup(name);

  /* set action */

  workstation_list = xbt_new0(void *, host_nb);
  for (i = 0; i < host_nb; i++)
    workstation_list[i] = host_list[i]->simdata->host;

  simdata->surf_action =
    surf_workstation_model->extension.
    workstation.execute_parallel_task(host_nb, workstation_list,
                                      computation_amount,
                                      communication_amount, amount, rate);

  surf_workstation_model->action_data_set(simdata->surf_action, act);

  return act;
}

e_surf_action_state_t SIMIX_action_get_state(smx_action_t action)
{
  xbt_assert0((action != NULL), "Invalid parameter");
  return surf_workstation_model->action_state_get(action->simdata->
                                                  surf_action);

}

void __SIMIX_cond_display_actions(smx_cond_t cond)
{
  xbt_fifo_item_t item = NULL;
  smx_action_t action = NULL;

  DEBUG1("Actions for condition %p", cond);
  xbt_fifo_foreach(cond->actions, item, action, smx_action_t)
    DEBUG2("\t %p [%s]", action, action->name);
}

void __SIMIX_action_display_conditions(smx_action_t action)
{
  xbt_fifo_item_t item = NULL;
  smx_cond_t cond = NULL;

  DEBUG1("Conditions for action %p", action);
  xbt_fifo_foreach(action->cond_list, item, cond, smx_cond_t)
    DEBUG1("\t %p", cond);
}
