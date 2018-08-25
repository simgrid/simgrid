/* Copyright (c) 2007-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc/mc.h"
#include "simgrid/Exception.hpp"
#include "smx_private.hpp"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/activity/ExecImpl.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/plugins/vm/VirtualMachineImpl.hpp"
#include "src/simix/smx_host_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_host, simix, "SIMIX hosts");

/* needs to be public and without simcall for exceptions and logging events */
const char* sg_host_self_get_name()
{
  sg_host_t host = sg_host_self();
  if (host == nullptr || SIMIX_process_self() == simix_global->maestro_process)
    return "";

  return host->get_cname();
}

simgrid::kernel::activity::ExecImplPtr SIMIX_execution_start(std::string name, std::string category,
                                                             double flops_amount, double priority, double bound,
                                                             sg_host_t host)
{
  /* set surf's action */
  simgrid::kernel::resource::Action* surf_action = nullptr;
  if (not MC_is_active() && not MC_record_replay_is_active()) {
    surf_action = host->pimpl_cpu->execution_start(flops_amount);
    surf_action->set_priority(priority);
    if (bound > 0)
      surf_action->set_bound(bound);
  }

  simgrid::kernel::activity::ExecImplPtr exec = simgrid::kernel::activity::ExecImplPtr(
      new simgrid::kernel::activity::ExecImpl(name, surf_action, /*timeout_detector*/ nullptr, host));

  exec->set_category(category);
  XBT_DEBUG("Create execute synchro %p: %s", exec.get(), exec->name_.c_str());
  simgrid::kernel::activity::ExecImpl::on_creation(exec);

  return exec;
}

simgrid::kernel::activity::ExecImplPtr SIMIX_execution_parallel_start(std::string name, int host_nb,
                                                                      sg_host_t* host_list, double* flops_amount,
                                                                      double* bytes_amount, double rate, double timeout)
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
    sg_host_t* host_list_cpy = new sg_host_t[host_nb];
    std::copy_n(host_list, host_nb, host_list_cpy);
    surf_action = surf_host_model->execute_parallel(host_nb, host_list_cpy, flops_amount, bytes_amount, rate);
    if (timeout > 0) {
      timeout_detector = host_list[0]->pimpl_cpu->sleep(timeout);
    }
  }

  simgrid::kernel::activity::ExecImplPtr exec = simgrid::kernel::activity::ExecImplPtr(
      new simgrid::kernel::activity::ExecImpl(name, surf_action, timeout_detector, nullptr));

  XBT_DEBUG("Create parallel execute synchro %p", exec.get());

  return exec;
}

void simcall_HANDLER_execution_wait(smx_simcall_t simcall, smx_activity_t synchro)
{
  XBT_DEBUG("Wait for execution of synchro %p, state %d", synchro.get(), (int)synchro->state_);

  /* Associate this simcall to the synchro */
  synchro->simcalls_.push_back(simcall);
  simcall->issuer->waiting_synchro = synchro;

  /* set surf's synchro */
  if (MC_is_active() || MC_record_replay_is_active()) {
    synchro->state_ = SIMIX_DONE;
    SIMIX_execution_finish(synchro);
    return;
  }

  /* If the synchro is already finished then perform the error handling */
  if (synchro->state_ != SIMIX_RUNNING)
    SIMIX_execution_finish(synchro);
}

void simcall_HANDLER_execution_test(smx_simcall_t simcall, smx_activity_t synchro)
{
  int res = (synchro->state_ != SIMIX_WAITING && synchro->state_ != SIMIX_RUNNING);
  if (res) {
    synchro->simcalls_.push_back(simcall);
    SIMIX_execution_finish(synchro);
  } else {
    SIMIX_simcall_answer(simcall);
  }
  simcall_execution_test__set__result(simcall, res);
}

void SIMIX_execution_finish(smx_activity_t synchro)
{
  simgrid::kernel::activity::ExecImplPtr exec =
      boost::static_pointer_cast<simgrid::kernel::activity::ExecImpl>(synchro);

  while (not synchro->simcalls_.empty()) {
    smx_simcall_t simcall = synchro->simcalls_.front();
    synchro->simcalls_.pop_front();
    switch (exec->state_) {

      case SIMIX_DONE:
        /* do nothing, synchro done */
        XBT_DEBUG("SIMIX_execution_finished: execution successful");
        break;

      case SIMIX_FAILED:
        XBT_DEBUG("SIMIX_execution_finished: host '%s' failed", simcall->issuer->host_->get_cname());
        simcall->issuer->context_->iwannadie = 1;
        simcall->issuer->exception =
            std::make_exception_ptr(simgrid::HostFailureException(XBT_THROW_POINT, "Host failed"));
        break;

      case SIMIX_CANCELED:
        XBT_DEBUG("SIMIX_execution_finished: execution canceled");
        SMX_EXCEPTION(simcall->issuer, cancel_error, 0, "Canceled");
        break;

      case SIMIX_TIMEOUT:
        XBT_DEBUG("SIMIX_execution_finished: execution timeouted");
        simcall->issuer->exception = std::make_exception_ptr(simgrid::TimeoutError(XBT_THROW_POINT, "Timeouted"));
        break;

      default:
        xbt_die("Internal error in SIMIX_execution_finish: unexpected synchro state %d", (int)exec->state_);
    }
    /* Fail the process if the host is down */
    if (simcall->issuer->host_->is_off())
      simcall->issuer->context_->iwannadie = 1;

    simcall->issuer->waiting_synchro = nullptr;
    simcall_execution_wait__set__result(simcall, exec->state_);
    SIMIX_simcall_answer(simcall);
  }
}

void SIMIX_set_category(smx_activity_t synchro, std::string category)
{
  if (synchro->state_ != SIMIX_RUNNING)
    return;

  simgrid::kernel::activity::ExecImplPtr exec =
      boost::dynamic_pointer_cast<simgrid::kernel::activity::ExecImpl>(synchro);
  if (exec != nullptr) {
    exec->surf_action_->set_category(category);
    return;
  }

  simgrid::kernel::activity::CommImplPtr comm =
      boost::dynamic_pointer_cast<simgrid::kernel::activity::CommImpl>(synchro);
  if (comm != nullptr) {
    comm->surfAction_->set_category(category);
  }
}
