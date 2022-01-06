/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_MODEL_CHECKER_HPP
#define SIMGRID_MC_MODEL_CHECKER_HPP

#include "src/mc/remote/CheckerSide.hpp"
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
  Checker* checker_ = nullptr;

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
  void handle_simcall(Transition const& transition);

  /* Interactions with the simcall observer */
  bool simcall_is_visible(aid_t aid);
  std::string simcall_to_string(aid_t aid, int times_considered);
  std::string simcall_dot_label(aid_t aid, int times_considered);

  XBT_ATTRIB_NORETURN void exit(int status);

  bool checkDeadlock();
  void finalize_app(bool terminate_asap = false);

  Checker* getChecker() const { return checker_; }
  void setChecker(Checker* checker) { checker_ = checker; }

private:
  void setup_ignore();
  bool handle_message(const char* buffer, ssize_t size);
  void handle_waitpid();

public:
  unsigned long visited_states = 0;
  unsigned long executed_transitions = 0;
};

}
}

#endif
