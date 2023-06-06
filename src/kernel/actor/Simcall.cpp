/* Copyright (c) 2010-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/actor/Simcall.hpp"
#include "simgrid/modelchecker.h"
#include "simgrid/s4u/Host.hpp"
#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/actor/SimcallObserver.hpp"
#include "src/kernel/context/Context.hpp"
#include "src/mc/mc_replay.hpp"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ker_simcall, kernel, "transmuting from user request into kernel handlers");

namespace simgrid::kernel::actor {

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
ObjectAccessSimcallItem::ObjectAccessSimcallItem()
{
  take_ownership();
}
void ObjectAccessSimcallItem::take_ownership()
{
  simcall_owner_ = ActorImpl::self();
}

} // namespace simgrid::kernel::actor

static void simcall(simgrid::kernel::actor::Simcall::Type call, std::function<void()> const& code,
                    simgrid::kernel::actor::SimcallObserver* observer)
{
  auto* self               = simgrid::kernel::actor::ActorImpl::self();
  self->simcall_.call_     = call;
  self->simcall_.code_     = &code;
  self->simcall_.observer_ = observer;
  if (simgrid::kernel::EngineImpl::get_instance()->is_maestro(self)) {
    self->simcall_handle(0);
  } else {
    XBT_DEBUG("Yield process '%s' on simcall %s", self->get_cname(), self->simcall_.get_cname());
    self->yield();
  }
  self->simcall_.observer_ = nullptr;
}

void simcall_run_answered(std::function<void()> const& code, simgrid::kernel::actor::SimcallObserver* observer)
{
  // The function `code` is called in kernel mode (either because we are already in maestor or after a context switch)
  // and simcall_answer() is called
  simcall(simgrid::kernel::actor::Simcall::Type::RUN_ANSWERED, code, observer);
}

void simcall_run_blocking(std::function<void()> const& code, simgrid::kernel::actor::SimcallObserver* observer)
{
  // The function `code` is called in kernel mode (either because we are already in maestor or after a context switch)
  // BUT simcall_answer IS NOT CALLED
  simcall(simgrid::kernel::actor::Simcall::Type::RUN_BLOCKING, code, observer);
}

void simcall_run_object_access(std::function<void()> const& code, simgrid::kernel::actor::ObjectAccessSimcallItem* item)
{
  auto* self = simgrid::kernel::actor::ActorImpl::self();

  // We only need a simcall if the order of the setters is important (parallel run or MC execution).
  // Otherwise, just call the function with no simcall

  if (simgrid::kernel::context::Context::is_parallel() || MC_is_active() || MC_record_replay_is_active()) {
    simgrid::kernel::actor::ObjectAccessSimcallObserver observer{self, item};
    simcall(simgrid::kernel::actor::Simcall::Type::RUN_ANSWERED, code, &observer);
    item->take_ownership();
  } else {
    // don't return from the context-switch we don't do
    self->simcall_.call_     = simgrid::kernel::actor::Simcall::Type::RUN_BLOCKING;
    self->simcall_.code_     = &code;
    self->simcall_.observer_ = nullptr;
    self->simcall_handle(0);
  }
}
