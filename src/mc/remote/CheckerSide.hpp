/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_REMOTE_EVENTLOOP_HPP
#define SIMGRID_MC_REMOTE_EVENTLOOP_HPP

#include "src/mc/mc_forward.hpp"
#include "src/mc/remote/Channel.hpp"

#include <event2/event.h>
#include <functional>
#include <memory>

namespace simgrid::mc {

/* CheckerSide: All what the checker needs to interact with a given application process */

class CheckerSide {
  std::unique_ptr<event_base, decltype(&event_base_free)> base_{nullptr, &event_base_free};
  std::unique_ptr<event, decltype(&event_free)> socket_event_{nullptr, &event_free};
  std::unique_ptr<event, decltype(&event_free)> signal_event_{nullptr, &event_free};
  std::unique_ptr<RemoteProcessMemory> remote_memory_;
  Channel channel_;

  bool running_ = false;
  pid_t pid_;

public:
  explicit CheckerSide(int sockfd, pid_t pid);

  // No copy:
  CheckerSide(CheckerSide const&) = delete;
  CheckerSide& operator=(CheckerSide const&) = delete;
  CheckerSide& operator=(CheckerSide&&) = delete;

  /* Communicating with the application */
  Channel const& get_channel() const { return channel_; }
  Channel& get_channel() { return channel_; }

  bool handle_message(const char* buffer, ssize_t size);
  void dispatch_events() const;
  void break_loop() const;

  /* Interacting with the application process */
  pid_t get_pid() const { return pid_; }
  bool running() const { return running_; }
  void terminate() { running_ = false; }
  void handle_waitpid();
  RemoteProcessMemory& get_remote_memory() { return *remote_memory_.get(); }
  void clear_memory_cache();
};

} // namespace simgrid::mc

#endif
