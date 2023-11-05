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
  event* socket_event_;
  event* signal_event_;
  std::unique_ptr<event_base, decltype(&event_base_free)> base_{nullptr, &event_base_free};

  Channel channel_;
  bool running_ = false;
  pid_t pid_;
  // When forking (no meminfo), the real app is our grandchild. In this case,
  // child_checker_ is a CheckerSide to our child that can waitpid our grandchild on our behalf
  CheckerSide* child_checker_ = nullptr;

  void setup_events(bool socket_only); // Part of the initialization
  void handle_dead_child(int status); // Launched when the dying child is the PID we follow
  void handle_waitpid();              // Launched when receiving a sigchild

public:
  explicit CheckerSide(int socket, CheckerSide* child_checker);
  explicit CheckerSide(const std::vector<char*>& args);
  ~CheckerSide();

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
  void wait_for_requests();

  /* Create a new CheckerSide by forking the currently existing one, and connect it through the master_socket */
  std::unique_ptr<CheckerSide> clone(int master_socket, const std::string& master_socket_name);

  /** Ask the application to run post-mortem analysis, and maybe to stop ASAP */
  void finalize(bool terminate_asap = false);

  /* Interacting with the application process */
  pid_t get_pid() const { return pid_; }
  bool running() const { return running_; }
  void terminate() { running_ = false; }
};

} // namespace simgrid::mc

#endif
