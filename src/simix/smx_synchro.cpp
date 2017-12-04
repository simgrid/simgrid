/* Copyright (c) 2007-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smx_private.hpp"
#include "src/surf/cpu_interface.hpp"
#include "src/surf/surf_interface.hpp"
#include <xbt/ex.hpp>
#include <xbt/log.h>
#include <xbt/utility.hpp>

#include "src/kernel/activity/SynchroRaw.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_synchro, simix, "SIMIX Synchronization (mutex, semaphores and conditions)");

static smx_activity_t SIMIX_synchro_wait(sg_host_t smx_host, double timeout);
static void _SIMIX_cond_wait(smx_cond_t cond, smx_mutex_t mutex, double timeout,
                             smx_actor_t issuer, smx_simcall_t simcall);
static void _SIMIX_sem_wait(smx_sem_t sem, double timeout, smx_actor_t issuer,
                            smx_simcall_t simcall);

/***************************** Raw synchronization *********************************/

static smx_activity_t SIMIX_synchro_wait(sg_host_t smx_host, double timeout)
{
  XBT_IN("(%p, %f)",smx_host,timeout);

  simgrid::kernel::activity::RawImplPtr sync =
      simgrid::kernel::activity::RawImplPtr(new simgrid::kernel::activity::RawImpl());
  sync->sleep                          = smx_host->pimpl_cpu->sleep(timeout);
  sync->sleep->setData(sync.get());
  XBT_OUT();
  return sync;
}

void SIMIX_synchro_stop_waiting(smx_actor_t process, smx_simcall_t simcall)
{
  XBT_IN("(%p, %p)",process,simcall);
  switch (simcall->call) {

    case SIMCALL_MUTEX_LOCK:
      simgrid::xbt::intrusive_erase(simcall_mutex_lock__get__mutex(simcall)->sleeping, *process);
      break;

    case SIMCALL_COND_WAIT:
      simgrid::xbt::intrusive_erase(simcall_cond_wait__get__cond(simcall)->sleeping, *process);
      break;

    case SIMCALL_COND_WAIT_TIMEOUT:
      simgrid::xbt::intrusive_erase(simcall_cond_wait_timeout__get__cond(simcall)->sleeping, *process);
      break;

    case SIMCALL_SEM_ACQUIRE:
      simgrid::xbt::intrusive_erase(simcall_sem_acquire__get__sem(simcall)->sleeping, *process);
      break;

    case SIMCALL_SEM_ACQUIRE_TIMEOUT:
      simgrid::xbt::intrusive_erase(simcall_sem_acquire_timeout__get__sem(simcall)->sleeping, *process);
      break;

    default:
      THROW_IMPOSSIBLE;
  }
  XBT_OUT();
}

void SIMIX_synchro_finish(smx_activity_t synchro)
{
  XBT_IN("(%p)", synchro.get());
  smx_simcall_t simcall = synchro->simcalls.front();
  synchro->simcalls.pop_front();

  switch (synchro->state) {

    case SIMIX_SRC_TIMEOUT:
      SMX_EXCEPTION(simcall->issuer, timeout_error, 0, "Synchro's wait timeout");
      break;

    case SIMIX_FAILED:
        simcall->issuer->context->iwannadie = 1;
      break;

    default:
      THROW_IMPOSSIBLE;
      break;
  }

  SIMIX_synchro_stop_waiting(simcall->issuer, simcall);
  simcall->issuer->waiting_synchro = nullptr;
  SIMIX_simcall_answer(simcall);
  XBT_OUT();
}
/*********************************** Mutex ************************************/

namespace simgrid {
namespace simix {

MutexImpl::MutexImpl() : mutex_(this)
{
  XBT_IN("(%p)", this);
  XBT_OUT();
}

MutexImpl::~MutexImpl()
{
  XBT_IN("(%p)", this);
  XBT_OUT();
}

void MutexImpl::lock(smx_actor_t issuer)
{
  XBT_IN("(%p; %p)", this, issuer);
  /* FIXME: check where to validate the arguments */
  smx_activity_t synchro = nullptr;

  if (this->locked) {
    /* FIXME: check if the host is active ? */
    /* Somebody using the mutex, use a synchronization to get host failures */
    synchro = SIMIX_synchro_wait(issuer->host, -1);
    synchro->simcalls.push_back(&issuer->simcall);
    issuer->waiting_synchro = synchro;
    this->sleeping.push_back(*issuer);
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
bool MutexImpl::try_lock(smx_actor_t issuer)
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
void MutexImpl::unlock(smx_actor_t issuer)
{
  XBT_IN("(%p, %p)", this, issuer);
  if (not this->locked)
    THROWF(mismatch_error, 0, "Cannot release that mutex: it was not locked.");

  /* If the mutex is not owned by the issuer, that's not good */
  if (issuer != this->owner)
    THROWF(mismatch_error, 0, "Cannot release that mutex: it was locked by %s (pid:%lu), not by you.",
           this->owner->getCname(), this->owner->pid);

  if (not this->sleeping.empty()) {
    /*process to wake up */
    smx_actor_t p = &this->sleeping.front();
    this->sleeping.pop_front();
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
smx_cond_t SIMIX_cond_init()
{
  XBT_IN("()");
  smx_cond_t cond = new s_smx_cond_t();
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
  smx_actor_t issuer = simcall->issuer;

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
  smx_actor_t issuer = simcall->issuer;

  _SIMIX_cond_wait(cond, mutex, timeout, issuer, simcall);
  XBT_OUT();
}


static void _SIMIX_cond_wait(smx_cond_t cond, smx_mutex_t mutex, double timeout,
                             smx_actor_t issuer, smx_simcall_t simcall)
{
  XBT_IN("(%p, %p, %f, %p,%p)",cond,mutex,timeout,issuer,simcall);
  smx_activity_t synchro = nullptr;

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
  cond->sleeping.push_back(*simcall->issuer);
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
  XBT_DEBUG("Signal condition %p", cond);

  /* If there are processes waiting for the condition choose one and try
     to make it acquire the mutex */
  if (not cond->sleeping.empty()) {
    auto& proc = cond->sleeping.front();
    cond->sleeping.pop_front();

    /* Destroy waiter's synchronization */
    proc.waiting_synchro = nullptr;

    /* Now transform the cond wait simcall into a mutex lock one */
    smx_simcall_t simcall = &proc.simcall;
    smx_mutex_t mutex;
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
  while (not cond->sleeping.empty()) {
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
  auto previous = cond->refcount_.fetch_add(1);
  xbt_assert(previous != 0);
}

void intrusive_ptr_release(s_smx_cond_t *cond)
{
  if (cond->refcount_.fetch_sub(1) == 1) {
    xbt_assert(cond->sleeping.empty(), "Cannot destroy conditional since someone is still using it");
    delete cond;
  }
}

/******************************** Semaphores **********************************/
/** @brief Initialize a semaphore */
smx_sem_t SIMIX_sem_init(unsigned int value)
{
  XBT_IN("(%u)",value);
  smx_sem_t sem = new s_smx_sem_t;
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
    xbt_assert(sem->sleeping.empty(), "Cannot destroy semaphore since someone is still using it");
    delete sem;
  }
  XBT_OUT();
}

/** @brief release the semaphore
 *
 * Unlock a process waiting on the semaphore.
 * If no one was blocked, the semaphore capacity is increased by 1.
 */
void SIMIX_sem_release(smx_sem_t sem)
{
  XBT_IN("(%p)",sem);
  XBT_DEBUG("Sem release semaphore %p", sem);
  if (not sem->sleeping.empty()) {
    auto& proc = sem->sleeping.front();
    sem->sleeping.pop_front();
    proc.waiting_synchro = nullptr;
    SIMIX_simcall_answer(&proc.simcall);
  } else {
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

/** @brief Returns the current capacity of the semaphore */
int SIMIX_sem_get_capacity(smx_sem_t sem)
{
  XBT_IN("(%p)",sem);
  XBT_OUT();
  return sem->value;
}

static void _SIMIX_sem_wait(smx_sem_t sem, double timeout, smx_actor_t issuer,
                            smx_simcall_t simcall)
{
  XBT_IN("(%p, %f, %p, %p)",sem,timeout,issuer,simcall);
  smx_activity_t synchro = nullptr;

  XBT_DEBUG("Wait semaphore %p (timeout:%f)", sem, timeout);
  if (sem->value <= 0) {
    synchro = SIMIX_synchro_wait(issuer->host, timeout);
    synchro->simcalls.push_front(simcall);
    issuer->waiting_synchro = synchro;
    sem->sleeping.push_back(*issuer);
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
