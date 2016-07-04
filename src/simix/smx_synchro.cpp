
/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/surf_interface.hpp"
#include "smx_private.h"
#include <xbt/ex.hpp>
#include <xbt/log.h>

#include "src/simix/SynchroRaw.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_synchro, simix,
                                "SIMIX Synchronization (mutex, semaphores and conditions)");

static smx_synchro_t SIMIX_synchro_wait(sg_host_t smx_host, double timeout);
static void _SIMIX_cond_wait(smx_cond_t cond, smx_mutex_t mutex, double timeout,
                             smx_process_t issuer, smx_simcall_t simcall);
static void _SIMIX_sem_wait(smx_sem_t sem, double timeout, smx_process_t issuer,
                            smx_simcall_t simcall);

/***************************** Raw synchronization *********************************/

static smx_synchro_t SIMIX_synchro_wait(sg_host_t smx_host, double timeout)
{
  XBT_IN("(%p, %f)",smx_host,timeout);

  simgrid::simix::Raw *sync = new simgrid::simix::Raw();
  sync->sleep = surf_host_sleep(smx_host, timeout);
  sync->sleep->setData(sync);
  XBT_OUT();
  return sync;
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

void SIMIX_synchro_finish(smx_synchro_t synchro)
{
  XBT_IN("(%p)",synchro);
  smx_simcall_t simcall = synchro->simcalls.front();
  synchro->simcalls.pop_front();

  switch (synchro->state) {

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
  simcall->issuer->waiting_synchro = nullptr;
  delete synchro;
  SIMIX_simcall_answer(simcall);
  XBT_OUT();
}
/*********************************** Mutex ************************************/

namespace simgrid {
namespace simix {

Mutex::Mutex()
{
  XBT_IN("(%p)", this);
  // Useful to initialize sleeping swag:
  simgrid::simix::Process p;
  this->sleeping = xbt_swag_new(xbt_swag_offset(p, synchro_hookup));
  XBT_OUT();
}

Mutex::~Mutex()
{
  XBT_IN("(%p)", this);
  xbt_swag_free(this->sleeping);
  XBT_OUT();
}

void Mutex::lock(smx_process_t issuer)
{
  XBT_IN("(%p; %p)", this, issuer);
  /* FIXME: check where to validate the arguments */
  smx_synchro_t synchro = nullptr;

  if (this->locked) {
    /* FIXME: check if the host is active ? */
    /* Somebody using the mutex, use a synchronization to get host failures */
    synchro = SIMIX_synchro_wait(issuer->host, -1);
    synchro->simcalls.push_back(&issuer->simcall);
    issuer->waiting_synchro = synchro;
    xbt_swag_insert(issuer, this->sleeping);
  } else {
    /* mutex free */
    this->locked = true;
    this->owner = issuer;
    SIMIX_simcall_answer(&issuer->simcall);
  }
  XBT_OUT();
}

/** Tries to lock the mutex for a process
 *
 * \param  issuer  the process that tries to acquire the mutex
 * \return whether we managed to lock the mutex
 */
bool Mutex::try_lock(smx_process_t issuer)
{
  XBT_IN("(%p, %p)", this, issuer);
  if (this->locked) {
    XBT_OUT();
    return false;
  }

  this->locked = true;
  this->owner = issuer;
  XBT_OUT();
  return true;
}

/** Unlock a mutex for a process
 *
 * Unlocks the mutex and gives it to a process waiting for it. 
 * If the unlocker is not the owner of the mutex nothing happens.
 * If there are no process waiting, it sets the mutex as free.
 */
void Mutex::unlock(smx_process_t issuer)
{
  XBT_IN("(%p, %p)", this, issuer);

  /* If the mutex is not owned by the issuer, that's not good */
  if (issuer != this->owner)
    THROWF(mismatch_error, 0, "Cannot release that mutex: it was locked by %s (pid:%d), not by you.",
        SIMIX_process_get_name(this->owner),SIMIX_process_get_PID(this->owner));

  if (xbt_swag_size(this->sleeping) > 0) {
    /*process to wake up */
    smx_process_t p = (smx_process_t) xbt_swag_extract(this->sleeping);
    delete p->waiting_synchro;
    p->waiting_synchro = nullptr;
    this->owner = p;
    SIMIX_simcall_answer(&p->simcall);
  } else {
    /* nobody to wake up */
    this->locked = false;
    this->owner = nullptr;
  }
  XBT_OUT();
}

}
}

/** Increase the refcount for this mutex */
smx_mutex_t SIMIX_mutex_ref(smx_mutex_t mutex)
{
  if (mutex != nullptr)
    intrusive_ptr_add_ref(mutex);
  return mutex;
}

/** Decrease the refcount for this mutex */
void SIMIX_mutex_unref(smx_mutex_t mutex)
{
  if (mutex != nullptr)
    intrusive_ptr_release(mutex);
}

smx_mutex_t simcall_HANDLER_mutex_init(smx_simcall_t simcall)
{
  return new simgrid::simix::Mutex();
}

// Simcall handlers:

void simcall_HANDLER_mutex_lock(smx_simcall_t simcall, smx_mutex_t mutex)
{
  mutex->lock(simcall->issuer);
}

int simcall_HANDLER_mutex_trylock(smx_simcall_t simcall, smx_mutex_t mutex)
{
  return mutex->try_lock(simcall->issuer);
}

void simcall_HANDLER_mutex_unlock(smx_simcall_t simcall, smx_mutex_t mutex)
{
  mutex->unlock(simcall->issuer);
}

/********************************* Condition **********************************/

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
  simgrid::simix::Process p;
  smx_cond_t cond = xbt_new0(s_smx_cond_t, 1);
  cond->sleeping = xbt_swag_new(xbt_swag_offset(p, synchro_hookup));
  cond->mutex = nullptr;
  cond->refcount_ = 1;
  XBT_OUT();
  return cond;
}

/**
 * \brief Handle a condition waiting simcall without timeouts
 * \param simcall the simcall
 */
void simcall_HANDLER_cond_wait(smx_simcall_t simcall, smx_cond_t cond, smx_mutex_t mutex)
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
void simcall_HANDLER_cond_wait_timeout(smx_simcall_t simcall, smx_cond_t cond,
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
  smx_synchro_t synchro = nullptr;

  XBT_DEBUG("Wait condition %p", cond);

  /* If there is a mutex unlock it */
  /* FIXME: what happens if the issuer is not the owner of the mutex? */
  if (mutex != nullptr) {
    cond->mutex = mutex;
    mutex->unlock(issuer);
  }

  synchro = SIMIX_synchro_wait(issuer->host, timeout);
  synchro->simcalls.push_front(simcall);
  issuer->waiting_synchro = synchro;
  xbt_swag_insert(simcall->issuer, cond->sleeping);   
  XBT_OUT();
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
  smx_process_t proc = nullptr;
  smx_mutex_t mutex = nullptr;
  smx_simcall_t simcall = nullptr;

  XBT_DEBUG("Signal condition %p", cond);

  /* If there are processes waiting for the condition choose one and try 
     to make it acquire the mutex */
  if ((proc = (smx_process_t) xbt_swag_extract(cond->sleeping))) {

    /* Destroy waiter's synchronization */
    delete proc->waiting_synchro;
    proc->waiting_synchro = nullptr;

    /* Now transform the cond wait simcall into a mutex lock one */
    simcall = &proc->simcall;
    if(simcall->call == SIMCALL_COND_WAIT)
      mutex = simcall_cond_wait__get__mutex(simcall);
    else
      mutex = simcall_cond_wait_timeout__get__mutex(simcall);
    simcall->call = SIMCALL_MUTEX_LOCK;

    simcall_HANDLER_mutex_lock(simcall, mutex);
  }
  XBT_OUT();
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

smx_cond_t SIMIX_cond_ref(smx_cond_t cond)
{
  if (cond != nullptr)
    intrusive_ptr_add_ref(cond);
  return cond;
}

void SIMIX_cond_unref(smx_cond_t cond)
{
  XBT_IN("(%p)",cond);
  XBT_DEBUG("Destroy condition %p", cond);
  if (cond != nullptr) {
    intrusive_ptr_release(cond);
  }
  XBT_OUT();
}


void intrusive_ptr_add_ref(s_smx_cond_t *cond)
{
  auto previous = (cond->refcount_)++;
  xbt_assert(previous != 0);
  (void) previous;
}

void intrusive_ptr_release(s_smx_cond_t *cond)
{
  auto count = --(cond->refcount_);
  if (count == 0) {
    xbt_assert(xbt_swag_size(cond->sleeping) == 0,
                "Cannot destroy conditional since someone is still using it");

    xbt_swag_free(cond->sleeping);
    xbt_free(cond);
  }
}

/******************************** Semaphores **********************************/
#define SMX_SEM_NOLIMIT 99999
/** @brief Initialize a semaphore */
smx_sem_t SIMIX_sem_init(unsigned int value)
{
  XBT_IN("(%u)",value);
  simgrid::simix::Process p;

  smx_sem_t sem = xbt_new0(s_smx_sem_t, 1);
  sem->sleeping = xbt_swag_new(xbt_swag_offset(p, synchro_hookup));
  sem->value = value;
  XBT_OUT();
  return sem;
}

/** @brief Destroys a semaphore */
void SIMIX_sem_destroy(smx_sem_t sem)
{
  XBT_IN("(%p)",sem);
  XBT_DEBUG("Destroy semaphore %p", sem);
  if (sem != nullptr) {
    xbt_assert(xbt_swag_size(sem->sleeping) == 0,
                "Cannot destroy semaphore since someone is still using it");
    xbt_swag_free(sem->sleeping);
    xbt_free(sem);
  }
  XBT_OUT();
}

void simcall_HANDLER_sem_release(smx_simcall_t simcall, smx_sem_t sem){
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
  if ((proc = (smx_process_t) xbt_swag_extract(sem->sleeping))) {
    delete proc->waiting_synchro;
    proc->waiting_synchro = nullptr;
    SIMIX_simcall_answer(&proc->simcall);
  } else if (sem->value < SMX_SEM_NOLIMIT) {
    sem->value++;
  }
  XBT_OUT();
}

/** @brief Returns true if acquiring this semaphore would block */
int SIMIX_sem_would_block(smx_sem_t sem)
{
  XBT_IN("(%p)",sem);
  XBT_OUT();
  return (sem->value <= 0);
}

int simcall_HANDLER_sem_get_capacity(smx_simcall_t simcall, smx_sem_t sem){
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
  smx_synchro_t synchro = nullptr;

  XBT_DEBUG("Wait semaphore %p (timeout:%f)", sem, timeout);
  if (sem->value <= 0) {
    synchro = SIMIX_synchro_wait(issuer->host, timeout);
    synchro->simcalls.push_front(simcall);
    issuer->waiting_synchro = synchro;
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
void simcall_HANDLER_sem_acquire(smx_simcall_t simcall, smx_sem_t sem)
{
  XBT_IN("(%p)",simcall);
  _SIMIX_sem_wait(sem, -1, simcall->issuer, simcall);
  XBT_OUT();
}

/**
 * \brief Handles a sem acquire simcall with timeout.
 * \param simcall the simcall
 */
void simcall_HANDLER_sem_acquire_timeout(smx_simcall_t simcall, smx_sem_t sem, double timeout)
{
  XBT_IN("(%p)",simcall);
  _SIMIX_sem_wait(sem, timeout, simcall->issuer, simcall);  
  XBT_OUT();
}
int simcall_HANDLER_sem_would_block(smx_simcall_t simcall, smx_sem_t sem) {
  return SIMIX_sem_would_block(sem);
}
