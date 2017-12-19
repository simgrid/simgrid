/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc/mc.h"
#include "smx_private.hpp"
#include "src/kernel/activity/CommImpl.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/plugins/vm/VirtualMachineImpl.hpp"
#include "src/surf/surf_interface.hpp"
#include "xbt/ex.hpp"
#include <algorithm>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_host, simix, "SIMIX hosts");

namespace simgrid {
  namespace simix {
    simgrid::xbt::Extension<simgrid::s4u::Host, Host> Host::EXTENSION_ID;

    Host::Host()
    {
      if (not Host::EXTENSION_ID.valid())
        Host::EXTENSION_ID = s4u::Host::extension_create<simix::Host>();
    }

    Host::~Host()
    {
      /* All processes should be gone when the host is turned off (by the end of the simulation). */
      if (not process_list.empty()) {
        std::string msg     = std::string("Shutting down host, but it's not empty:");
        for (auto const& process : process_list)
          msg += "\n\t" + std::string(process.getName());

        SIMIX_display_process_status();
        THROWF(arg_error, 0, "%s", msg.c_str());
      }
      for (auto const& arg : auto_restart_processes)
        delete arg;
      auto_restart_processes.clear();
      for (auto const& arg : boot_processes)
        delete arg;
      boot_processes.clear();
    }

    /** Re-starts all the actors that are marked as restartable.
     *
     * Weird things will happen if you turn on an host that is already on. S4U is fool-proof, not this.
     */
    void Host::turnOn()
    {
      for (auto const& arg : boot_processes) {
        XBT_DEBUG("Booting Process %s(%s) right now", arg->name.c_str(), arg->host->getCname());
        smx_actor_t actor = simix_global->create_process_function(arg->name.c_str(), arg->code, nullptr, arg->host,
                                                                  arg->properties.get(), nullptr);
        if (arg->kill_time >= 0)
          simcall_process_set_kill_time(actor, arg->kill_time);
        if (arg->auto_restart)
          actor->auto_restart = arg->auto_restart;
      }
    }

}} // namespaces

/** @brief Stop the host if it is on */
void SIMIX_host_off(sg_host_t h, smx_actor_t issuer)
{
  simgrid::simix::Host* host = h->extension<simgrid::simix::Host>();

  xbt_assert((host != nullptr), "Invalid parameters");

  if (h->isOn()) {
    h->pimpl_cpu->turnOff();

    /* Clean Simulator data */
    if (not host->process_list.empty()) {
      for (auto& process : host->process_list) {
        SIMIX_process_kill(&process, issuer);
        XBT_DEBUG("Killing %s@%s on behalf of %s which turned off that host.", process.getCname(),
                  process.host->getCname(), issuer->getCname());
      }
    }
  } else {
    XBT_INFO("Host %s is already off", h->getCname());
  }
}

sg_host_t sg_host_self()
{
  smx_actor_t process = SIMIX_process_self();
  return (process == nullptr) ? nullptr : process->host;
}

/* needs to be public and without simcall for exceptions and logging events */
const char* sg_host_self_get_name()
{
  sg_host_t host = sg_host_self();
  if (host == nullptr || SIMIX_process_self() == simix_global->maestro_process)
    return "";

  return host->getCname();
}

/**
 * \brief Add a process to the list of the processes that the host will restart when it comes back
 * This function add a process to the list of the processes that will be restarted when the host comes
 * back. It is expected that this function is called when the host is down.
 * The processes will only be restarted once, meaning that you will have to register the process
 * again to restart the process again.
 */
void SIMIX_host_add_auto_restart_process(sg_host_t host, const char* name, std::function<void()> code, void* data,
                                         double kill_time, std::map<std::string, std::string>* properties,
                                         int auto_restart)
{
  smx_process_arg_t arg = new simgrid::simix::ProcessArg();
  arg->name = name;
  arg->code = std::move(code);
  arg->data = data;
  arg->host = host;
  arg->kill_time = kill_time;
  arg->properties.reset(properties, [](decltype(properties)) {});
  arg->auto_restart = auto_restart;

  if (host->isOff() && watched_hosts.find(host->getCname()) == watched_hosts.end()) {
    watched_hosts.insert(host->getCname());
    XBT_DEBUG("Push host %s to watched_hosts because state == SURF_RESOURCE_OFF", host->getCname());
  }
  host->extension<simgrid::simix::Host>()->auto_restart_processes.push_back(arg);
}
/** @brief Restart the list of processes that have been registered to the host */
void SIMIX_host_autorestart(sg_host_t host)
{
  std::vector<simgrid::simix::ProcessArg*> process_list =
      host->extension<simgrid::simix::Host>()->auto_restart_processes;

  for (auto const& arg : process_list) {
    XBT_DEBUG("Restarting Process %s@%s right now", arg->name.c_str(), arg->host->getCname());
    smx_actor_t actor = simix_global->create_process_function(arg->name.c_str(), arg->code, nullptr, arg->host,
                                                              arg->properties.get(), nullptr);
    if (arg->kill_time >= 0)
      simcall_process_set_kill_time(actor, arg->kill_time);
    if (arg->auto_restart)
      actor->auto_restart = arg->auto_restart;
  }
  process_list.clear();
}

boost::intrusive_ptr<simgrid::kernel::activity::ExecImpl>
SIMIX_execution_start(const char* name, double flops_amount, double priority, double bound, sg_host_t host)
{

  /* alloc structures and initialize */
  simgrid::kernel::activity::ExecImplPtr exec =
      simgrid::kernel::activity::ExecImplPtr(new simgrid::kernel::activity::ExecImpl(name, host));

  /* set surf's action */
  if (not MC_is_active() && not MC_record_replay_is_active()) {

    exec->surfAction_ = host->pimpl_cpu->execution_start(flops_amount);
    exec->surfAction_->setData(exec.get());
    exec->surfAction_->setSharingWeight(priority);

    if (bound > 0)
      static_cast<simgrid::surf::CpuAction*>(exec->surfAction_)->setBound(bound);
  }

  XBT_DEBUG("Create execute synchro %p: %s", exec.get(), exec->name.c_str());
  simgrid::kernel::activity::ExecImpl::onCreation(exec);

  return exec;
}

boost::intrusive_ptr<simgrid::kernel::activity::ExecImpl>
SIMIX_execution_parallel_start(const char* name, int host_nb, sg_host_t* host_list, double* flops_amount,
                               double* bytes_amount, double rate, double timeout)
{

  /* alloc structures and initialize */
  simgrid::kernel::activity::ExecImplPtr exec =
      simgrid::kernel::activity::ExecImplPtr(new simgrid::kernel::activity::ExecImpl(name, nullptr));

  /* Check that we are not mixing VMs and PMs in the parallel task */
  bool is_a_vm = (nullptr != dynamic_cast<simgrid::s4u::VirtualMachine*>(host_list[0]));
  for (int i = 1; i < host_nb; i++) {
    bool tmp_is_a_vm = (nullptr != dynamic_cast<simgrid::s4u::VirtualMachine*>(host_list[i]));
    xbt_assert(is_a_vm == tmp_is_a_vm, "parallel_execute: mixing VMs and PMs is not supported (yet).");
  }

  /* set surf's synchro */
  if (not MC_is_active() && not MC_record_replay_is_active()) {
    /* set surf's synchro */
    sg_host_t* host_list_cpy = new sg_host_t[host_nb];
    std::copy_n(host_list, host_nb, host_list_cpy);
    exec->surfAction_ = surf_host_model->executeParallelTask(host_nb, host_list_cpy, flops_amount, bytes_amount, rate);
    exec->surfAction_->setData(exec.get());
    if (timeout > 0) {
      exec->timeoutDetector = host_list[0]->pimpl_cpu->sleep(timeout);
      exec->timeoutDetector->setData(exec.get());
    }
  }
  XBT_DEBUG("Create parallel execute synchro %p", exec.get());

  return exec;
}

void simcall_HANDLER_execution_wait(smx_simcall_t simcall, smx_activity_t synchro)
{
  XBT_DEBUG("Wait for execution of synchro %p, state %d", synchro.get(), (int)synchro->state);

  /* Associate this simcall to the synchro */
  synchro->simcalls.push_back(simcall);
  simcall->issuer->waiting_synchro = synchro;

  /* set surf's synchro */
  if (MC_is_active() || MC_record_replay_is_active()) {
    synchro->state = SIMIX_DONE;
    SIMIX_execution_finish(synchro);
    return;
  }

  /* If the synchro is already finished then perform the error handling */
  if (synchro->state != SIMIX_RUNNING)
    SIMIX_execution_finish(synchro);
}

void simcall_HANDLER_execution_test(smx_simcall_t simcall, smx_activity_t synchro)
{
  simcall_execution_test__set__result(simcall, (synchro->state != SIMIX_WAITING && synchro->state != SIMIX_RUNNING));
  if (simcall_execution_test__get__result(simcall)) {
    synchro->simcalls.push_back(simcall);
    SIMIX_execution_finish(synchro);
  } else {
    SIMIX_simcall_answer(simcall);
  }
  /* If the synchro is already finished then perform the error handling */
  if (synchro->state != SIMIX_RUNNING)
    SIMIX_execution_finish(synchro);
}

void SIMIX_execution_finish(smx_activity_t synchro)
{
  simgrid::kernel::activity::ExecImplPtr exec =
      boost::static_pointer_cast<simgrid::kernel::activity::ExecImpl>(synchro);

  while (not synchro->simcalls.empty()) {
    smx_simcall_t simcall = synchro->simcalls.front();
    synchro->simcalls.pop_front();
    switch (exec->state) {

      case SIMIX_DONE:
        /* do nothing, synchro done */
        XBT_DEBUG("SIMIX_execution_finished: execution successful");
        break;

      case SIMIX_FAILED:
        XBT_DEBUG("SIMIX_execution_finished: host '%s' failed", simcall->issuer->host->getCname());
        simcall->issuer->context->iwannadie = 1;
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
        xbt_die("Internal error in SIMIX_execution_finish: unexpected synchro state %d",
            (int)exec->state);
    }
    /* Fail the process if the host is down */
    if (simcall->issuer->host->isOff())
      simcall->issuer->context->iwannadie = 1;

    simcall->issuer->waiting_synchro = nullptr;
    simcall_execution_wait__set__result(simcall, exec->state);
    SIMIX_simcall_answer(simcall);
  }
}

void SIMIX_set_category(smx_activity_t synchro, const char *category)
{
  if (synchro->state != SIMIX_RUNNING)
    return;

  simgrid::kernel::activity::ExecImplPtr exec =
      boost::dynamic_pointer_cast<simgrid::kernel::activity::ExecImpl>(synchro);
  if (exec != nullptr) {
    exec->surfAction_->setCategory(category);
    return;
  }

  simgrid::kernel::activity::CommImplPtr comm =
      boost::dynamic_pointer_cast<simgrid::kernel::activity::CommImpl>(synchro);
  if (comm != nullptr) {
    comm->surfAction_->setCategory(category);
  }
}
