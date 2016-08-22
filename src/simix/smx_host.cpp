/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smx_private.h"
#include <xbt/ex.hpp>
#include "xbt/sysdep.h"
#include "mc/mc.h"
#include "src/mc/mc_replay.h"
#include "src/surf/virtual_machine.hpp"
#include "src/surf/HostImpl.hpp"

#include "src/kernel/activity/SynchroExec.hpp"
#include "src/kernel/activity/SynchroComm.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_host, simix, "SIMIX hosts");

namespace simgrid {
  namespace simix {
    Host::Host()
    {
      simgrid::simix::ActorImpl act;
      process_list = xbt_swag_new(xbt_swag_offset(act, host_proc_hookup));
    }
  }
}

/** @brief Internal function to create a SIMIX host. */
void SIMIX_host_create(sg_host_t host)
{
  sg_host_simix_set(host, new simgrid::simix::Host());
}

/** @brief Start the host if it is off */
void SIMIX_host_on(sg_host_t h)
{
  smx_host_priv_t host = sg_host_simix(h);

  xbt_assert((host != nullptr), "Invalid parameters");

  if (h->isOff()) {
    simgrid::surf::HostImpl* surf_host = h->extension<simgrid::surf::HostImpl>();
    surf_host->turnOn();

    unsigned int cpt;
    smx_process_arg_t arg;
    xbt_dynar_foreach(host->boot_processes,cpt,arg) {
      XBT_DEBUG("Booting Process %s(%s) right now",
        arg->name.c_str(), arg->hostname);
      if (simix_global->create_process_function) {
        simix_global->create_process_function(arg->name.c_str(),
                                              arg->code,
                                              nullptr,
                                              arg->hostname,
                                              arg->kill_time,
                                              arg->properties,
                                              arg->auto_restart,
                                              nullptr);
      } else {
        simcall_process_create(arg->name.c_str(),
                               arg->code,
                               nullptr,
                               arg->hostname,
                               arg->kill_time,
                               arg->properties,
                               arg->auto_restart);
      }
    }
  }
}

/** @brief Stop the host if it is on */
void SIMIX_host_off(sg_host_t h, smx_actor_t issuer)
{
  smx_host_priv_t host = sg_host_simix(h);

  xbt_assert((host != nullptr), "Invalid parameters");

  if (h->isOn()) {
    simgrid::surf::HostImpl* surf_host = h->extension<simgrid::surf::HostImpl>();
    surf_host->turnOff();

    /* Clean Simulator data */
    if (xbt_swag_size(host->process_list) != 0) {
      smx_actor_t process = nullptr;
      xbt_swag_foreach(process, host->process_list) {
        SIMIX_process_kill(process, issuer);
        XBT_DEBUG("Killing %s on %s by %s",
          process->name.c_str(),  sg_host_get_name(process->host),
          issuer->name.c_str());
      }
    }
  } else {
    XBT_INFO("Host %s is already off", h->name().c_str());
  }
}

/**
 * \brief Internal function to destroy a SIMIX host.
 *
 * \param h the host to destroy (a sg_host_t)
 */
void SIMIX_host_destroy(void *h)
{
  smx_host_priv_t host = static_cast<smx_host_priv_t>(h);

  xbt_assert((host != nullptr), "Invalid parameters");

  /* Clean Simulator data */
  if (xbt_swag_size(host->process_list) != 0) {
    char *msg = xbt_strdup("Shutting down host, but it's not empty:");
    char *tmp;
    smx_actor_t process = nullptr;

    xbt_swag_foreach(process, host->process_list) {
      tmp = bprintf("%s\n\t%s", msg, process->name.c_str());
      free(msg);
      msg = tmp;
    }
    SIMIX_display_process_status();
    THROWF(arg_error, 0, "%s", msg);
  }
  xbt_dynar_free(&host->auto_restart_processes);
  xbt_dynar_free(&host->boot_processes);
  xbt_swag_free(host->process_list);

  /* Clean host structure */
  free(host);
  return;
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

  return sg_host_get_name(host);
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
  void* data, const char *hostname, double kill_time,
  xbt_dict_t properties, int auto_restart)
{
  if (!sg_host_simix(host)->auto_restart_processes) {
    sg_host_simix(host)->auto_restart_processes = xbt_dynar_new(sizeof(smx_process_arg_t),_SIMIX_host_free_process_arg);
  }
  smx_process_arg_t arg = new simgrid::simix::ProcessArg();
  arg->name = name;
  arg->code = std::move(code);
  arg->data = data;
  arg->hostname = hostname;
  arg->kill_time = kill_time;
  arg->properties = properties;
  arg->auto_restart = auto_restart;

  if( host->isOff() && !xbt_dict_get_or_null(watched_hosts_lib,sg_host_get_name(host))){
    xbt_dict_set(watched_hosts_lib,sg_host_get_name(host),host,nullptr);
    XBT_DEBUG("Push host %s to watched_hosts_lib because state == SURF_RESOURCE_OFF",sg_host_get_name(host));
  }
  xbt_dynar_push_as(sg_host_simix(host)->auto_restart_processes,smx_process_arg_t,arg);
}
/** @brief Restart the list of processes that have been registered to the host */
void SIMIX_host_autorestart(sg_host_t host)
{
  unsigned int cpt;
  smx_process_arg_t arg;
  xbt_dynar_t process_list = sg_host_simix(host)->auto_restart_processes;
  if (!process_list)
    return;

  xbt_dynar_foreach (process_list, cpt, arg) {

    XBT_DEBUG("Restarting Process %s(%s) right now", arg->name.c_str(), arg->hostname);
    if (simix_global->create_process_function) {
      simix_global->create_process_function(arg->name.c_str(), arg->code, nullptr, arg->hostname, arg->kill_time,
                                            arg->properties, arg->auto_restart, nullptr);
    } else {
      simcall_process_create(arg->name.c_str(), arg->code, nullptr, arg->hostname, arg->kill_time, arg->properties,
                             arg->auto_restart);
    }
  }
  xbt_dynar_reset(process_list);
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

smx_activity_t SIMIX_execution_parallel_start(const char *name, int host_nb, sg_host_t *host_list, double *flops_amount,
                                             double *bytes_amount, double amount, double rate){

  /* alloc structures and initialize */
  simgrid::kernel::activity::Exec *exec = new simgrid::kernel::activity::Exec(name, nullptr);

  /* set surf's synchro */
  sg_host_t *host_list_cpy = xbt_new0(sg_host_t, host_nb);
  for (int i = 0; i < host_nb; i++)
    host_list_cpy[i] = host_list[i];

  /* Check that we are not mixing VMs and PMs in the parallel task */
  simgrid::surf::HostImpl *host = host_list[0]->extension<simgrid::surf::HostImpl>();
  bool is_a_vm = (nullptr != dynamic_cast<simgrid::surf::VirtualMachine*>(host));
  for (int i = 1; i < host_nb; i++) {
    bool tmp_is_a_vm = (nullptr != dynamic_cast<simgrid::surf::VirtualMachine*>(host_list[i]->extension<simgrid::surf::HostImpl>()));
    xbt_assert(is_a_vm == tmp_is_a_vm, "parallel_execute: mixing VMs and PMs is not supported (yet).");
  }

  /* set surf's synchro */
  if (!MC_is_active() && !MC_record_replay_is_active()) {
    exec->surf_exec = surf_host_model->executeParallelTask(host_nb, host_list_cpy, flops_amount, bytes_amount, rate);
    exec->surf_exec->setData(exec);
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
        XBT_DEBUG("SIMIX_execution_finished: host '%s' failed", sg_host_get_name(simcall->issuer->host));
        simcall->issuer->context->iwannadie = 1;
        SMX_EXCEPTION(simcall->issuer, host_error, 0, "Host failed");
        break;

      case SIMIX_CANCELED:
        XBT_DEBUG("SIMIX_execution_finished: execution canceled");
        SMX_EXCEPTION(simcall->issuer, cancel_error, 0, "Canceled");
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
