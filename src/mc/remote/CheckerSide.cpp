/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/remote/CheckerSide.hpp"
#include <csignal>
#include <sys/wait.h>

namespace simgrid::mc {

void CheckerSide::start(void (*handler_sock)(int, short, void*), void (*handler_sig)(int, short, void*),
                        ModelChecker* mc)
{
  auto* base = event_base_new();
  base_.reset(base);

  auto* socket_event = event_new(base, get_channel().get_socket(), EV_READ | EV_PERSIST, handler_sock, this);
  event_add(socket_event, nullptr);
  socket_event_.reset(socket_event);

  auto* signal_event = event_new(base, SIGCHLD, EV_SIGNAL | EV_PERSIST, handler_sig, mc);
  event_add(signal_event, nullptr);
  signal_event_.reset(signal_event);
}

void CheckerSide::dispatch() const
{
  event_base_dispatch(base_.get());
}

void CheckerSide::break_loop() const
{
  event_base_loopbreak(base_.get());
}

} // namespace simgrid::mc
