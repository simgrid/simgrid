/* 	$Id$	 */

/* Copyright (c) 2007 Arnaud Legrand, Bruno Donnassolo.
   All rights reserved.                                          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/log.h"


XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_synchro, simix,
                                "Logging specific to SIMIX (synchronization)");


/****************************** Synchronization *******************************/

/*********************************** Mutex ************************************/

/**
 * \brief Initialize a mutex.
 *
 * Allocs and creates the data for the mutex. It have to be called before the utilisation of the mutex.
 * \return A mutex
 */
smx_mutex_t SIMIX_mutex_init()
{
  smx_mutex_t m = xbt_new0(s_smx_mutex_t, 1);
  s_smx_process_t p;            /* useful to initialize sleeping swag */
  /* structures initialization */
  m->refcount = 0;
  m->sleeping = xbt_swag_new(xbt_swag_offset(p, synchro_hookup));
  return m;
}

/**
 * \brief Locks a mutex.
 *
 * Tries to lock a mutex, if the mutex isn't used yet, the process can continue its execution, else it'll be blocked here. You have to call #SIMIX_mutex_unlock to free the mutex.
 * \param mutex The mutex
 */
void SIMIX_mutex_lock(smx_mutex_t mutex)
{
  smx_process_t self = SIMIX_process_self();
  xbt_assert0((mutex != NULL), "Invalid parameters");


  if (mutex->refcount) {
    /* somebody using the mutex, block */
    xbt_swag_insert(self, mutex->sleeping);
    self->mutex = mutex;
    /* wait for some process make the unlock and wake up me from mutex->sleeping */
    SIMIX_process_yield();
    self->mutex = NULL;

    /* verify if the process was suspended */
    while (self->suspended) {
      SIMIX_process_yield();
    }

    mutex->refcount = 1;
  } else {
    /* mutex free */
    mutex->refcount = 1;
  }
  return;
}

/**
 * \brief Tries to lock a mutex.
 *
 * Tries to lock a mutex, return 1 if the mutex is free, 0 else. This function does not block the process if the mutex is used.
 * \param mutex The mutex
 * \return 1 - mutex free, 0 - mutex used
 */
int SIMIX_mutex_trylock(smx_mutex_t mutex)
{
  xbt_assert0((mutex != NULL), "Invalid parameters");

  if (mutex->refcount)
    return 0;
  else {
    mutex->refcount = 1;
    return 1;
  }
}

/**
 * \brief Unlocks a mutex.
 *
 * Unlocks the mutex and wakes up a process blocked on it. If there are no process sleeping, only sets the mutex as free.
 * \param mutex The mutex
 */
void SIMIX_mutex_unlock(smx_mutex_t mutex)
{
  smx_process_t p;              /*process to wake up */

  xbt_assert0((mutex != NULL), "Invalid parameters");

  if (xbt_swag_size(mutex->sleeping) > 0) {
    p = xbt_swag_extract(mutex->sleeping);
    mutex->refcount = 0;
    xbt_swag_insert(p, simix_global->process_to_run);
  } else {
    /* nobody to wake up */
    mutex->refcount = 0;
  }
  return;
}

/**
 * \brief Destroys a mutex.
 *
 * Destroys and frees the mutex's memory. 
 * \param mutex A mutex
 */
void SIMIX_mutex_destroy(smx_mutex_t mutex)
{
  if (mutex == NULL)
    return;
  else {
    xbt_swag_free(mutex->sleeping);
    xbt_free(mutex);
    return;
  }
}

/******************************** Conditional *********************************/

/**
 * \brief Initialize a condition.
 *
 * Allocs and creates the data for the condition. It have to be called before the utilisation of the condition.
 * \return A condition
 */
smx_cond_t SIMIX_cond_init()
{
  smx_cond_t cond = xbt_new0(s_smx_cond_t, 1);
  s_smx_process_t p;

  cond->sleeping = xbt_swag_new(xbt_swag_offset(p, synchro_hookup));
  cond->actions = xbt_fifo_new();
  cond->mutex = NULL;
  return cond;
}

/**
 * \brief Signalizes a condition.
 *
 * Signalizes a condition and wakes up a sleeping process. If there are no process sleeping, no action is done.
 * \param cond A condition
 */
void SIMIX_cond_signal(smx_cond_t cond)
{
  smx_process_t proc = NULL;
  DEBUG1("Signal condition %p", cond);
  xbt_assert0((cond != NULL), "Invalid parameters");


  if (xbt_swag_size(cond->sleeping) >= 1) {
    proc = xbt_swag_extract(cond->sleeping);
    xbt_swag_insert(proc, simix_global->process_to_run);
  }

  return;
}

/**
 * \brief Waits on a condition.
 *
 * Blocks a process until the signal is called. This functions frees the mutex associated and locks it after its execution.
 * \param cond A condition
 * \param mutex A mutex
 */
void SIMIX_cond_wait(smx_cond_t cond, smx_mutex_t mutex)
{
  smx_action_t act_sleep;
  xbt_assert0((mutex != NULL), "Invalid parameters");

  DEBUG1("Wait condition %p", cond);
  cond->mutex = mutex;

  SIMIX_mutex_unlock(mutex);
  /* always create an action null in case there is a host failure */
/*   if (xbt_fifo_size(cond->actions) == 0) { */
  act_sleep = SIMIX_action_sleep(SIMIX_host_self(), -1);
  SIMIX_register_action_to_condition(act_sleep, cond);
  __SIMIX_cond_wait(cond);
  SIMIX_unregister_action_to_condition(act_sleep, cond);
  SIMIX_action_destroy(act_sleep);
/*   } else { */
/*     __SIMIX_cond_wait(cond); */
/*   } */
  /* get the mutex again */
  SIMIX_mutex_lock(cond->mutex);

  return;
}

xbt_fifo_t SIMIX_cond_get_actions(smx_cond_t cond)
{
  xbt_assert0((cond != NULL), "Invalid parameters");
  return cond->actions;
}

void __SIMIX_cond_wait(smx_cond_t cond)
{
  smx_process_t self = SIMIX_process_self();
  xbt_assert0((cond != NULL), "Invalid parameters");

  /* process status */

  self->cond = cond;
  xbt_swag_insert(self, cond->sleeping);
  SIMIX_process_yield();
  self->cond = NULL;
  while (self->suspended) {
    SIMIX_process_yield();
  }
  return;

}

/**
 * \brief Waits on a condition with timeout.
 *
 * Same behavior of #SIMIX_cond_wait, but waits a maximum time and throws an timeout_error if it happens.
 * \param cond A condition
 * \param mutex A mutex
 * \param max_duration Timeout time
 */
void SIMIX_cond_wait_timeout(smx_cond_t cond, smx_mutex_t mutex,
                             double max_duration)
{

  smx_action_t act_sleep;
  xbt_assert0((mutex != NULL), "Invalid parameters");

  DEBUG1("Timed wait condition %p", cond);
  cond->mutex = mutex;

  SIMIX_mutex_unlock(mutex);
  if (max_duration >= 0) {
    act_sleep = SIMIX_action_sleep(SIMIX_host_self(), max_duration);
    SIMIX_register_action_to_condition(act_sleep, cond);
    __SIMIX_cond_wait(cond);
    SIMIX_unregister_action_to_condition(act_sleep, cond);
    if (SIMIX_action_get_state(act_sleep) == SURF_ACTION_DONE) {
      SIMIX_action_destroy(act_sleep);
      THROW0(timeout_error, 0, "Condition timeout");
    } else {
      SIMIX_action_destroy(act_sleep);
    }

  } else
    __SIMIX_cond_wait(cond);

  /* get the mutex again */
  SIMIX_mutex_lock(cond->mutex);

  return;
}

/**
 * \brief Broadcasts a condition.
 *
 * Signalizes a condition and wakes up ALL sleping process. If there are no process sleeping, no action is done.
 * \param cond A condition
 */
void SIMIX_cond_broadcast(smx_cond_t cond)
{
  smx_process_t proc = NULL;
  smx_process_t proc_next = NULL;

  xbt_assert0((cond != NULL), "Invalid parameters");

  DEBUG1("Broadcast condition %p", cond);
  xbt_swag_foreach_safe(proc, proc_next, cond->sleeping) {
    xbt_swag_remove(proc, cond->sleeping);
    xbt_swag_insert(proc, simix_global->process_to_run);
  }

  return;
}

/**
 * \brief Destroys a contidion.
 *
 * Destroys and frees the condition's memory. 
 * \param cond A condition
 */
void SIMIX_cond_destroy(smx_cond_t cond)
{
  DEBUG1("Destroy condition %p", cond);
  if (cond == NULL)
    return;
  else {
    xbt_fifo_item_t item = NULL;
    smx_action_t action = NULL;

    xbt_assert0(xbt_swag_size(cond->sleeping) == 0,
                "Cannot destroy conditional since someone is still using it");
    xbt_swag_free(cond->sleeping);

    DEBUG1("%d actions registered", xbt_fifo_size(cond->actions));
    __SIMIX_cond_display_actions(cond);
    xbt_fifo_foreach(cond->actions, item, action, smx_action_t) {
      SIMIX_unregister_action_to_condition(action, cond);
    }
    __SIMIX_cond_display_actions(cond);

    xbt_fifo_free(cond->actions);
    xbt_free(cond);
    return;
  }
}

void SIMIX_cond_display_info(smx_cond_t cond)
{
  if (cond == NULL)
    return;
  else {
    smx_process_t process = NULL;

    INFO0("Blocked process on this condition:");
    xbt_swag_foreach(process, cond->sleeping) {
      INFO2("\t %s running on host %s", process->name,
            process->smx_host->name);
    }
  }
}
