/* Copyright (c) 2007-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc/mc.h"
#include "smx_private.hpp"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/activity/ExecImpl.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/plugins/vm/VirtualMachineImpl.hpp"
#include "src/simix/smx_host_private.hpp"
#include "xbt/ex.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_host, simix, "SIMIX hosts");

/* needs to be public and without simcall for exceptions and logging events */
const char* sg_host_self_get_name()
{
  sg_host_t host = sg_host_self();
  if (host == nullptr || SIMIX_process_self() == simix_global->maestro_process)
    return "";

  return host->get_cname();
}

/**
 * @brief Add a process to the list of the processes that the host will restart when it comes back
 * This function add a process to the list of the processes that will be restarted when the host comes
 * back. It is expected that this function is called when the host is down.
 * The processes will only be restarted once, meaning that you will have to register the process
 * again to restart the process again.
 */
void SIMIX_host_add_auto_restart_process(sg_host_t host, simgrid::kernel::actor::ActorImpl* actor)
{
  simgrid::kernel::actor::ProcessArg* arg = new simgrid::kernel::actor::ProcessArg(host, actor);

  if (host->is_off() && watched_hosts.find(host->get_cname()) == watched_hosts.end()) {
    watched_hosts.insert(host->get_cname());
    XBT_DEBUG("Push host %s to watched_hosts because state == SURF_RESOURCE_OFF", host->get_cname());
  }
  XBT_DEBUG("Adding Process %s to the auto-restart list of Host %s", arg->name.c_str(), arg->host->get_cname());
  host->pimpl_->auto_restart_processes_.push_back(arg);
}

/** @brief Restart the list of processes that have been registered to the host */
void SIMIX_host_autorestart(sg_host_t host)
{
  std::vector<simgrid::kernel::actor::ProcessArg*> process_list = host->pimpl_->auto_restart_processes_;

  for (auto const& arg : process_list) {
    XBT_DEBUG("Restarting Process %s@%s right now", arg->name.c_str(), arg->host->get_cname());
    smx_actor_t actor = simix_global->create_process_function(arg->name.c_str(), arg->code, nullptr, arg->host,
                                                              arg->properties.get(), nullptr);
    if (arg->kill_time >= 0)
      simcall_process_set_kill_time(actor, arg->kill_time);
    if (arg->auto_restart)
      actor->auto_restart_ = arg->auto_restart;
  }
  process_list.clear();
}

boost::intrusive_ptr<simgrid::kernel::activity::ExecImpl>
SIMIX_execution_start(std::string name, double flops_amount, double priority, double bound, sg_host_t host)
{
  /* set surf's action */
  simgrid::kernel::resource::Action* surf_action = nullptr;
  if (not MC_is_active() && not MC_record_replay_is_active()) {
    surf_action = host->pimpl_cpu->execution_start(flops_amount);
    surf_action->set_priority(priority);
    if (bound > 0)
      static_cast<simgrid::surf::CpuAction*>(surf_action)->set_bound(bound);
  }

  simgrid::kernel::activity::ExecImplPtr exec = simgrid::kernel::activity::ExecImplPtr(
      new simgrid::kernel::activity::ExecImpl(name, surf_action, /*timeout_detector*/ nullptr, host));

  XBT_DEBUG("Create execute synchro %p: %s", exec.get(), exec->name_.c_str());
  simgrid::kernel::activity::ExecImpl::on_creation(exec);

  return exec;
}

boost::intrusive_ptr<simgrid::kernel::activity::ExecImpl>
SIMIX_execution_parallel_start(std::string name, int host_nb, sg_host_t* host_list, double* flops_amount,
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
        SMX_EXCEPTION(simcall->issuer, host_error, 0, "Host failed");
        break;

      case SIMIX_CANCELED:
        XBT_DEBUG("SIMIX_execution_finished: execution canceled");
        SMX_EXCEPTION(simcall->issuer, cancel_error, 0, "Canceled");
        break;

      case SIMIX_TIMEOUT:
        XBT_DEBUG("SIMIX_execution_finished: execution timeouted");
        SMX_EXCEPTION(simcall->issuer, timeout_error, 0, "Timeouted");
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
