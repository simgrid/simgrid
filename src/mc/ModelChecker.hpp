/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_MODEL_CHECKER_HPP
#define SIMGRID_MC_MODEL_CHECKER_HPP

#include "src/mc/remote/CheckerSide.hpp"
#include "src/mc/remote/RemotePtr.hpp"
#include "src/mc/sosp/PageStore.hpp"
#include "xbt/base.h"

#include <memory>

namespace simgrid::mc {

/** State of the model-checker (global variables for the model checker)
 */
class ModelChecker {
  CheckerSide checker_side_;
  std::unique_ptr<RemoteProcessMemory> remote_process_memory_;
  Exploration* exploration_ = nullptr;

public:
  ModelChecker(ModelChecker const&) = delete;
  ModelChecker& operator=(ModelChecker const&) = delete;
  explicit ModelChecker(std::unique_ptr<RemoteProcessMemory> remote_simulation, int sockfd);

  RemoteProcessMemory& get_remote_process_memory() { return *remote_process_memory_; }
  Channel& channel() { return checker_side_.get_channel(); }

  void start();
  void resume();
  void wait_for_requests();

  /** Let the application take a transition. A new Transition is created iff the last parameter is true */
  Transition* handle_simcall(aid_t aid, int times_considered, bool new_transition);

  /* Interactions with the simcall observer */
  XBT_ATTRIB_NORETURN void exit(int status);

  Exploration* get_exploration() const { return exploration_; }
  void set_exploration(Exploration* exploration) { exploration_ = exploration; }

private:
  void setup_ignore();
  bool handle_message(const char* buffer, ssize_t size);
  void handle_waitpid();
};

} // namespace simgrid::mc

#endif
