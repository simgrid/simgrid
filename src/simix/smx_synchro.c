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
XBT_INLINE int SIMIX_mutex_trylock(smx_mutex_t mutex)
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
XBT_INLINE void SIMIX_mutex_destroy(smx_mutex_t mutex)
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
 * Allocates and creates the data for the condition.
 * It have to be called before the use of the condition.
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

  DEBUG1("Wait condition %p", cond);

  /* If there is a mutex unlock it */
  if(mutex != NULL){
    cond->mutex = mutex;
    SIMIX_mutex_unlock(mutex);
  }
  
  /* Always create an action null in case there is a host failure */
  act_sleep = SIMIX_action_sleep(SIMIX_host_self(), -1);
  SIMIX_action_set_name(act_sleep,bprintf("Wait condition %p", cond));
  SIMIX_process_self()->waiting_action = act_sleep;
  SIMIX_register_action_to_condition(act_sleep, cond);
  __SIMIX_cond_wait(cond);
  SIMIX_process_self()->waiting_action = NULL;
  SIMIX_unregister_action_to_condition(act_sleep, cond);
  SIMIX_action_destroy(act_sleep);

  /* get the mutex again if necessary */
  if(mutex != NULL)
    SIMIX_mutex_lock(cond->mutex);

  return;
}

XBT_INLINE xbt_fifo_t SIMIX_cond_get_actions(smx_cond_t cond)
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

  DEBUG1("Timed wait condition %p", cond);

  /* If there is a mutex unlock it */
  if(mutex != NULL){
    cond->mutex = mutex;
    SIMIX_mutex_unlock(mutex);
  }

  if (max_duration >= 0) {
    act_sleep = SIMIX_action_sleep(SIMIX_host_self(), max_duration);
    SIMIX_action_set_name(act_sleep,bprintf("Timed wait condition %p (max_duration:%f)", cond,max_duration));
    SIMIX_register_action_to_condition(act_sleep, cond);
    SIMIX_process_self()->waiting_action = act_sleep;
    __SIMIX_cond_wait(cond);
    SIMIX_process_self()->waiting_action = NULL;
    SIMIX_unregister_action_to_condition(act_sleep, cond);
    if (SIMIX_action_get_state(act_sleep) == SURF_ACTION_DONE) {
      SIMIX_action_destroy(act_sleep);
      THROW1(timeout_error, 0, "Condition timeout after %f",max_duration);
    } else {
      SIMIX_action_destroy(act_sleep);
    }

  } else
    SIMIX_cond_wait(cond,NULL);

  /* get the mutex again if necessary */
  if(mutex != NULL)
    SIMIX_mutex_lock(cond->mutex);
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

/* ************************** Semaphores ************************************** */
#define SMX_SEM_NOLIMIT 99999
/** @brief Initialize a semaphore */
smx_sem_t SIMIX_sem_init(int capacity) {
  smx_sem_t sem = xbt_new0(s_smx_sem_t, 1);
  s_smx_process_t p;

  sem->sleeping = xbt_swag_new(xbt_swag_offset(p, synchro_hookup));
  sem->actions = xbt_fifo_new();
  sem->capacity = capacity;
  return sem;
}
/** @brief Destroys a semaphore */
void SIMIX_sem_destroy(smx_sem_t sem) {
  DEBUG1("Destroy semaphore %p", sem);
  if (sem == NULL)
    return;

  smx_action_t action = NULL;

  xbt_assert0(xbt_swag_size(sem->sleeping) == 0,
      "Cannot destroy semaphore since someone is still using it");
  xbt_swag_free(sem->sleeping);

  DEBUG1("%d actions registered", xbt_fifo_size(sem->actions));
  while((action=xbt_fifo_pop(sem->actions)))
    SIMIX_unregister_action_to_semaphore(action, sem);

  xbt_fifo_free(sem->actions);
  xbt_free(sem);
}

/** @brief release the semaphore
 *
 * The first locked process on this semaphore is unlocked.
 * If no one was blocked, the semaphore capacity is increased by 1.
 * */
void SIMIX_sem_release(smx_sem_t sem) {
	DEBUG1("Sem release semaphore %p", sem);
  if (xbt_swag_size(sem->sleeping) >= 1) {
    smx_process_t proc = xbt_swag_extract(sem->sleeping);
    xbt_swag_insert(proc, simix_global->process_to_run);
  } else if (sem->capacity != SMX_SEM_NOLIMIT) {
    sem->capacity++;
  }
}
/** @brief make sure the semaphore will never be blocking again
 *
 * This function is not really in the semaphore spirit. It makes
 * sure that the semaphore will never be blocking anymore.
 *
 * Releasing and acquiring the semaphore after calling this
 * function is a noop. Such "broken" semaphores are useful to
 * implement something between condition variables (with broadcast)
 * and semaphore (with memory). It's like a semaphore signaled for ever.
 *
 * There is no way to reset the semaphore to a more regular state afterward.
 * */
void SIMIX_sem_release_forever(smx_sem_t sem) {
  smx_process_t proc = NULL;
  smx_process_t proc_next = NULL;

  DEBUG1("Broadcast semaphore %p", sem);
  xbt_swag_foreach_safe(proc, proc_next, sem->sleeping) {
    xbt_swag_remove(proc, sem->sleeping);
    xbt_swag_insert(proc, simix_global->process_to_run);
  }
}

/**
 * \brief Low level wait on a semaphore
 *
 * This function does not test the capacity of the semaphore and direcly locks
 * the calling process on the semaphore (until someone call SIMIX_sem_release()
 * on this semaphore). Do not call this function if you did not attach any action
 * to this semaphore to be awaken. Note also that you may miss host failure if you
 * do not attach a dummy action beforehand. SIMIX_sem_acquire does all these
 * things for you so you it may be preferable to use.
 */
void SIMIX_sem_block_onto(smx_sem_t sem) {
  smx_process_t self = SIMIX_process_self();

  /* process status */
  self->sem = sem;
  xbt_swag_insert(self, sem->sleeping);
  SIMIX_process_yield();
  self->sem = NULL;
  while (self->suspended)
    SIMIX_process_yield();
}

/** @brief Returns true if acquiring this semaphore would block */
XBT_INLINE int SIMIX_sem_would_block(smx_sem_t sem) {
  return (sem->capacity>0);
}

/** @brief Returns the current capacity of the semaphore
 *
 * If it's negative, that's the amount of processes locked on the semaphore
 */
int SIMIX_sem_get_capacity(smx_sem_t sem){
  return sem->capacity;
}

/**
 * \brief Waits on a semaphore
 *
 * If the capacity>0, decrease the capacity.
 *
 * If capacity==0, locks the current process
 * until someone call SIMIX_sem_release() on this semaphore
 */
void SIMIX_sem_acquire(smx_sem_t sem) {
  smx_action_t act_sleep;

  DEBUG1("Wait semaphore %p", sem);

  if (sem->capacity == SMX_SEM_NOLIMIT)
    return; /* don't even decrease it if wide open */

  /* If capacity sufficient, decrease it */
  if (sem->capacity>0) {
    sem->capacity--;
    return;
  }

  sem->capacity--;
  /* Always create an action null in case there is a host failure */
  act_sleep = SIMIX_action_sleep(SIMIX_host_self(), -1);
  SIMIX_action_set_name(act_sleep,bprintf("Locked in semaphore %p", sem));
  SIMIX_process_self()->waiting_action = act_sleep;
  SIMIX_register_action_to_semaphore(act_sleep, sem);
  SIMIX_sem_block_onto(sem);
  SIMIX_process_self()->waiting_action = NULL;
  SIMIX_unregister_action_to_semaphore(act_sleep, sem);
  SIMIX_action_destroy(act_sleep);
  DEBUG1("End of Wait on semaphore %p", sem);
  sem->capacity++;
}
/**
 * \brief Tries to acquire a semaphore before a timeout
 *
 * Same behavior of #SIMIX_sem_acquire, but waits a maximum time and throws an timeout_error if it happens.
 */
void SIMIX_sem_acquire_timeout(smx_sem_t sem, double max_duration) {
  smx_action_t act_sleep;

  DEBUG2("Timed wait semaphore %p (timeout:%f)", sem,max_duration);

  if (sem->capacity == SMX_SEM_NOLIMIT)
    return; /* don't even decrease it if wide open */

  /* If capacity sufficient, decrease it */
  if (sem->capacity>0) {
    sem->capacity--;
    return;
  }

  if (max_duration >= 0) {
    sem->capacity--;
    act_sleep = SIMIX_action_sleep(SIMIX_host_self(), max_duration);
    SIMIX_action_set_name(act_sleep,bprintf("Timed wait semaphore %p (max_duration:%f)", sem,max_duration));
    SIMIX_register_action_to_semaphore(act_sleep, sem);
    SIMIX_process_self()->waiting_action = act_sleep;
    SIMIX_sem_block_onto(sem);
    SIMIX_process_self()->waiting_action = NULL;
    SIMIX_unregister_action_to_semaphore(act_sleep, sem);
    if (SIMIX_action_get_state(act_sleep) == SURF_ACTION_DONE) {
      SIMIX_action_destroy(act_sleep);
      THROW1(timeout_error, 0, "Semaphore acquire timeouted after %f",max_duration);
    } else {
      SIMIX_action_destroy(act_sleep);
    }
    sem->capacity++;

  } else
    SIMIX_sem_acquire(sem);
}
/**
 * \brief Blocks on a set of semaphore
 *
 * If any of the semaphores has some more capacity, it gets decreased.
 * If not, blocks until the capacity of one of the semaphores becomes more friendly.
 *
 * \return the rank in the dynar of the semaphore which just got locked from the set
 */
unsigned int SIMIX_sem_acquire_any(xbt_dynar_t sems) {
  smx_sem_t sem;
  unsigned int counter,result=-1;
  smx_action_t act_sleep;
  smx_process_t self = SIMIX_process_self();

  xbt_assert0(xbt_dynar_length(sems),
      "I refuse to commit sucide by locking on an **empty** set of semaphores!!");
  DEBUG1("Wait on semaphore set %p", sems);

  xbt_dynar_foreach(sems,counter,sem) {
    if (!SIMIX_sem_would_block(sem))
      SIMIX_sem_acquire(sem);
    return counter;
  }

  /* Always create an action null in case there is a host failure */
  act_sleep = SIMIX_action_sleep(SIMIX_host_self(), -1);
  SIMIX_action_set_name(act_sleep,bprintf("Locked in semaphore %p", sem));
  self->waiting_action = act_sleep;
  SIMIX_register_action_to_semaphore(act_sleep, xbt_dynar_get_as(sems,0,smx_sem_t));

  /* Get listed as member of all the provided semaphores */
  self->sem = (smx_sem_t)sems; /* FIXME: we pass a pointer to dynar where a pointer to sem is expected...*/
  xbt_dynar_foreach(sems,counter,sem) {
    xbt_swag_insert(self, sem->sleeping);
  }
  SIMIX_process_yield();
  while (self->suspended)
    SIMIX_process_yield();

  /* one of the semaphore unsuspended us -- great, let's search which one (and get out of the others) */
  self->sem = NULL;
  xbt_dynar_foreach(sems,counter,sem) {
    if (xbt_swag_belongs(self,sem->sleeping))
      xbt_swag_remove(self,sem->sleeping);
    else {
      xbt_assert0(result==-1,"More than one semaphore unlocked us. Dunno what to do");
      result = counter;
    }
  }
  xbt_assert0(counter!=-1,"Cannot find which semaphore unlocked me!");

  /* Destroy the waiting action */
  self->waiting_action = NULL;
  SIMIX_unregister_action_to_semaphore(act_sleep, xbt_dynar_get_as(sems,0,smx_sem_t));
  SIMIX_action_destroy(act_sleep);
  return result;
}
