/* Copyright (c) 2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/activity/SemaphoreImpl.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_semaphore, simix_synchro, "Semaphore kernel-space implementation");

namespace simgrid {
namespace kernel {
namespace activity {

void SemaphoreImpl::release()
{
  XBT_DEBUG("Sem release semaphore %p", this);

  if (not sleeping_.empty()) {
    auto& actor = sleeping_.front();
    sleeping_.pop_front();
    actor.waiting_synchro = nullptr;
    SIMIX_simcall_answer(&actor.simcall);
  } else {
    value_++;
  }
}

} // namespace activity
} // namespace kernel
} // namespace simgrid
