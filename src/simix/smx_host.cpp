/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smx_private.h"
#include "xbt/sysdep.h"
#include "mc/mc.h"
#include "src/mc/mc_replay.h"
#include "src/surf/virtual_machine.hpp"
#include "src/surf/HostImpl.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_host, simix,
                                "SIMIX hosts");

static void SIMIX_execution_finish(smx_synchro_t synchro);

/**
 * \brief Internal function to create a SIMIX host.
 * \param name name of the host to create
 */
void SIMIX_host_create(sg_host_t host) // FIXME: braindead prototype. Take sg_host as parameter
{
  smx_host_priv_t smx_host = xbt_new0(s_smx_host_priv_t, 1);
  s_smx_process_t proc;

  /* Host structure */
  smx_host->process_list = xbt_swag_new(xbt_swag_offset(proc, host_proc_hookup));

  /* Update global variables */
  sg_host_simix_set(host, smx_host);
}

/**
 * \brief Start the host if it is off
 *
 */
void SIMIX_host_on(sg_host_t h)
{
  smx_host_priv_t host = sg_host_simix(h);

  xbt_assert((host != NULL), "Invalid parameters");

  if (h->isOff()) {
    simgrid::surf::HostImpl* surf_host = h->extension<simgrid::surf::HostImpl>();
    surf_host->turnOn();

    unsigned int cpt;
    smx_process_arg_t arg;
    xbt_dynar_foreach(host->boot_processes,cpt,arg) {

      char** argv = xbt_new(char*, arg->argc);
      for (int i=0; i<arg->argc; i++)
        argv[i] = xbt_strdup(arg->argv[i]);

      XBT_DEBUG("Booting Process %s(%s) right now", arg->argv[0], arg->hostname);
      if (simix_global->create_process_function) {
        simix_global->create_process_function(argv[0],
                                              arg->code,
                                              NULL,
                                              arg->hostname,
                                              arg->kill_time,
                                              arg->argc,
                                              argv,
                                              arg->properties,
                                              arg->auto_restart,
                                              NULL);
      } else {
        simcall_process_create(arg->argv[0],
                               arg->code,
                               NULL,
                               arg->hostname,
                               arg->kill_time,
                               arg->argc,
                               argv,
                               arg->properties,
                               arg->auto_restart);
      }
    }
  }
}

/**
 * \brief Stop the host if it is on
 *
 */
void SIMIX_host_off(sg_host_t h, smx_process_t issuer)
{
  smx_host_priv_t host = sg_host_simix(h);

  xbt_assert((host != NULL), "Invalid parameters");

  if (h->isOn()) {
    simgrid::surf::HostImpl* surf_host = h->extension<simgrid::surf::HostImpl>();
    surf_host->turnOff();

    /* Clean Simulator data */
    if (xbt_swag_size(host->process_list) != 0) {
      smx_process_t process = NULL;
      xbt_swag_foreach(process, host->process_list) {
        SIMIX_process_kill(process, issuer);
        XBT_DEBUG("Killing %s on %s by %s", process->name,  sg_host_get_name(process->host), issuer->name);
      }
    }
  } else {
    XBT_INFO("Host %s is already off",h->name().c_str());
  }
}

/**
 * \brief Internal function to destroy a SIMIX host.
 *
 * \param h the host to destroy (a sg_host_t)
 */
void SIMIX_host_destroy(void *h)
{
  smx_host_priv_t host = (smx_host_priv_t) h;

  xbt_assert((host != NULL), "Invalid parameters");

  /* Clean Simulator data */
  if (xbt_swag_size(host->process_list) != 0) {
    char *msg = xbt_strdup("Shutting down host, but it's not empty:");
    char *tmp;
    smx_process_t process = NULL;

    xbt_swag_foreach(process, host->process_list) {
      tmp = bprintf("%s\n\t%s", msg, process->name);
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

sg_host_t SIMIX_host_self(void)
{
  smx_process_t process = SIMIX_process_self();
  return (process == NULL) ? NULL : SIMIX_process_get_host(process);
}

/* needs to be public and without simcall because it is called
   by exceptions and logging events */
const char* SIMIX_host_self_get_name(void)
{
  sg_host_t host = SIMIX_host_self();
  if (host == NULL || SIMIX_process_self() == simix_global->maestro_process)
    return "";

  return sg_host_get_name(host);
}

void _SIMIX_host_free_process_arg(void *data)
{
  smx_process_arg_t arg = *(smx_process_arg_t*)data;
  int i;
  for (i = 0; i < arg->argc; i++)
    xbt_free(arg->argv[i]);
  xbt_free(arg->argv);
  xbt_free(arg->name);
  xbt_free(arg);
}
/**
 * \brief Add a process to the list of the processes that the host will restart when it comes back
 * This function add a process to the list of the processes that will be restarted when the host comes
 * back. It is expected that this function is called when the host is down.
 * The processes will only be restarted once, meaning that you will have to register the process
 * again to restart the process again.
 */
void SIMIX_host_add_auto_restart_process(sg_host_t host,
                                         const char *name,
                                         xbt_main_func_t code,
                                         void *data,
                                         const char *hostname,
                                         double kill_time,
                                         int argc, char **argv,
                                         xbt_dict_t properties,
                                         int auto_restart)
{
  if (!sg_host_simix(host)->auto_restart_processes) {
    sg_host_simix(host)->auto_restart_processes = xbt_dynar_new(sizeof(smx_process_arg_t),_SIMIX_host_free_process_arg);
  }
  smx_process_arg_t arg = xbt_new(s_smx_process_arg_t,1);
  arg->name = xbt_strdup(name);
  arg->code = code;
  arg->data = data;
  arg->hostname = hostname;
  arg->kill_time = kill_time;
  arg->argc = argc;

  arg->argv = xbt_new(char*,argc + 1);

  int i;
  for (i = 0; i < argc; i++) {
    arg->argv[i] = xbt_strdup(argv[i]);
  }
  arg->argv[argc] = NULL;

  arg->properties = properties;
  arg->auto_restart = auto_restart;

  if( ! sg_host_is_on(host)
      && !xbt_dict_get_or_null(watched_hosts_lib,sg_host_get_name(host))){
    xbt_dict_set(watched_hosts_lib,sg_host_get_name(host),host,NULL);
    XBT_DEBUG("Have pushed host %s to watched_hosts_lib because state == SURF_RESOURCE_OFF",sg_host_get_name(host));
  }
  xbt_dynar_push_as(sg_host_simix(host)->auto_restart_processes,smx_process_arg_t,arg);
}
/**
 * \brief Restart the list of processes that have been registered to the host
 */
void SIMIX_host_autorestart(sg_host_t host)
{
  unsigned int cpt;
  smx_process_arg_t arg;
  xbt_dynar_t process_list = sg_host_simix(host)->auto_restart_processes;
  if (!process_list)
    return;

  xbt_dynar_foreach (process_list, cpt, arg) {

    XBT_DEBUG("Restarting Process %s(%s) right now", arg->argv[0], arg->hostname);
    if (simix_global->create_process_function) {
      simix_global->create_process_function(arg->argv[0],
                                            arg->code,
                                            NULL,
                                            arg->hostname,
                                            arg->kill_time,
                                            arg->argc,
                                            arg->argv,
                                            arg->properties,
                                            arg->auto_restart,
                                            NULL);
    } else {
      simcall_process_create(arg->argv[0],
                             (xbt_main_func_t) arg->code,
                             NULL,
                             arg->hostname,
                             arg->kill_time,
                             arg->argc,
                             arg->argv,
                             arg->properties,
                             arg->auto_restart);

    }
    /* arg->argv is used by the process created above.  Hide it to
     * _SIMIX_host_free_process_arg() which is called by xbt_dynar_reset()
     * below. */
    arg->argc = 0;
    arg->argv = NULL;
  }
  xbt_dynar_reset(process_list);
}

smx_synchro_t simcall_HANDLER_execution_start(smx_simcall_t simcall,
    const char* name, double flops_amount, double priority, double bound, unsigned long affinity_mask) {
  return SIMIX_execution_start(simcall->issuer, name,flops_amount,priority,bound,affinity_mask);
}
smx_synchro_t SIMIX_execution_start(smx_process_t issuer, const char *name,
     double flops_amount, double priority, double bound, unsigned long affinity_mask){

  /* alloc structures and initialize */
  smx_synchro_t synchro = (smx_synchro_t) xbt_mallocator_get(simix_global->synchro_mallocator);
  synchro->type = SIMIX_SYNC_EXECUTE;
  synchro->name = xbt_strdup(name);
  synchro->state = SIMIX_RUNNING;
  synchro->execution.host = issuer->host;
  synchro->category = NULL;

  /* set surf's action */
  if (!MC_is_active() && !MC_record_replay_is_active()) {

    synchro->execution.surf_exec = issuer->host->pimpl_cpu->execution_start(flops_amount);
    synchro->execution.surf_exec->setData(synchro);
    synchro->execution.surf_exec->setPriority(priority);

    if (bound != 0)
      static_cast<simgrid::surf::CpuAction*>(synchro->execution.surf_exec)
        ->setBound(bound);

    if (affinity_mask != 0) {
      /* just a double check to confirm that this host is the host where this task is running. */
      xbt_assert(synchro->execution.host == issuer->host);
      static_cast<simgrid::surf::CpuAction*>(synchro->execution.surf_exec)
        ->setAffinity(issuer->host->pimpl_cpu, affinity_mask);
    }
  }

  XBT_DEBUG("Create execute synchro %p: %s", synchro, synchro->name);

  return synchro;
}

smx_synchro_t SIMIX_execution_parallel_start(const char *name,
    int host_nb, sg_host_t *host_list,
    double *flops_amount, double *bytes_amount,
    double amount, double rate){

  sg_host_t*host_list_cpy = NULL;
  int i;

  /* alloc structures and initialize */
  smx_synchro_t synchro = (smx_synchro_t) xbt_mallocator_get(simix_global->synchro_mallocator);
  synchro->type = SIMIX_SYNC_PARALLEL_EXECUTE;
  synchro->name = xbt_strdup(name);
  synchro->state = SIMIX_RUNNING;
  synchro->execution.host = NULL; /* FIXME: do we need the list of hosts? */
  synchro->category = NULL;

  /* set surf's synchro */
  host_list_cpy = xbt_new0(sg_host_t, host_nb);
  for (i = 0; i < host_nb; i++)
    host_list_cpy[i] = host_list[i];

  /* Check that we are not mixing VMs and PMs in the parallel task */
  simgrid::surf::HostImpl *host = host_list[0]->extension<simgrid::surf::HostImpl>();
  bool is_a_vm = (nullptr != dynamic_cast<simgrid::surf::VirtualMachine*>(host));
  for (i = 1; i < host_nb; i++) {
    bool tmp_is_a_vm = (nullptr != dynamic_cast<simgrid::surf::VirtualMachine*>(host_list[i]->extension<simgrid::surf::HostImpl>()));
    xbt_assert(is_a_vm == tmp_is_a_vm, "parallel_execute: mixing VMs and PMs is not supported (yet).");
  }

  /* set surf's synchro */
  if (!MC_is_active() && !MC_record_replay_is_active()) {
    synchro->execution.surf_exec =
      surf_host_model->executeParallelTask(
          host_nb, host_list_cpy, flops_amount, bytes_amount, rate);

    synchro->execution.surf_exec->setData(synchro);
  }
  XBT_DEBUG("Create parallel execute synchro %p", synchro);

  return synchro;
}

void SIMIX_execution_destroy(smx_synchro_t synchro)
{
  XBT_DEBUG("Destroy synchro %p", synchro);

  if (synchro->execution.surf_exec) {
    synchro->execution.surf_exec->unref();
    synchro->execution.surf_exec = NULL;
  }
  xbt_free(synchro->name);
  xbt_mallocator_release(simix_global->synchro_mallocator, synchro);
}

void SIMIX_execution_cancel(smx_synchro_t synchro)
{
  XBT_DEBUG("Cancel synchro %p", synchro);

  if (synchro->execution.surf_exec)
    synchro->execution.surf_exec->cancel();
}

double SIMIX_execution_get_remains(smx_synchro_t synchro)
{
  double result = 0.0;

  if (synchro->state == SIMIX_RUNNING)
    result = synchro->execution.surf_exec->getRemains();

  return result;
}

e_smx_state_t SIMIX_execution_get_state(smx_synchro_t synchro)
{
  return synchro->state;
}

void SIMIX_execution_set_priority(smx_synchro_t synchro, double priority)
{
  if(synchro->execution.surf_exec)
    synchro->execution.surf_exec->setPriority(priority);
}

void SIMIX_execution_set_bound(smx_synchro_t synchro, double bound)
{
  if(synchro->execution.surf_exec)
    static_cast<simgrid::surf::CpuAction*>(synchro->execution.surf_exec)->setBound(bound);
}

void SIMIX_execution_set_affinity(smx_synchro_t synchro, sg_host_t host, unsigned long mask)
{
  xbt_assert(synchro->type == SIMIX_SYNC_EXECUTE);

  if (synchro->execution.surf_exec) {
    /* just a double check to confirm that this host is the host where this task is running. */
    xbt_assert(synchro->execution.host == host);
    static_cast<simgrid::surf::CpuAction*>(synchro->execution.surf_exec)
      ->setAffinity(host->pimpl_cpu, mask);
  }
}

void simcall_HANDLER_execution_wait(smx_simcall_t simcall, smx_synchro_t synchro)
{

  XBT_DEBUG("Wait for execution of synchro %p, state %d", synchro, (int)synchro->state);

  /* Associate this simcall to the synchro */
  xbt_fifo_push(synchro->simcalls, simcall);
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

void SIMIX_execution_suspend(smx_synchro_t synchro)
{
  if(synchro->execution.surf_exec)
    synchro->execution.surf_exec->suspend();
}

void SIMIX_execution_resume(smx_synchro_t synchro)
{
  if(synchro->execution.surf_exec)
    synchro->execution.surf_exec->resume();
}

void SIMIX_execution_finish(smx_synchro_t synchro)
{
  xbt_fifo_item_t item;
  smx_simcall_t simcall;

  xbt_fifo_foreach(synchro->simcalls, item, simcall, smx_simcall_t) {

    switch (synchro->state) {

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
            (int)synchro->state);
    }
    /* check if the host is down */
    if (simcall->issuer->host->isOff()) {
      simcall->issuer->context->iwannadie = 1;
    }

    simcall->issuer->waiting_synchro =    NULL;
    simcall_execution_wait__set__result(simcall, synchro->state);
    SIMIX_simcall_answer(simcall);
  }

  /* We no longer need it */
  SIMIX_execution_destroy(synchro);
}


void SIMIX_post_host_execute(smx_synchro_t synchro)
{
  if (synchro->type == SIMIX_SYNC_EXECUTE && /* FIMXE: handle resource failure
                                               * for parallel tasks too */
      synchro->execution.host->isOff()) {
    /* If the host running the synchro failed, notice it so that the asking
     * process can be killed if it runs on that host itself */
    synchro->state = SIMIX_FAILED;
  } else if (synchro->execution.surf_exec->getState() == SURF_ACTION_FAILED) {
    /* If the host running the synchro didn't fail, then the synchro was
     * canceled */
    synchro->state = SIMIX_CANCELED;
  } else {
    synchro->state = SIMIX_DONE;
  }

  if (synchro->execution.surf_exec) {
    synchro->execution.surf_exec->unref();
    synchro->execution.surf_exec = NULL;
  }

  /* If there are simcalls associated with the synchro, then answer them */
  if (xbt_fifo_size(synchro->simcalls)) {
    SIMIX_execution_finish(synchro);
  }
}


void SIMIX_set_category(smx_synchro_t synchro, const char *category)
{
  if (synchro->state != SIMIX_RUNNING) return;
  if (synchro->type == SIMIX_SYNC_EXECUTE){
    synchro->execution.surf_exec->setCategory(category);
  }else if (synchro->type == SIMIX_SYNC_COMMUNICATE){
    synchro->comm.surf_comm->setCategory(category);
  }
}
