/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_SIMCALL_HPP
#define SIMGRID_SIMCALL_HPP

#include "simgrid/forward.h"
#include "xbt/utility.hpp"

namespace simgrid::kernel::actor {

/** Contains what's needed to run some code in kernel mode on behalf of an actor */
class Simcall {
public:
  /** All possible simcalls. */
  XBT_DECLARE_ENUM_CLASS(Type, NONE, RUN_ANSWERED, RUN_BLOCKING);

  Type call_                                         = Type::NONE;
  simgrid::kernel::actor::ActorImpl* issuer_         = nullptr;
  simgrid::kernel::timer::Timer* timeout_cb_         = nullptr; // Callback to timeouts
  simgrid::kernel::actor::SimcallObserver* observer_ = nullptr; // makes that simcall observable by the MC
  std::function<void()> const* code_ = nullptr;

  const char* get_cname() const;
};

/** A thing that can be used for an ObjectAccess simcall (getter or setter). */
class ObjectAccessSimcallItem {
public:
  ObjectAccessSimcallItem();
  void take_ownership();
  ActorImpl* simcall_owner_;
};

} // namespace simgrid::kernel::actor

#endif
