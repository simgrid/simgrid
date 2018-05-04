/* Copyright (c) 2012-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_ACTIVITY_CONDITIONVARIABLEIMPL_HPP
#define SIMGRID_KERNEL_ACTIVITY_CONDITIONVARIABLEIMPL_HPP

#include "simgrid/s4u/ConditionVariable.hpp"
#include "src/simix/ActorImpl.hpp"
#include <boost/intrusive/list.hpp>

namespace simgrid {
namespace kernel {
namespace activity {

class XBT_PUBLIC ConditionVariableImpl {
public:
  ConditionVariableImpl();
  ~ConditionVariableImpl();

  simgrid::kernel::actor::SynchroList sleeping; /* list of sleeping processes */
  smx_mutex_t mutex = nullptr;
  simgrid::s4u::ConditionVariable cond_;

  void broadcast();
  void signal();

private:
  std::atomic_int_fast32_t refcount_{1};
  friend void intrusive_ptr_add_ref(ConditionVariableImpl* cond);
  friend void intrusive_ptr_release(ConditionVariableImpl* cond);
};
} // namespace activity
} // namespace kernel
} // namespace simgrid

#endif
