/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SG_POPPING_PRIVATE_HPP
#define SG_POPPING_PRIVATE_HPP

#include "simgrid/forward.h"
#include "src/kernel/activity/ActivityImpl.hpp"

#include <array>
#include <boost/intrusive_ptr.hpp>

/********************************* Simcalls *********************************/
namespace simgrid {
namespace simix {
/** All possible simcalls. */
enum class Simcall {
  NONE,
  RUN_KERNEL,
  RUN_BLOCKING,
};
constexpr int NUM_SIMCALLS = 3;
/** @brief Simcalls' names */
} // namespace simix
} // namespace simgrid

XBT_PUBLIC_DATA const std::array<const char*, simgrid::simix::NUM_SIMCALLS> simcall_names; /* Name of each simcall */

using simix_match_func_t     = bool (*)(void*, void*, simgrid::kernel::activity::CommImpl*);
using simix_copy_data_func_t = void (*)(simgrid::kernel::activity::CommImpl*, void*, size_t);
using simix_clean_func_t     = void (*)(void*);

/**
 * @brief Represents a simcall to the kernel.
 */
struct s_smx_simcall {
  simgrid::simix::Simcall call_                      = simgrid::simix::Simcall::NONE;
  smx_actor_t issuer_                                = nullptr;
  simgrid::kernel::timer::Timer* timeout_cb_         = nullptr; // Callback to timeouts
  simgrid::kernel::actor::SimcallObserver* observer_ = nullptr; // makes that simcall observable by the MC
  unsigned int mc_max_consider_ =
      0; // How many times this simcall should be used. If >1, this will be a fork in the state space.
  std::function<void()> const* code_      = nullptr;
};

/******************************** General *************************************/

XBT_PRIVATE const char* SIMIX_simcall_name(const s_smx_simcall& simcall);
XBT_PRIVATE void SIMIX_run_kernel(std::function<void()> const* code);
XBT_PRIVATE void SIMIX_run_blocking(std::function<void()> const* code);

#endif
