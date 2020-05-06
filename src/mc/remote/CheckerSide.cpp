/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/remote/CheckerSide.hpp"
#include <signal.h>
#include <sys/wait.h>

simgrid::mc::CheckerSide::~CheckerSide()
{
  if (socket_event_ != nullptr)
    event_free(socket_event_);
  if (signal_event_ != nullptr)
    event_free(signal_event_);
  if (base_ != nullptr)
    event_base_free(base_);
}

void simgrid::mc::CheckerSide::start(int socket, void (*handler)(int, short, void*))
{
  base_ = event_base_new();

  socket_event_ = event_new(base_, socket, EV_READ | EV_PERSIST, handler, this);
  event_add(socket_event_, NULL);

  signal_event_ = event_new(base_, SIGCHLD, EV_SIGNAL | EV_PERSIST, handler, this);
  event_add(signal_event_, NULL);
}

void simgrid::mc::CheckerSide::dispatch()
{
  event_base_dispatch(base_);
}

void simgrid::mc::CheckerSide::break_loop()
{
  event_base_loopbreak(base_);
}
