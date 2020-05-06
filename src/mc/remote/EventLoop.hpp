/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_REMOTE_EVENTLOOP_HPP
#define SIMGRID_MC_REMOTE_EVENTLOOP_HPP

#include <event2/event.h>
#include <functional>

namespace simgrid {
namespace mc {

class EventLoop {
  struct event_base* base_    = nullptr;
  struct event* socket_event_ = nullptr;
  struct event* signal_event_ = nullptr;

public:
  ~EventLoop();

  void start(int socket, void (*handler)(int, short, void*));
  void dispatch();
  void break_loop();
};

} // namespace mc
} // namespace simgrid

#endif
