/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smx_private.h"
#include "xbt/log.h"


XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_synchro, simix,
                                "Logging specific to SIMIX (synchronization)");

static smx_action_t SIMIX_synchro_wait(smx_host_t smx_host, double timeout);
static void SIMIX_synchro_finish(smx_action_t action);
static void _SIMIX_cond_wait(smx_cond_t cond, smx_mutex_t mutex, double timeout,
                             smx_process_t issuer, smx_simcall_t simcall);
static void _SIMIX_sem_wait(smx_sem_t sem, double timeout, smx_process_t issuer,
                            smx_simcall_t simcall);

/***************************** Synchro action *********************************/

static smx_action_t SIMIX_synchro_wait(smx_host_t smx_host, double timeout)
{
  XBT_IN("(%p, %f)",smx_host,timeout);
  surf_model_t ws_model = surf_resource_model(smx_host, SURF_WKS_LEVEL);

  smx_action_t action;
  action = xbt_mallocator_get(simix_global->action_mallocator);
  action->type = SIMIX_ACTION_SYNCHRO;
  action->name = xbt_strdup("synchro");
  action->synchro.sleep = 
    ws_model->extension.workstation.sleep(smx_host, timeout);

  ws_model->action_data_set(action->synchro.sleep, action);
  XBT_OUT();
  return action;
}

void SIMIX_synchro_stop_waiting(smx_process_t process, smx_simcall_t simcall)
{
  XBT_IN("(%p, %p)",process,simcall);
  switch (simcall->call) {

    case SIMCALL_MUTEX_LOCK:
      xbt_swag_remove(process, simcall_mutex_lock__get__mutex(simcall)->sleeping);
      break;

    case SIMCALL_COND_WAIT:
      xbt_swag_remove(process, simcall_cond_wait__get__cond(simcall)->sleeping);
      break;

    case SIMCALL_COND_WAIT_TIMEOUT:
      xbt_swag_remove(process, simcall_cond_wait_timeout__get__cond(simcall)->sleeping);
      break;

    case SIMCALL_SEM_ACQUIRE:
      xbt_swag_remove(process, simcall_sem_acquire__get__sem(simcall)->sleeping);
      break;

    case SIMCALL_SEM_ACQUIRE_TIMEOUT:
      xbt_swag_remove(process, simcall_sem_acquire_timeout__get__sem(simcall)->sleeping);
      break;

    default:
      THROW_IMPOSSIBLE;
  }
  XBT_OUT();
}

void SIMIX_synchro_destroy(smx_action_t action)
{
  XBT_IN("(%p)",action);
  XBT_DEBUG("Destroying synchro %p", action);
  xbt_assert(action->type == SIMIX_ACTION_SYNCHRO);

  action->synchro.sleep->model_obj->action_unref(action->synchro.sleep);
  xbt_free(action->name);
  xbt_mallocator_release(simix_global->action_mallocator, action);
  XBT_OUT();
}

void SIMIX_post_synchro(smx_action_t action)
{
  XBT_IN("(%p)",action);
  xbt_assert(action->type == SIMIX_ACTION_SYNCHRO);
  surf_model_t ws_model = action->synchro.sleep->model_obj;

  if (ws_model->action_state_get(action->synchro.sleep) == SURF_ACTION_FAILED)
    action->state = SIMIX_FAILED;
  else if(ws_model->action_state_get(action->synchro.sleep) == SURF_ACTION_DONE)
    action->state = SIMIX_SRC_TIMEOUT;

  SIMIX_synchro_finish(action);  
  XBT_OUT();
}

static void SIMIX_synchro_finish(smx_action_t action)
{
  XBT_IN("(%p)",action);
  smx_simcall_t simcall = xbt_fifo_shift(action->simcalls);

  switch (action->state) {

    case SIMIX_SRC_TIMEOUT:
      SMX_EXCEPTION(simcall->issuer, timeout_error, 0, "Synchro's wait timeout");
      break;

    case SIMIX_FAILED:
        simcall->issuer->context->iwannadie = 1;
//      SMX_EXCEPTION(simcall->issuer, host_error, 0, "Host failed");
      break;

    default:
      THROW_IMPOSSIBLE;
      break;
  }

  SIMIX_synchro_stop_waiting(simcall->issuer, simcall);
  simcall->issuer->waiting_action = NULL;
  SIMIX_synchro_destroy(action);
  SIMIX_simcall_answer(simcall);
  XBT_OUT();
}
/*********************************** Mutex ************************************/

smx_mutex_t SIMIX_pre_mutex_init(smx_simcall_t simcall){
  return SIMIX_mutex_init();
}
/**
 * \brief Initialize a mutex.
 *
 * Allocs and creates the data for the mutex.
 * \return A mutex
 */
smx_mutex_t SIMIX_mutex_init(void)
{
  XBT_IN("()");
  s_smx_process_t p;            /* useful to initialize sleeping swag */

  smx_mutex_t mutex = xbt_new0(s_smx_mutex_t, 1);
  mutex->locked = 0;
  mutex->sleeping = xbt_swag_new(xbt_swag_offset(p, synchro_hookup));
  XBT_OUT();
  return mutex;
}

/**
 * \brief Handles a mutex lock simcall.
 * \param simcall the simcall
 */
void SIMIX_pre_mutex_lock(smx_simcall_t simcall, smx_mutex_t mutex)
{
  XBT_IN("(%p)",simcall);
  /* FIXME: check where to validate the arguments */
  smx_action_t sync_act = NULL;
  smx_process_t process = simcall->issuer;

  if (mutex->locked) {
    /* FIXME: check if the host is active ? */
    /* Somebody using the mutex, use a synchro action to get host failures */
    sync_act = SIMIX_synchro_wait(process->smx_host, -1);
    xbt_fifo_push(sync_act->simcalls, simcall);
    simcall->issuer->waiting_action = sync_act;
    xbt_swag_insert(simcall->issuer, mutex->sleeping);   
  } else {
    /* mutex free */
    mutex->locked = 1;
    mutex->owner = simcall->issuer;
    SIMIX_simcall_answer(simcall);
  }
  XBT_OUT();
}

int SIMIX_pre_mutex_trylock(smx_simcall_t simcall, smx_mutex_t mutex){
  return SIMIX_mutex_trylock(mutex, simcall->issuer);
}	
/**
 * \brief Tries to lock a mutex.
 *
 * Tries to lock a mutex, return 1 if the mutex is unlocked, else 0.
 * This function does not block and wait for the mutex to be unlocked.
 * \param mutex The mutex
 * \param issuer The process that tries to acquire the mutex
 * \return 1 - mutex free, 0 - mutex used
 */
int SIMIX_mutex_trylock(smx_mutex_t mutex, smx_process_t issuer)
{
  XBT_IN("(%p, %p)",mutex,issuer);
  if (mutex->locked){
    XBT_OUT();
    return 0;
  }

  mutex->locked = 1;
  mutex->owner = issuer;
  XBT_OUT();
  return 1;
}

void SIMIX_pre_mutex_unlock(smx_simcall_t simcall, smx_mutex_t mutex){
   SIMIX_mutex_unlock(mutex, simcall->issuer);
}
/**
 * \brief Unlocks a mutex.
 *
 * Unlocks the mutex and gives it to a process waiting for it. 
 * If the unlocker is not the owner of the mutex nothing happens.
 * If there are no process waiting, it sets the mutex as free.
 * \param mutex The mutex
 * \param issuer The process trying to unlock the mutex
 */
void SIMIX_mutex_unlock(smx_mutex_t mutex, smx_process_t issuer)
{
  XBT_IN("(%p, %p)",mutex,issuer);
  smx_process_t p;              /*process to wake up */

  /* If the mutex is not owned by the issuer do nothing */
  if (issuer != mutex->owner){
    XBT_OUT();
    return;
  }

  if (xbt_swag_size(mutex->sleeping) > 0) {
    p = xbt_swag_extract(mutex->sleeping);
    SIMIX_synchro_destroy(p->waiting_action);
    p->waiting_action = NULL;
    mutex->owner = p;
    SIMIX_simcall_answer(&p->simcall);
  } else {
    /* nobody to wake up */
    mutex->locked = 0;
    mutex->owner = NULL;
  }
  XBT_OUT();
}

void SIMIX_pre_mutex_destroy(smx_simcall_t simcall, smx_mutex_t mutex){
  SIMIX_mutex_destroy(mutex);
}
/**
 * \brief Destroys a mutex.
 *
 * Destroys and frees the mutex's memory. 
 * \param mutex A mutex
 */
void SIMIX_mutex_destroy(smx_mutex_t mutex)
{
  XBT_IN("(%p)",mutex);
  if (mutex){
    xbt_swag_free(mutex->sleeping);
    xbt_free(mutex);
  }
  XBT_OUT();
}

/********************************* Condition **********************************/

smx_cond_t SIMIX_pre_cond_init(smx_simcall_t simcall){
  return SIMIX_cond_init();
}
/**
 * \brief Initialize a condition.
 *
 * Allocates and creates the data for the condition.
 * It have to be called before the use of the condition.
 * \return A condition
 */
smx_cond_t SIMIX_cond_init(void)
{
  XBT_IN("()");
  s_smx_process_t p;
  smx_cond_t cond = xbt_new0(s_smx_cond_t, 1);
  cond->sleeping = xbt_swag_new(xbt_swag_offset(p, synchro_hookup));
  cond->mutex = NULL;
  XBT_OUT();
  return cond;
}

/**
 * \brief Handle a condition waiting simcall without timeouts
 * \param simcall the simcall
 */
void SIMIX_pre_cond_wait(smx_simcall_t simcall, smx_cond_t cond, smx_mutex_t mutex)
{
  XBT_IN("(%p)",simcall);
  smx_process_t issuer = simcall->issuer;

  _SIMIX_cond_wait(cond, mutex, -1, issuer, simcall);
  XBT_OUT();
}

/**
 * \brief Handle a condition waiting simcall with timeouts
 * \param simcall the simcall
 */
void SIMIX_pre_cond_wait_timeout(smx_simcall_t simcall, smx_cond_t cond,
		                 smx_mutex_t mutex, double timeout)
{
  XBT_IN("(%p)",simcall);
  smx_process_t issuer = simcall->issuer;

  _SIMIX_cond_wait(cond, mutex, timeout, issuer, simcall);
  XBT_OUT();
}


static void _SIMIX_cond_wait(smx_cond_t cond, smx_mutex_t mutex, double timeout,
                             smx_process_t issuer, smx_simcall_t simcall)
{
  XBT_IN("(%p, %p, %f, %p,%p)",cond,mutex,timeout,issuer,simcall);
  smx_action_t sync_act = NULL;

  XBT_DEBUG("Wait condition %p", cond);

  /* If there is a mutex unlock it */
  /* FIXME: what happens if the issuer is not the owner of the mutex? */
  if (mutex != NULL) {
    cond->mutex = mutex;
    SIMIX_mutex_unlock(mutex, issuer);
  }

  sync_act = SIMIX_synchro_wait(issuer->smx_host, timeout);
  xbt_fifo_unshift(sync_act->simcalls, simcall);
  issuer->waiting_action = sync_act;
  xbt_swag_insert(simcall->issuer, cond->sleeping);   
  XBT_OUT();
}

void SIMIX_pre_cond_signal(smx_simcall_t simcall, smx_cond_t cond){
  SIMIX_cond_signal(cond);
}
/**
 * \brief Signalizes a condition.
 *
 * Signalizes a condition and wakes up a sleeping process. 
 * If there are no process sleeping, no action is done.
 * \param cond A condition
 */
void SIMIX_cond_signal(smx_cond_t cond)
{
  XBT_IN("(%p)",cond);
  smx_process_t proc = NULL;
  smx_mutex_t mutex = NULL;
  smx_simcall_t simcall = NULL;

  XBT_DEBUG("Signal condition %p", cond);

  /* If there are processes waiting for the condition choose one and try 
     to make it acquire the mutex */
  if ((proc = xbt_swag_extract(cond->sleeping))) {

    /* Destroy waiter's synchro action */
    SIMIX_synchro_destroy(proc->waiting_action);
    proc->waiting_action = NULL;

    /* Now transform the cond wait simcall into a mutex lock one */
    simcall = &proc->simcall;
    if(simcall->call == SIMCALL_COND_WAIT)
      mutex = simcall_cond_wait__get__mutex(simcall);
    else
      mutex = simcall_cond_wait_timeout__get__mutex(simcall);
    simcall->call = SIMCALL_MUTEX_LOCK;

    SIMIX_pre_mutex_lock(simcall, mutex);
  }
  XBT_OUT();
}

void SIMIX_pre_cond_broadcast(smx_simcall_t simcall, smx_cond_t cond){
  SIMIX_cond_broadcast(cond);
}
/**
 * \brief Broadcasts a condition.
 *
 * Signal ALL processes waiting on a condition.
 * If there are no process waiting, no action is done.
 * \param cond A condition
 */
void SIMIX_cond_broadcast(smx_cond_t cond)
{
  XBT_IN("(%p)",cond);
  XBT_DEBUG("Broadcast condition %p", cond);

  /* Signal the condition until nobody is waiting on it */
  while (xbt_swag_size(cond->sleeping)) {
    SIMIX_cond_signal(cond);
  }
  XBT_OUT();
}

void SIMIX_pre_cond_destroy(smx_simcall_t simcall, smx_cond_t cond){
  SIMIX_cond_destroy(cond);
}
/**
 * \brief Destroys a contidion.
 *
 * Destroys and frees the condition's memory. 
 * \param cond A condition
 */
void SIMIX_cond_destroy(smx_cond_t cond)
{
  XBT_IN("(%p)",cond);
  XBT_DEBUG("Destroy condition %p", cond);

  if (cond != NULL) {
    xbt_assert(xbt_swag_size(cond->sleeping) == 0,
                "Cannot destroy conditional since someone is still using it");

    xbt_swag_free(cond->sleeping);
    xbt_free(cond);
  }
  XBT_OUT();
}

/******************************** Semaphores **********************************/
#define SMX_SEM_NOLIMIT 99999
smx_sem_t SIMIX_pre_sem_init(smx_simcall_t simcall, unsigned int value){
  return SIMIX_sem_init(value);
}
/** @brief Initialize a semaphore */
smx_sem_t SIMIX_sem_init(unsigned int value)
{
  XBT_IN("(%u)",value);
  s_smx_process_t p;

  smx_sem_t sem = xbt_new0(s_smx_sem_t, 1);
  sem->sleeping = xbt_swag_new(xbt_swag_offset(p, synchro_hookup));
  sem->value = value;
  XBT_OUT();
  return sem;
}

void SIMIX_pre_sem_destroy(smx_simcall_t simcall, smx_sem_t sem){
  SIMIX_sem_destroy(sem);
}
/** @brief Destroys a semaphore */
void SIMIX_sem_destroy(smx_sem_t sem)
{
  XBT_IN("(%p)",sem);
  XBT_DEBUG("Destroy semaphore %p", sem);
  if (sem != NULL) {
    xbt_assert(xbt_swag_size(sem->sleeping) == 0,
                "Cannot destroy semaphore since someone is still using it");
    xbt_swag_free(sem->sleeping);
    xbt_free(sem);
  }
  XBT_OUT();
}

void SIMIX_pre_sem_release(smx_simcall_t simcall, smx_sem_t sem){
  SIMIX_sem_release(sem);
}
/** @brief release the semaphore
 *
 * Unlock a process waiting on the semaphore.
 * If no one was blocked, the semaphore capacity is increased by 1.
 */
void SIMIX_sem_release(smx_sem_t sem)
{
  XBT_IN("(%p)",sem);
  smx_process_t proc;

  XBT_DEBUG("Sem release semaphore %p", sem);
  if ((proc = xbt_swag_extract(sem->sleeping))) {
    SIMIX_synchro_destroy(proc->waiting_action);
    proc->waiting_action = NULL;
    SIMIX_simcall_answer(&proc->simcall);
  } else if (sem->value < SMX_SEM_NOLIMIT) {
    sem->value++;
  }
  XBT_OUT();
}

/** @brief Returns true if acquiring this semaphore would block */
XBT_INLINE int SIMIX_sem_would_block(smx_sem_t sem)
{
  XBT_IN("(%p)",sem);
  XBT_OUT();
  return (sem->value <= 0);
}

int SIMIX_pre_sem_get_capacity(smx_simcall_t simcall, smx_sem_t sem){
  return SIMIX_sem_get_capacity(sem);
}
/** @brief Returns the current capacity of the semaphore */
int SIMIX_sem_get_capacity(smx_sem_t sem)
{
  XBT_IN("(%p)",sem);
  XBT_OUT();
  return sem->value;
}

static void _SIMIX_sem_wait(smx_sem_t sem, double timeout, smx_process_t issuer,
                            smx_simcall_t simcall)
{
  XBT_IN("(%p, %f, %p, %p)",sem,timeout,issuer,simcall);
  smx_action_t sync_act = NULL;

  XBT_DEBUG("Wait semaphore %p (timeout:%f)", sem, timeout);
  if (sem->value <= 0) {
    sync_act = SIMIX_synchro_wait(issuer->smx_host, timeout);
    xbt_fifo_unshift(sync_act->simcalls, simcall);
    issuer->waiting_action = sync_act;
    xbt_swag_insert(issuer, sem->sleeping);
  } else {
    sem->value--;
    SIMIX_simcall_answer(simcall);
  }
  XBT_OUT();
}

/**
 * \brief Handles a sem acquire simcall without timeout.
 * \param simcall the simcall
 */
void SIMIX_pre_sem_acquire(smx_simcall_t simcall, smx_sem_t sem)
{
  XBT_IN("(%p)",simcall);
  _SIMIX_sem_wait(sem, -1, simcall->issuer, simcall);
  XBT_OUT();
}

/**
 * \brief Handles a sem acquire simcall with timeout.
 * \param simcall the simcall
 */
void SIMIX_pre_sem_acquire_timeout(smx_simcall_t simcall, smx_sem_t sem, double timeout)
{
  XBT_IN("(%p)",simcall);
  _SIMIX_sem_wait(sem, timeout, simcall->issuer, simcall);  
  XBT_OUT();
}
