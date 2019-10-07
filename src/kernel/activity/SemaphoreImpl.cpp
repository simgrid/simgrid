/* Copyright (c) 2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/SemaphoreImpl.hpp"
#include "src/kernel/activity/SynchroRaw.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_semaphore, simix_synchro, "Semaphore kernel-space implementation");

namespace simgrid {
namespace kernel {
namespace activity {

void SemaphoreImpl::acquire(actor::ActorImpl* issuer, double timeout)
{
  XBT_DEBUG("Wait semaphore %p (timeout:%f)", this, timeout);
  if (value_ <= 0) {
    RawImplPtr synchro = RawImplPtr(new RawImpl());
    synchro->set_host(issuer->get_host()).set_timeout(timeout).start();
    synchro->register_simcall(&issuer->simcall);
    sleeping_.push_back(*issuer);
  } else {
    value_--;
    issuer->simcall_answer();
  }
}
void SemaphoreImpl::release()
{
  XBT_DEBUG("Sem release semaphore %p", this);

  if (not sleeping_.empty()) {
    auto& actor = sleeping_.front();
    sleeping_.pop_front();
    actor.waiting_synchro = nullptr;
    actor.simcall_answer();
  } else {
    value_++;
  }
}

} // namespace activity
} // namespace kernel
} // namespace simgrid

// Simcall handlers:
/**
 * @brief Handles a sem acquire simcall without timeout.
 */
void simcall_HANDLER_sem_acquire(smx_simcall_t simcall, smx_sem_t sem)
{
  sem->acquire(simcall->issuer_, -1);
}

/**
 * @brief Handles a sem acquire simcall with timeout.
 */
void simcall_HANDLER_sem_acquire_timeout(smx_simcall_t simcall, smx_sem_t sem, double timeout)
{
  simcall_sem_acquire_timeout__set__result(simcall, 0); // default result, will be set to 1 on timeout
  sem->acquire(simcall->issuer_, timeout);
}
