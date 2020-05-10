/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_REMOTE_EVENTLOOP_HPP
#define SIMGRID_MC_REMOTE_EVENTLOOP_HPP

#include "src/mc/remote/Channel.hpp"

#include <event2/event.h>
#include <functional>

namespace simgrid {
namespace mc {

class CheckerSide {
  struct event_base* base_    = nullptr;
  struct event* socket_event_ = nullptr;
  struct event* signal_event_ = nullptr;

  Channel channel_;

public:
  explicit CheckerSide(int sockfd) : channel_(sockfd) {}
  ~CheckerSide();

  // No copy:
  CheckerSide(CheckerSide const&) = delete;
  CheckerSide& operator=(CheckerSide const&) = delete;
  CheckerSide& operator=(CheckerSide&&) = delete;

  Channel const& get_channel() const { return channel_; }
  Channel& get_channel() { return channel_; }

  void start(void (*handler)(int, short, void*));
  void dispatch();
  void break_loop();
};

} // namespace mc
} // namespace simgrid

#endif
