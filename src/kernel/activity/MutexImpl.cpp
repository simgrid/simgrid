/* Copyright (c) 2007-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/MutexImpl.hpp"
#include "src/kernel/activity/SynchroRaw.hpp"
#include "src/simix/smx_synchro_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_mutex, simix_synchro, "Mutex kernel-space implementation");

namespace simgrid {
namespace kernel {
namespace activity {

MutexImpl::MutexImpl() : piface_(this)
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
    synchro = SIMIX_synchro_wait(issuer->host_, -1);
    synchro->simcalls_.push_back(&issuer->simcall);
    issuer->waiting_synchro = synchro;
    this->sleeping.push_back(*issuer);
  } else {
    /* mutex free */
    this->locked = true;
    this->owner  = issuer;
    SIMIX_simcall_answer(&issuer->simcall);
  }
  XBT_OUT();
}

/** Tries to lock the mutex for a process
 *
 * @param  issuer  the process that tries to acquire the mutex
 * @return whether we managed to lock the mutex
 */
bool MutexImpl::try_lock(smx_actor_t issuer)
{
  XBT_IN("(%p, %p)", this, issuer);
  if (this->locked) {
    XBT_OUT();
    return false;
  }

  this->locked = true;
  this->owner  = issuer;
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
    THROWF(mismatch_error, 0, "Cannot release that mutex: it was locked by %s (pid:%ld), not by you.",
           this->owner->get_cname(), this->owner->pid_);

  if (not this->sleeping.empty()) {
    /*process to wake up */
    smx_actor_t p = &this->sleeping.front();
    this->sleeping.pop_front();
    p->waiting_synchro = nullptr;
    this->owner        = p;
    SIMIX_simcall_answer(&p->simcall);
  } else {
    /* nobody to wake up */
    this->locked = false;
    this->owner  = nullptr;
  }
  XBT_OUT();
}
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
