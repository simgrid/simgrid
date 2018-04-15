/* Copyright (c) 2012-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_ACTIVITY_CONDITIONVARIABLEIMPL_HPP
#define SIMGRID_KERNEL_ACTIVITY_CONDITIONVARIABLEIMPL_HPP

#include "simgrid/s4u/ConditionVariable.hpp"
#include "src/simix/ActorImpl.hpp"
#include <boost/intrusive/list.hpp>

struct s_smx_cond_t {
  s_smx_cond_t() : cond_(this) {}

  std::atomic_int_fast32_t refcount_{1};
  smx_mutex_t mutex = nullptr;
  simgrid::kernel::actor::SynchroList sleeping; /* list of sleeping processes */
  simgrid::s4u::ConditionVariable cond_;
};

XBT_PRIVATE smx_cond_t SIMIX_cond_init();
XBT_PRIVATE void SIMIX_cond_broadcast(smx_cond_t cond);
XBT_PRIVATE void SIMIX_cond_signal(smx_cond_t cond);
XBT_PRIVATE void intrusive_ptr_add_ref(s_smx_cond_t* cond);
XBT_PRIVATE void intrusive_ptr_release(s_smx_cond_t* cond);

#endif
