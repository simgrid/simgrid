/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_REMOTE_EVENTLOOP_HPP
#define SIMGRID_MC_REMOTE_EVENTLOOP_HPP

#include "src/mc/mc_forward.hpp"
#include "src/mc/remote/Channel.hpp"

#include <event2/event.h>
#include <functional>
#include <memory>

namespace simgrid {
namespace mc {

class CheckerSide {
  std::unique_ptr<event_base, decltype(&event_base_free)> base_{nullptr, &event_base_free};
  std::unique_ptr<event, decltype(&event_free)> socket_event_{nullptr, &event_free};
  std::unique_ptr<event, decltype(&event_free)> signal_event_{nullptr, &event_free};

  Channel channel_;

public:
  explicit CheckerSide(int sockfd) : channel_(sockfd) {}

  // No copy:
  CheckerSide(CheckerSide const&) = delete;
  CheckerSide& operator=(CheckerSide const&) = delete;
  CheckerSide& operator=(CheckerSide&&) = delete;

  Channel const& get_channel() const { return channel_; }
  Channel& get_channel() { return channel_; }

  void start(void (*handler)(int, short, void*), ModelChecker* mc);
  void dispatch() const;
  void break_loop() const;
};

} // namespace mc
} // namespace simgrid

#endif
