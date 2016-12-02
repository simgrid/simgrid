/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc/mc.h"
#include "smx_private.h"
#include "src/mc/mc_replay.h"
#include "src/plugins/vm/VirtualMachineImpl.hpp"
#include "src/surf/HostImpl.hpp"
#include "xbt/sysdep.h"
#include <xbt/ex.hpp>

#include "src/kernel/activity/SynchroExec.hpp"
#include "src/kernel/activity/SynchroComm.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_host, simix, "SIMIX hosts");

namespace simgrid {
  namespace simix {
    simgrid::xbt::Extension<simgrid::s4u::Host, Host> Host::EXTENSION_ID;

    Host::Host()
    {
      if (!Host::EXTENSION_ID.valid())
        Host::EXTENSION_ID = s4u::Host::extension_create<simix::Host>();

      simgrid::simix::ActorImpl act;
      process_list = xbt_swag_new(xbt_swag_offset(act, host_proc_hookup));
    }

    Host::~Host()
    {
      /* Clean Simulator data */
      if (xbt_swag_size(process_list) != 0) {
        char *msg = xbt_strdup("Shutting down host, but it's not empty:");
        char *tmp;
        smx_actor_t process = nullptr;

        xbt_swag_foreach(process, process_list) {
          tmp = bprintf("%s\n\t%s", msg, process->name.c_str());
          free(msg);
          msg = tmp;
        }
        SIMIX_display_process_status();
        THROWF(arg_error, 0, "%s", msg);
      }
      for (auto arg : auto_restart_processes)
        delete arg;
      auto_restart_processes.clear();
      for (auto arg : boot_processes)
        delete arg;
      boot_processes.clear();
      xbt_swag_free(process_list);
    }

    /** Re-starts all the actors that are marked as restartable.
     *
     * Weird things will happen if you turn on an host that is already on. S4U is fool-proof, not this.
     */
    void Host::turnOn()
    {
      for (auto arg : boot_processes) {
        XBT_DEBUG("Booting Process %s(%s) right now", arg->name.c_str(), arg->host->name().c_str());
        simix_global->create_process_function(arg->name.c_str(),
            arg->code,
            nullptr,
            arg->host,
            arg->kill_time,
            arg->properties,
            arg->auto_restart,
            nullptr);
      }
    }

}} // namespaces

/** @brief Stop the host if it is on */
void SIMIX_host_off(sg_host_t h, smx_actor_t issuer)
{
  smx_host_priv_t host = sg_host_simix(h);

  xbt_assert((host != nullptr), "Invalid parameters");

  if (h->isOn()) {
    h->pimpl_cpu->turnOff();

    /* Clean Simulator data */
    if (xbt_swag_size(host->process_list) != 0) {
      smx_actor_t process = nullptr;
      xbt_swag_foreach(process, host->process_list) {
        SIMIX_process_kill(process, issuer);
        XBT_DEBUG("Killing %s@%s on behalf of %s", process->cname(), process->host->cname(), issuer->cname());
      }
    }
  } else {
    XBT_INFO("Host %s is already off", h->cname());
  }
}

sg_host_t SIMIX_host_self()
{
  smx_actor_t process = SIMIX_process_self();
  return (process == nullptr) ? nullptr : process->host;
}

/* needs to be public and without simcall for exceptions and logging events */
const char* SIMIX_host_self_get_name()
{
  sg_host_t host = SIMIX_host_self();
  if (host == nullptr || SIMIX_process_self() == simix_global->maestro_process)
    return "";

  return host->cname();
}

void _SIMIX_host_free_process_arg(void *data)
{
  smx_process_arg_t arg = *(static_cast<smx_process_arg_t*>(data));
  delete arg;
}
/**
 * \brief Add a process to the list of the processes that the host will restart when it comes back
 * This function add a process to the list of the processes that will be restarted when the host comes
 * back. It is expected that this function is called when the host is down.
 * The processes will only be restarted once, meaning that you will have to register the process
 * again to restart the process again.
 */
void SIMIX_host_add_auto_restart_process(
  sg_host_t host, const char *name, std::function<void()> code,
  void* data, double kill_time, xbt_dict_t properties, int auto_restart)
{
  smx_process_arg_t arg = new simgrid::simix::ProcessArg();
  arg->name = name;
  arg->code = std::move(code);
  arg->data = data;
  arg->host = host;
  arg->kill_time = kill_time;
  arg->properties = properties;
  arg->auto_restart = auto_restart;

  if (host->isOff() && !xbt_dict_get_or_null(watched_hosts_lib, host->cname())) {
    xbt_dict_set(watched_hosts_lib, host->cname(), host, nullptr);
    XBT_DEBUG("Push host %s to watched_hosts_lib because state == SURF_RESOURCE_OFF", host->cname());
  }
  sg_host_simix(host)->auto_restart_processes.push_back(arg);
}
/** @brief Restart the list of processes that have been registered to the host */
void SIMIX_host_autorestart(sg_host_t host)
{
  std::vector<simgrid::simix::ProcessArg*> process_list = sg_host_simix(host)->auto_restart_processes;
  if (process_list.empty())
    return;

  for (auto arg : process_list) {

    XBT_DEBUG("Restarting Process %s@%s right now", arg->name.c_str(), arg->host->cname());
    simix_global->create_process_function(arg->name.c_str(), arg->code, nullptr, arg->host, arg->kill_time,
        arg->properties, arg->auto_restart, nullptr);
  }
  process_list.clear();
}

smx_activity_t simcall_HANDLER_execution_start(smx_simcall_t simcall, const char* name, double flops_amount,
                                              double priority, double bound) {
  return SIMIX_execution_start(simcall->issuer, name,flops_amount,priority,bound);
}

smx_activity_t SIMIX_execution_start(smx_actor_t issuer, const char *name, double flops_amount, double priority,
                                    double bound){

  /* alloc structures and initialize */
  simgrid::kernel::activity::Exec *exec = new simgrid::kernel::activity::Exec(name, issuer->host);

  /* set surf's action */
  if (!MC_is_active() && !MC_record_replay_is_active()) {

    exec->surf_exec = issuer->host->pimpl_cpu->execution_start(flops_amount);
    exec->surf_exec->setData(exec);
    exec->surf_exec->setPriority(priority);

    if (bound > 0)
      static_cast<simgrid::surf::CpuAction*>(exec->surf_exec)->setBound(bound);
  }

  XBT_DEBUG("Create execute synchro %p: %s", exec, exec->name.c_str());

  return exec;
}

smx_activity_t SIMIX_execution_parallel_start(const char* name, int host_nb, sg_host_t* host_list, double* flops_amount,
                                              double* bytes_amount, double amount, double rate, double timeout)
{

  /* alloc structures and initialize */
  simgrid::kernel::activity::Exec *exec = new simgrid::kernel::activity::Exec(name, nullptr);

  /* set surf's synchro */
  sg_host_t *host_list_cpy = xbt_new0(sg_host_t, host_nb);
  for (int i = 0; i < host_nb; i++)
    host_list_cpy[i] = host_list[i];

  /* Check that we are not mixing VMs and PMs in the parallel task */
  bool is_a_vm = (nullptr != dynamic_cast<simgrid::s4u::VirtualMachine*>(host_list[0]));
  for (int i = 1; i < host_nb; i++) {
    bool tmp_is_a_vm = (nullptr != dynamic_cast<simgrid::s4u::VirtualMachine*>(host_list[i]));
    xbt_assert(is_a_vm == tmp_is_a_vm, "parallel_execute: mixing VMs and PMs is not supported (yet).");
  }

  /* set surf's synchro */
  if (!MC_is_active() && !MC_record_replay_is_active()) {
    exec->surf_exec = surf_host_model->executeParallelTask(host_nb, host_list_cpy, flops_amount, bytes_amount, rate);
    exec->surf_exec->setData(exec);
    if (timeout > 0) {
      exec->timeoutDetector = host_list[0]->pimpl_cpu->sleep(timeout);
      exec->timeoutDetector->setData(exec);
    }
  }
  XBT_DEBUG("Create parallel execute synchro %p", exec);

  return exec;
}

void SIMIX_execution_cancel(smx_activity_t synchro)
{
  XBT_DEBUG("Cancel synchro %p", synchro);
  simgrid::kernel::activity::Exec *exec = static_cast<simgrid::kernel::activity::Exec *>(synchro);

  if (exec->surf_exec)
    exec->surf_exec->cancel();
}

void SIMIX_execution_set_priority(smx_activity_t synchro, double priority)
{
  simgrid::kernel::activity::Exec *exec = static_cast<simgrid::kernel::activity::Exec *>(synchro);
  if(exec->surf_exec)
    exec->surf_exec->setPriority(priority);
}

void SIMIX_execution_set_bound(smx_activity_t synchro, double bound)
{
  simgrid::kernel::activity::Exec *exec = static_cast<simgrid::kernel::activity::Exec *>(synchro);
  if(exec->surf_exec)
    static_cast<simgrid::surf::CpuAction*>(exec->surf_exec)->setBound(bound);
}

void simcall_HANDLER_execution_wait(smx_simcall_t simcall, smx_activity_t synchro)
{
  simgrid::kernel::activity::Exec *exec = static_cast<simgrid::kernel::activity::Exec *>(synchro);
  XBT_DEBUG("Wait for execution of synchro %p, state %d", synchro, (int)synchro->state);

  /* Associate this simcall to the synchro */
  synchro->simcalls.push_back(simcall);
  simcall->issuer->waiting_synchro = synchro;

  /* set surf's synchro */
  if (MC_is_active() || MC_record_replay_is_active()) {
    synchro->state = SIMIX_DONE;
    SIMIX_execution_finish(exec);
    return;
  }

  /* If the synchro is already finished then perform the error handling */
  if (synchro->state != SIMIX_RUNNING)
    SIMIX_execution_finish(exec);
}

void SIMIX_execution_finish(simgrid::kernel::activity::Exec *exec)
{
  for (smx_simcall_t simcall : exec->simcalls) {
    switch (exec->state) {

      case SIMIX_DONE:
        /* do nothing, synchro done */
        XBT_DEBUG("SIMIX_execution_finished: execution successful");
        break;

      case SIMIX_FAILED:
        XBT_DEBUG("SIMIX_execution_finished: host '%s' failed", simcall->issuer->host->cname());
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

  /* We no longer need it */
  exec->unref();
}

void SIMIX_set_category(smx_activity_t synchro, const char *category)
{
  if (synchro->state != SIMIX_RUNNING)
    return;

  simgrid::kernel::activity::Exec *exec = dynamic_cast<simgrid::kernel::activity::Exec *>(synchro);
  if (exec != nullptr) {
    exec->surf_exec->setCategory(category);
    return;
  }

  simgrid::kernel::activity::Comm *comm = dynamic_cast<simgrid::kernel::activity::Comm *>(synchro);
  if (comm != nullptr) {
    comm->surf_comm->setCategory(category);
    return;
  }
}
