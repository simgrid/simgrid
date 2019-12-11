/* Copyright (c) 2006-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"

#include "simgrid/s4u/Activity.hpp"
#include "simgrid/s4u/Engine.hpp"

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

double Activity::get_remaining()
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
