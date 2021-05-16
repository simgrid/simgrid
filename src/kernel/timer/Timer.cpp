/* Copyright (c) 2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/Timer.hpp"

namespace simgrid {
namespace kernel {
namespace timer {

Timer* Timer::set(double date, xbt::Task<void()>&& callback)
{
  auto* timer    = new Timer(date, std::move(callback));
  timer->handle_ = kernel_timers().emplace(std::make_pair(date, timer));
  return timer;
}

/** @brief cancels a timer that was added earlier */
void Timer::remove()
{
  kernel_timers().erase(handle_);
  delete this;
}

} // namespace timer
} // namespace kernel
} // namespace simgrid
