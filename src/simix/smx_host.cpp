/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc/mc.h"
#include "smx_private.hpp"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/activity/ExecImpl.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/plugins/vm/VirtualMachineImpl.hpp"
#include "src/simix/smx_host_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_host, simix, "SIMIX hosts");

simgrid::kernel::activity::ExecImplPtr
SIMIX_execution_parallel_start(std::string name, int host_nb, const sg_host_t* host_list, const double* flops_amount,
                               const double* bytes_amount, double rate, double timeout)
{
  simgrid::kernel::activity::ExecImplPtr exec = simgrid::kernel::activity::ExecImplPtr(
      new simgrid::kernel::activity::ExecImpl(std::move(name), "", host_list[0], timeout));

  /* set surf's synchro */
  if (not MC_is_active() && not MC_record_replay_is_active()) {
    exec->surf_action_ = surf_host_model->execute_parallel(host_nb, host_list, flops_amount, bytes_amount, rate);
    if (exec->surf_action_ != nullptr) {
      exec->surf_action_->set_data(exec.get());
    }
  }

  XBT_DEBUG("Create parallel execute synchro %p", exec.get());

  return exec;
}

