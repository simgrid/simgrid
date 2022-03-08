/* Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "Simcall.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"
#include "src/kernel/context/Context.hpp"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ker_simcall, kernel, "transmuting from user request into kernel handlers");

namespace simgrid {
namespace kernel {
namespace actor {

/** @private
 * @brief (in kernel mode) unpack the simcall and activate the handler
 *
 */
void ActorImpl::simcall_handle(int times_considered)
{
  XBT_DEBUG("Handling simcall %p: %s", &simcall_, simcall_.get_cname());
  if (simcall_.observer_ != nullptr)
    simcall_.observer_->prepare(times_considered);
  if (context_->wannadie())
    return;

  xbt_assert(simcall_.call_ != Simcall::Type::NONE, "Asked to do the noop syscall on %s@%s", get_cname(),
             get_host()->get_cname());

  (*simcall_.code_)();
  if (simcall_.call_ == Simcall::Type::RUN_ANSWERED)
    simcall_answer();
}

/** @brief returns a printable string representing a simcall */
const char* Simcall::get_cname() const
{
  if (observer_ != nullptr) {
    static std::string name;
    name              = boost::core::demangle(typeid(*observer_).name());
    const char* cname = name.c_str();
    if (name.rfind("simgrid::kernel::", 0) == 0)
      cname += 17; // strip prefix "simgrid::kernel::"
    return cname;
  } else {
    return to_c_str(call_);
  }
}

} // namespace actor
} // namespace kernel
} // namespace simgrid
