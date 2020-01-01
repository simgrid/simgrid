/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_MODEL_CHECKER_HPP
#define SIMGRID_MC_MODEL_CHECKER_HPP

#include "src/mc/sosp/PageStore.hpp"
#include "xbt/base.h"

#include <memory>
#include <set>
#include <string>

#include <event2/event.h>

namespace simgrid {
namespace mc {

/** State of the model-checker (global variables for the model checker)
 */
class ModelChecker {
  struct event_base* base_    = nullptr;
  struct event* socket_event_ = nullptr;
  struct event* signal_event_ = nullptr;
  /** String pool for host names */
  // TODO, use std::set with heterogeneous comparison lookup (C++14)?
  std::set<std::string> hostnames_;
  // This is the parent snapshot of the current state:
  PageStore page_store_{500};
  std::unique_ptr<RemoteClient> process_;
  Checker* checker_ = nullptr;
public:

  ModelChecker(ModelChecker const&) = delete;
  ModelChecker& operator=(ModelChecker const&) = delete;
  explicit ModelChecker(std::unique_ptr<RemoteClient> process);
  ~ModelChecker();

  RemoteClient& process() { return *process_; }
  PageStore& page_store()
  {
    return page_store_;
  }

  std::string const& get_host_name(std::string const& hostname)
  {
    return *this->hostnames_.insert(hostname).first;
  }

  void start();
  void shutdown();
  void resume(simgrid::mc::RemoteClient& process);
  void loop();
  void handle_events(int fd, short events);
  void wait_for_requests();
  void handle_simcall(Transition const& transition);
  XBT_ATTRIB_NORETURN void exit(int status);

  bool checkDeadlock();

  Checker* getChecker() const { return checker_; }
  void setChecker(Checker* checker) { checker_ = checker; }

private:
  void setup_ignore();
  bool handle_message(const char* buffer, ssize_t size);
  void handle_waitpid();
  void on_signal(int signo);

public:
  unsigned long visited_states = 0;
  unsigned long executed_transitions = 0;
};

}
}

#endif
