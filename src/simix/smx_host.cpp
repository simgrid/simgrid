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

  /* Check that we are not mixing VMs and PMs in the parallel task */
  bool is_a_vm = (nullptr != dynamic_cast<simgrid::s4u::VirtualMachine*>(host_list[0]));
  for (int i = 1; i < host_nb; i++) {
    bool tmp_is_a_vm = (nullptr != dynamic_cast<simgrid::s4u::VirtualMachine*>(host_list[i]));
    xbt_assert(is_a_vm == tmp_is_a_vm, "parallel_execute: mixing VMs and PMs is not supported (yet).");
  }

  /* set surf's synchro */
  simgrid::kernel::resource::Action* surf_action      = nullptr;
  simgrid::kernel::resource::Action* timeout_detector = nullptr;
  if (not MC_is_active() && not MC_record_replay_is_active()) {
    surf_action = surf_host_model->execute_parallel(host_nb, host_list, flops_amount, bytes_amount, rate);
    if (timeout > 0) {
      timeout_detector = host_list[0]->pimpl_cpu->sleep(timeout);
    }
  }

  simgrid::kernel::activity::ExecImplPtr exec = simgrid::kernel::activity::ExecImplPtr(
      new simgrid::kernel::activity::ExecImpl(std::move(name), "", timeout_detector, nullptr));
  if (surf_action != nullptr) {
    exec->surf_action_ = surf_action;
    exec->surf_action_->set_data(exec.get());
  }
  XBT_DEBUG("Create parallel execute synchro %p", exec.get());

  return exec;
}

