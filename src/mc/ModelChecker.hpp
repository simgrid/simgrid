/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_MODEL_CHECKER_HPP
#define SIMGRID_MC_MODEL_CHECKER_HPP

#include <sys/types.h>

#include <poll.h>
#include <memory>

#include <simgrid_config.h>
#include <xbt/dict.h>
#include <xbt/base.h>
#include <sys/types.h>

#include "src/mc/mc_forward.hpp"
#include "src/mc/Process.hpp"
#include "src/mc/PageStore.hpp"
#include "src/mc/mc_protocol.h"

namespace simgrid {
namespace mc {

/** State of the model-checker (global variables for the model checker)
 */
class ModelChecker {
  struct pollfd fds_[2];
  /** String pool for host names */
  // TODO, use std::set with heterogeneous comparison lookup (C++14)?
  xbt_dict_t /* <hostname, nullptr> */ hostnames_;
  // This is the parent snapshot of the current state:
  PageStore page_store_;
  std::unique_ptr<Process> process_;
public:
  simgrid::mc::Snapshot* parent_snapshot_;

public:
  ModelChecker(ModelChecker const&) = delete;
  ModelChecker& operator=(ModelChecker const&) = delete;
  ModelChecker(std::unique_ptr<Process> process);
  ~ModelChecker();

  Process& process()
  {
    return *process_;
  }
  PageStore& page_store()
  {
    return page_store_;
  }
  const char* get_host_name(const char* name);

  bool is_important_snapshot(Snapshot const& snapshot) const
  {
    return &snapshot == this->parent_snapshot_;
  }

  void start();
  void shutdown();
  void resume(simgrid::mc::Process& process);
  void loop();
  bool handle_events();
  void wait_client(simgrid::mc::Process& process);
  void simcall_handle(simgrid::mc::Process& process, unsigned long pid, int value);
  void wait_for_requests()
  {
    mc_model_checker->wait_client(mc_model_checker->process());
  }
  void exit(int status);

  bool checkDeadlock();

private:
  void setup_ignore();
  bool handle_message(char* buffer, ssize_t size);
  void handle_signals();
  void handle_waitpid();
  void on_signal(const struct signalfd_siginfo* info);

};

}
}

#endif
