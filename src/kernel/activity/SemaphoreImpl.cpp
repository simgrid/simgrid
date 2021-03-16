/* Copyright (c) 2019-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/SemaphoreImpl.hpp"
#include "src/kernel/activity/SynchroRaw.hpp"
#include <cmath> // std::isfinite

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_semaphore, simix_synchro, "Semaphore kernel-space implementation");

namespace simgrid {
namespace kernel {
namespace activity {

void SemaphoreImpl::acquire(actor::ActorImpl* issuer, double timeout)
{
  XBT_DEBUG("Wait semaphore %p (timeout:%f)", this, timeout);
  xbt_assert(std::isfinite(timeout), "timeout is not finite!");
  simix::marshal<bool>(issuer->simcall_.result_, false); // default result, will be set to 'true' on timeout

  if (value_ <= 0) {
    RawImplPtr synchro(new RawImpl([this, issuer]() {
      this->remove_sleeping_actor(*issuer);
      simix::marshal<bool>(issuer->simcall_.result_, true);
    }));
    synchro->set_host(issuer->get_host()).set_timeout(timeout).start();
    synchro->register_simcall(&issuer->simcall_);
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
    actor.waiting_synchro_ = nullptr;
    actor.simcall_answer();
  } else {
    value_++;
  }
}

/** Increase the refcount for this semaphore */
SemaphoreImpl* SemaphoreImpl::ref()
{
  intrusive_ptr_add_ref(this);
  return this;
}

/** Decrease the refcount for this mutex */
void SemaphoreImpl::unref()
{
  intrusive_ptr_release(this);
}

} // namespace activity
} // namespace kernel
} // namespace simgrid
