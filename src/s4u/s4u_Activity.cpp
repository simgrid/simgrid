/* Copyright (c) 2006-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"

#include "simgrid/s4u/Activity.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "src/kernel/activity/ActivityImpl.hpp"

XBT_LOG_EXTERNAL_CATEGORY(s4u);
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_activity, s4u, "S4U activities");

namespace simgrid {
namespace s4u {

void Activity::wait_until(double time_limit)
{
  double now = Engine::get_clock();
  if (time_limit > now)
    wait_for(time_limit - now);
}

bool Activity::test()
{
  xbt_assert(state_ == State::INITED || state_ == State::STARTED || state_ == State::STARTING ||
             state_ == State::FINISHED);

  if (state_ == State::FINISHED)
    return true;

  if (state_ == State::INITED || state_ == State::STARTING)
    this->vetoable_start();

  if (kernel::actor::simcall([this] { return this->get_impl()->test(); })) {
    state_ = State::FINISHED;
    this->release_dependencies();
    return true;
  }

  return false;
}

double Activity::get_remaining() const
{
  return remains_;
}

Activity* Activity::set_remaining(double remains)
{
  xbt_assert(state_ == State::INITED, "Cannot change the remaining amount of work once the Activity is started");
  remains_ = remains;
  return this;
}

} // namespace s4u
} // namespace simgrid
