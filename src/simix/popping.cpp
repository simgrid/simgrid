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
    "Simcall::RUN_ANSWERED",
    "Simcall::RUN_BLOCKING",
}};

/** @private
 * @brief (in kernel mode) unpack the simcall and activate the handler
 *
 */
void simgrid::kernel::actor::ActorImpl::simcall_handle(int times_considered)
{
  XBT_DEBUG("Handling simcall %p: %s", &simcall_, SIMIX_simcall_name(simcall_));
  if (simcall_.observer_ != nullptr)
    simcall_.observer_->prepare(times_considered);
  if (context_->wannadie())
    return;

  xbt_assert(simcall_.call_ != simgrid::simix::Simcall::NONE, "Asked to do the noop syscall on %s@%s", get_cname(),
             get_host()->get_cname());

  (*simcall_.code_)();
  if (simcall_.call_ == simgrid::simix::Simcall::RUN_ANSWERED)
    simcall_answer();
}

/** @brief returns a printable string representing a simcall */
const char* SIMIX_simcall_name(const s_smx_simcall& simcall)
{
  if (simcall.observer_ != nullptr) {
    static std::string name;
    name              = boost::core::demangle(typeid(*simcall.observer_).name());
    const char* cname = name.c_str();
    if (name.rfind("simgrid::kernel::", 0) == 0)
      cname += 17; // strip prefix "simgrid::kernel::"
    return cname;
  } else {
    return simcall_names.at(static_cast<int>(simcall.call_));
  }
}
