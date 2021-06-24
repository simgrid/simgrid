/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/remote/CheckerSide.hpp"
#include <csignal>
#include <sys/wait.h>

namespace simgrid {
namespace mc {

CheckerSide::~CheckerSide()
{
  if (socket_event_ != nullptr)
    event_free(socket_event_);
  if (signal_event_ != nullptr)
    event_free(signal_event_);
  if (base_ != nullptr)
    event_base_free(base_);
}

void CheckerSide::start(void (*handler)(int, short, void*), ModelChecker* mc)
{
  base_ = event_base_new();

  socket_event_ = event_new(base_, get_channel().get_socket(), EV_READ | EV_PERSIST, handler, mc);
  event_add(socket_event_, nullptr);

  signal_event_ = event_new(base_, SIGCHLD, EV_SIGNAL | EV_PERSIST, handler, mc);
  event_add(signal_event_, nullptr);
}

void CheckerSide::dispatch()
{
  event_base_dispatch(base_);
}

void CheckerSide::break_loop()
{
  event_base_loopbreak(base_);
}

} // namespace mc
} // namespace simgrid
