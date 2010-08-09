/* Copyright (c) 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

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

  /* check if the host is active */
  if (surf_workstation_model->extension.workstation.get_state(sender->host) !=
      SURF_RESOURCE_ON) {
    THROW1(network_error, 0, "Host %s failed, you cannot call this function",
           sender->name);
  }
  if (surf_workstation_model->extension.workstation.
      get_state(receiver->host) != SURF_RESOURCE_ON) {
    THROW1(network_error, 0, "Host %s failed, you cannot call this function",
           receiver->name);
  }

  /* alloc structures */
  act = xbt_new0(s_smx_action_t, 1);
  act->cond_list = xbt_fifo_new();
  act->sem_list = xbt_fifo_new();

  /* initialize them */
  act->name = xbt_strdup(name);
  act->source = sender;
#ifdef HAVE_TRACING
  act->category = NULL;
#endif

  act->surf_action =
    surf_workstation_model->extension.workstation.communicate(sender->host,
                                                              receiver->host,
                                                              size, rate);
  surf_workstation_model->action_data_set(act->surf_action, act);

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

  /* check if the host is active */
  if (surf_workstation_model->extension.workstation.get_state(host->host) !=
      SURF_RESOURCE_ON) {
    THROW1(host_error, 0, "Host %s failed, you cannot call this function",
           host->name);
  }

  /* alloc structures */
  act = xbt_new0(s_smx_action_t, 1);
  act->cond_list = xbt_fifo_new();
  act->sem_list = xbt_fifo_new();

  /* initialize them */
  act->source = host;
  act->name = xbt_strdup(name);
#ifdef HAVE_TRACING
  act->category = NULL;
#endif

  /* set communication */
  act->surf_action =
    surf_workstation_model->extension.workstation.execute(host->host, amount);

  surf_workstation_model->action_data_set(act->surf_action, act);

  DEBUG1("Create execute action %p", act);
#ifdef HAVE_TRACING
  TRACE_smx_action_execute (act);
#endif
  return act;
}

/** \brief Creates a new sleep SIMIX action.
 *
 * This function creates a SURF action and allocates the data necessary
 * to create the SIMIX action. It can raise a host_error exception if the
 * host crashed. The default SIMIX name of the action is "sleep".
 *
 *	\param host SIMIX host where the sleep will run.
 * 	\param duration Time duration of the sleep.
 * 	\return A new SIMIX action
 * */
smx_action_t SIMIX_action_sleep(smx_host_t host, double duration)
{
  char name[] = "sleep";
  smx_action_t act;

  /* check if the host is active */
  if (surf_workstation_model->extension.workstation.get_state(host->host) !=
      SURF_RESOURCE_ON) {
    THROW1(host_error, 0, "Host %s failed, you cannot call this function",
           host->name);
  }

  /* alloc structures */
  act = xbt_new0(s_smx_action_t, 1);
  act->cond_list = xbt_fifo_new();
  act->sem_list = xbt_fifo_new();

  /* initialize them */
  act->source = host;
  act->name = xbt_strdup(name);
#ifdef HAVE_TRACING
  act->category = NULL;
#endif

  act->surf_action =
    surf_workstation_model->extension.workstation.sleep(host->host, duration);

  surf_workstation_model->action_data_set(act->surf_action, act);

  DEBUG1("Create sleep action %p", act);
  return act;
}

/**
 * 	\brief Cancels an action.
 *
 *	This functions stops the execution of an action. It calls a surf functions.
 *	\param action The SIMIX action
 */
XBT_INLINE void SIMIX_action_cancel(smx_action_t action)
{
  xbt_assert0((action != NULL), "Invalid parameter");

  DEBUG1("Cancel action %p", action);
  if (action->surf_action) {
    surf_workstation_model->action_cancel(action->surf_action);
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
XBT_INLINE void SIMIX_action_set_priority(smx_action_t action, double priority)
{
  xbt_assert0((action != NULL), "Invalid parameter");

  surf_workstation_model->set_priority(action->surf_action, priority);
  return;
}

/**
 * 	\brief Resumes the execution of an action.
 *
 *  This functions restarts the execution of an action. It just calls the right SURF function.
 *  \param action The SIMIX action
 *  \param priority The new priority
 */
XBT_INLINE void SIMIX_action_resume(smx_action_t action)
{
  xbt_assert0((action != NULL), "Invalid parameter");

  surf_workstation_model->resume(action->surf_action);
  return;
}

/**
 *  \brief Suspends the execution of an action.
 *
 *  This functions suspends the execution of an action. It just calls the right SURF function.
 *  \param action The SIMIX action
 *  \param priority The new priority
 */
XBT_INLINE void SIMIX_action_suspend(smx_action_t action)
{
  xbt_assert0((action != NULL), "Invalid parameter");

  surf_workstation_model->suspend(action->surf_action);
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

  xbt_assert1((xbt_fifo_size(action->sem_list) == 0),
              "Semaphore list not empty %d. There is a problem. Cannot destroy it now!",
              xbt_fifo_size(action->sem_list));

  DEBUG1("Destroy action %p", action);
  if (action->name)
    xbt_free(action->name);

  xbt_fifo_free(action->cond_list);
  xbt_fifo_free(action->sem_list);

  if (action->surf_action)
    action->surf_action->model_type->action_unref(action->surf_action);
#ifdef HAVE_TRACING
  TRACE_smx_action_destroy (action);
#endif
  xbt_free(action);
  return 1;
}

/**
 * 	\brief Increase refcount of anan action
 *
 *	\param action The SIMIX action
 */
XBT_INLINE void SIMIX_action_use(smx_action_t action)
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
XBT_INLINE void SIMIX_action_release(smx_action_t action)
{
  xbt_assert0((action != NULL), "Invalid parameter");

  action->refcount--;

  return;
}

/**
 *  \brief Set an action to a condition
 *
 *  Creates the "link" between an action and a condition. You have to call this function when you create an action and want to wait its ending.
 *  \param action SIMIX action
 *  \param cond SIMIX cond
 */
void SIMIX_register_action_to_condition(smx_action_t action, smx_cond_t cond)
{
  xbt_assert0((action != NULL) && (cond != NULL), "Invalid parameters");

  DEBUG2("Register action %p to cond %p", action, cond);
  if(XBT_LOG_ISENABLED(simix_action, xbt_log_priority_debug))
    __SIMIX_cond_display_actions(cond);

  xbt_fifo_push(cond->actions, action);

  if(XBT_LOG_ISENABLED(simix_action, xbt_log_priority_debug))
    __SIMIX_cond_display_actions(cond);

  DEBUG2("Register condition %p to action %p", cond, action);

  if(XBT_LOG_ISENABLED(simix_action, xbt_log_priority_debug))
    __SIMIX_action_display_conditions(action);

  xbt_fifo_push(action->cond_list, cond);

  if(XBT_LOG_ISENABLED(simix_action, xbt_log_priority_debug))
    __SIMIX_action_display_conditions(action);
}

/**
 *  \brief Unset an action to a condition.
 *
 *  Destroys the "links" from the condition to this action.
 *  \param action SIMIX action
 *  \param cond SIMIX cond
 */
void SIMIX_unregister_action_to_condition(smx_action_t action,
                                          smx_cond_t cond)
{
  xbt_assert0((action != NULL) && (cond != NULL), "Invalid parameters");

  if(XBT_LOG_ISENABLED(simix_action, xbt_log_priority_debug))
    __SIMIX_cond_display_actions(cond);

  xbt_fifo_remove_all(cond->actions, action);

  if(XBT_LOG_ISENABLED(simix_action, xbt_log_priority_debug))
    __SIMIX_cond_display_actions(cond);

  if(XBT_LOG_ISENABLED(simix_action, xbt_log_priority_debug))
    __SIMIX_action_display_conditions(action);

  xbt_fifo_remove_all(action->cond_list, cond);

  if(XBT_LOG_ISENABLED(simix_action, xbt_log_priority_debug))
    __SIMIX_action_display_conditions(action);
}
/**
 *  \brief Link an action to a semaphore
 *
 *  When the action terminates, the semaphore gets signaled automatically.
 */
XBT_INLINE void SIMIX_register_action_to_semaphore(smx_action_t action, smx_sem_t sem) {

  DEBUG2("Register action %p to semaphore %p (and otherwise)", action, sem);
  xbt_fifo_push(sem->actions, action);
  xbt_fifo_push(action->sem_list, sem);
}
/**
 *  \brief Unset an action to a semaphore.
 *
 *  Destroys the "links" from the semaphore to this action.
 */
XBT_INLINE void SIMIX_unregister_action_to_semaphore(smx_action_t action,
                                          smx_sem_t sem)
{
  xbt_fifo_remove_all(sem->actions, action);
  xbt_fifo_remove_all(action->sem_list, sem);
}

/**
 * 	\brief Return how much remais to be done in the action.
 *
 * 	\param action The SIMIX action
 *	\return Remains cost
 */
XBT_INLINE double SIMIX_action_get_remains(smx_action_t action)
{
  xbt_assert0((action != NULL), "Invalid parameter");
  return surf_workstation_model->get_remains(action->surf_action);
}

XBT_INLINE int SIMIX_action_is_latency_bounded(smx_action_t action)
{
  xbt_assert0((action != NULL), "Invalid parameter");
  return surf_workstation_model->get_latency_limited(action->surf_action);
}

smx_action_t SIMIX_action_parallel_execute(char *name, int host_nb,
                                           smx_host_t * host_list,
                                           double *computation_amount,
                                           double *communication_amount,
                                           double amount, double rate)
{
  void **workstation_list = NULL;
  smx_action_t act;
  int i;

  /* alloc structures */
  act = xbt_new0(s_smx_action_t, 1);
  act->cond_list = xbt_fifo_new();
  act->sem_list = xbt_fifo_new();

  /* initialize them */
  act->name = xbt_strdup(name);
#ifdef HAVE_TRACING
  act->category = NULL;
#endif

  /* set action */

  workstation_list = xbt_new0(void *, host_nb);
  for (i = 0; i < host_nb; i++)
    workstation_list[i] = host_list[i]->host;

  act->surf_action =
    surf_workstation_model->extension.workstation.
    execute_parallel_task(host_nb, workstation_list, computation_amount,
                          communication_amount, amount, rate);

  surf_workstation_model->action_data_set(act->surf_action, act);

  return act;
}

XBT_INLINE e_surf_action_state_t SIMIX_action_get_state(smx_action_t action)
{
  xbt_assert0((action != NULL), "Invalid parameter");
  return surf_workstation_model->action_state_get(action->surf_action);
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

XBT_INLINE char *SIMIX_action_get_name(smx_action_t action)
{
  xbt_assert0((action != NULL), "Invalid parameter");
  return action->name;
}
/** @brief Change the name of the action. Warning, the string you provide is not strdup()ed */
XBT_INLINE void SIMIX_action_set_name(smx_action_t action,char *name)
{
  xbt_free(action->name);
  action->name = name;
}

/** @brief broadcast any condition and release any semaphore including this action */
void SIMIX_action_signal_all(smx_action_t action){
  smx_cond_t cond;
  smx_sem_t sem;

  while ((cond = xbt_fifo_pop(action->cond_list)))
    SIMIX_cond_broadcast(cond);

  while ((sem = xbt_fifo_pop(action->sem_list)))
    SIMIX_sem_release(sem);
}
