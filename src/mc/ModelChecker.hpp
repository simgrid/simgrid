/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_MODEL_CHECKER_HPP
#define SIMGRID_MC_MODEL_CHECKER_HPP

#include <sys/types.h>

#include <simgrid_config.h>
#include <xbt/dict.h>
#include <xbt/base.h>

#include "mc_forward.hpp"
#include "mc_process.h"
#include "PageStore.hpp"
#include "mc_protocol.h"

namespace simgrid {
namespace mc {

/** State of the model-checker (global variables for the model checker)
 *
 *  Each part of the state of the model chercker represented as a global
 *  variable prevents some sharing between snapshots and must be ignored.
 *  By moving as much state as possible in this structure allocated
 *  on the model-checker heap, we avoid those issues.
 */
class ModelChecker {
  /** String pool for host names */
  // TODO, use std::unordered_set with heterogeneous comparison lookup (C++14)
  xbt_dict_t /* <hostname, NULL> */ hostnames_;
  // This is the parent snapshot of the current state:
  PageStore page_store_;
  Process process_;
public:
  mc_snapshot_t parent_snapshot_;

public:
  ModelChecker(ModelChecker const&) = delete;
  ModelChecker& operator=(ModelChecker const&) = delete;
  ModelChecker(pid_t pid, int socket);
  ~ModelChecker();
  Process& process()
  {
    return process_;
  }
  PageStore& page_store()
  {
    return page_store_;
  }
  const char* get_host_name(const char* name);
};

}
}

#endif
