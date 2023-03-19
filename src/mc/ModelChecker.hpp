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
  std::unique_ptr<RemoteProcessMemory> remote_process_memory_;

public:
  ModelChecker(ModelChecker const&) = delete;
  ModelChecker& operator=(ModelChecker const&) = delete;
  explicit ModelChecker(std::unique_ptr<RemoteProcessMemory> remote_simulation);

  RemoteProcessMemory& get_remote_process_memory() { return *remote_process_memory_; }

  bool handle_message(const char* buffer, ssize_t size); // FIXME move to RemoteApp
};

} // namespace simgrid::mc

#endif
