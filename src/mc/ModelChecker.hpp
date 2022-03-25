/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_MODEL_CHECKER_HPP
#define SIMGRID_MC_MODEL_CHECKER_HPP

#include "src/mc/remote/CheckerSide.hpp"
#include "src/mc/remote/RemotePtr.hpp"
#include "src/mc/sosp/PageStore.hpp"
#include "xbt/base.h"
#include "xbt/string.hpp"

#include <memory>
#include <set>

namespace simgrid {
namespace mc {

/** State of the model-checker (global variables for the model checker)
 */
class ModelChecker {
  CheckerSide checker_side_;
  /** String pool for host names */
  std::set<xbt::string, std::less<>> hostnames_;
  // This is the parent snapshot of the current state:
  PageStore page_store_{500};
  std::unique_ptr<RemoteProcess> remote_process_;
  Exploration* exploration_ = nullptr;

  unsigned long visited_states_ = 0;

  // Expect MessageType::SIMCALL_TO_STRING or MessageType::SIMCALL_DOT_LABEL
  std::string simcall_to_string(MessageType type, aid_t aid, int times_considered);

public:
  ModelChecker(ModelChecker const&) = delete;
  ModelChecker& operator=(ModelChecker const&) = delete;
  explicit ModelChecker(std::unique_ptr<RemoteProcess> remote_simulation, int sockfd);

  RemoteProcess& get_remote_process() { return *remote_process_; }
  Channel& channel() { return checker_side_.get_channel(); }
  PageStore& page_store() { return page_store_; }

  xbt::string const& get_host_name(const char* hostname)
  {
    return *this->hostnames_.insert(xbt::string(hostname)).first;
  }

  void start();
  void shutdown();
  void resume();
  void wait_for_requests();

  /** Let the application take a transition. A new Transition is created iff the last parameter is true */
  Transition* handle_simcall(aid_t aid, int times_considered, bool new_transition);

  /* Interactions with the simcall observer */
  XBT_ATTRIB_NORETURN void exit(int status);

  void finalize_app(bool terminate_asap = false);

  Exploration* get_exploration() const { return exploration_; }
  void set_exploration(Exploration* exploration) { exploration_ = exploration; }

  unsigned long get_visited_states() const { return visited_states_; }
  void inc_visited_states() { visited_states_++; }

private:
  void setup_ignore();
  bool handle_message(const char* buffer, ssize_t size);
  void handle_waitpid();
};

}
}

#endif
