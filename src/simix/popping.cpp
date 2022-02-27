/* Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Host.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"
#include "src/kernel/context/Context.hpp"
#include "src/simix/popping_private.hpp"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(simix, "transmuting from user request into kernel handlers");

constexpr std::array<const char*, simgrid::simix::NUM_SIMCALLS> simcall_names{{
    "Simcall::NONE",
    "Simcall::RUN_KERNEL",
    "Simcall::RUN_BLOCKING",
}};

/** @private
 * @brief (in kernel mode) unpack the simcall and activate the handler
 *
 * This function is generated from src/simix/simcalls.in
 */
void simgrid::kernel::actor::ActorImpl::simcall_handle(int times_considered)
{
  XBT_DEBUG("Handling simcall %p: %s", &simcall_, SIMIX_simcall_name(simcall_));
  if (simcall_.observer_ != nullptr)
    simcall_.observer_->prepare(times_considered);
  if (context_->wannadie())
    return;
  switch (simcall_.call_) {
    case simgrid::simix::Simcall::RUN_KERNEL:
      SIMIX_run_kernel(simcall_.code_);
      simcall_answer();
      break;

    case simgrid::simix::Simcall::RUN_BLOCKING:
      SIMIX_run_blocking(simcall_.code_);
      break;

    case simgrid::simix::Simcall::NONE:
      throw std::invalid_argument(
          simgrid::xbt::string_printf("Asked to do the noop syscall on %s@%s", get_cname(), get_host()->get_cname()));
    default:
      THROW_IMPOSSIBLE;
  }
}

void SIMIX_run_kernel(std::function<void()> const* code)
{
  (*code)();
}

/** Kernel code for run_blocking
 *
 * The implementation looks a lot like SIMIX_run_kernel ^^
 *
 * However, this `run_blocking` is blocking so the process will not be woken
 * up until `ActorImpl::simcall_answer()`` is called by the kernel.
 * This means that `code` is responsible for doing this.
 */
void SIMIX_run_blocking(std::function<void()> const* code)
{
  (*code)();
}
