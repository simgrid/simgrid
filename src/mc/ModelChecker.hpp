/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_MODEL_CHECKER_HPP
#define SIMGRID_MC_MODEL_CHECKER_HPP

#include "src/mc/remote/CheckerSide.hpp"
#include "src/mc/remote/RemotePtr.hpp"
#include "src/mc/sosp/PageStore.hpp"
#include "xbt/base.h"

#include <memory>
#include <set>

namespace simgrid::mc {

/** State of the model-checker (global variables for the model checker)
 */
class ModelChecker {
  CheckerSide checker_side_;
  /** String pool for host names */
  std::set<std::string, std::less<>> hostnames_;
  // This is the parent snapshot of the current state:
  PageStore page_store_{500};
  std::unique_ptr<RemoteProcess> remote_process_;
  Exploration* exploration_ = nullptr;

  FILE* dot_output_ = nullptr;

  unsigned long visited_states_ = 0;

public:
  ModelChecker(ModelChecker const&) = delete;
  ModelChecker& operator=(ModelChecker const&) = delete;
  explicit ModelChecker(std::unique_ptr<RemoteProcess> remote_simulation, int sockfd);

  RemoteProcess& get_remote_process() { return *remote_process_; }
  Channel& channel() { return checker_side_.get_channel(); }
  PageStore& page_store() { return page_store_; }

  std::string const& get_host_name(const char* hostname) { return *this->hostnames_.insert(hostname).first; }

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

  void dot_output(const char* fmt, ...) XBT_ATTRIB_PRINTF(2, 3);
  void dot_output_flush()
  {
    if (dot_output_ != nullptr)
      fflush(dot_output_);
  }
  void dot_output_close()
  {
    if (dot_output_ != nullptr)
      fclose(dot_output_);
  }

private:
  void setup_ignore();
  bool handle_message(const char* buffer, ssize_t size);
  void handle_waitpid();
};

} // namespace simgrid::mc

#endif
