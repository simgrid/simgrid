/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMCALL_HPP
#define SIMCALL_HPP

#include "simgrid/forward.h"
#include "src/kernel/activity/ActivityImpl.hpp"
#include "xbt/utility.hpp"

/********************************* Simcalls *********************************/
namespace simgrid {
namespace simix {

/**
 * @brief Represents a simcall to the kernel.
 */
class Simcall {
public:
  /** All possible simcalls. */
  XBT_DECLARE_ENUM_CLASS(Type, NONE, RUN_ANSWERED, RUN_BLOCKING);

  Type call_                                         = Type::NONE;
  smx_actor_t issuer_                                = nullptr;
  simgrid::kernel::timer::Timer* timeout_cb_         = nullptr; // Callback to timeouts
  simgrid::kernel::actor::SimcallObserver* observer_ = nullptr; // makes that simcall observable by the MC
  unsigned int mc_max_consider_ =
      0; // How many times this simcall should be used. If >1, this will be a fork in the state space.
  std::function<void()> const* code_ = nullptr;

  const char* get_cname() const;
};

} // namespace simix
} // namespace simgrid

#endif
