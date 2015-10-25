/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smx_private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/dict.h"
#include "mc/mc.h"
#include "src/mc/mc_replay.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_host, simix,
                                "SIMIX hosts");

static void SIMIX_execution_finish(smx_synchro_t synchro);

/**
 * \brief Internal function to create a SIMIX host.
 * \param name name of the host to create
 */
void SIMIX_host_create(const char *name) // FIXME: braindead prototype. Take sg_host as parameter
{
  sg_host_t host = xbt_lib_get_elm_or_null(host_lib, name);
  smx_host_priv_t smx_host = xbt_new0(s_smx_host_priv_t, 1);
  s_smx_process_t proc;

  /* Host structure */
  smx_host->process_list =
      xbt_swag_new(xbt_swag_offset(proc, host_proc_hookup));

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

  if (surf_host_get_state(surf_host_resource_priv(h))==SURF_RESOURCE_OFF) {
    surf_host_set_state(surf_host_resource_priv(h), SURF_RESOURCE_ON);

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

void simcall_HANDLER_host_off(smx_simcall_t simcall, sg_host_t h)
{
 SIMIX_host_off(h, simcall->issuer);
}

/**
 * \brief Stop the host if it is on
 *
 */
void SIMIX_host_off(sg_host_t h, smx_process_t issuer)
{
  smx_host_priv_t host = sg_host_simix(h);

  xbt_assert((host != NULL), "Invalid parameters");

  if (surf_host_get_state(surf_host_resource_priv(h))==SURF_RESOURCE_ON) {
	surf_host_set_state(surf_host_resource_priv(h), SURF_RESOURCE_OFF);

    /* Clean Simulator data */
    if (xbt_swag_size(host->process_list) != 0) {
      smx_process_t process = NULL;
      xbt_swag_foreach(process, host->process_list) {
        SIMIX_process_kill(process, issuer);
        XBT_DEBUG("Killing %s on %s by %s", process->name,  sg_host_name(process->host), issuer->name);
      }
    }
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

  return SIMIX_host_get_name(host);
}

xbt_dict_t SIMIX_host_get_properties(sg_host_t host){
  return surf_host_get_properties(surf_host_resource_priv(host));
}

double SIMIX_host_get_speed(sg_host_t host){
  return surf_host_get_speed(host, 1.0);
}

int SIMIX_host_get_core(sg_host_t host){
  return surf_host_get_core(host);
}

xbt_swag_t SIMIX_host_get_process_list(sg_host_t host){
  smx_host_priv_t host_priv = sg_host_simix(host);

  return host_priv->process_list;
}


double SIMIX_host_get_available_speed(sg_host_t host){
  return surf_host_get_available_speed(host);
}

double SIMIX_host_get_current_power_peak(sg_host_t host) {
	  return surf_host_get_current_power_peak(host);
}

double SIMIX_host_get_power_peak_at(sg_host_t host, int pstate_index) {
	  return surf_host_get_power_peak_at(host, pstate_index);
}

int SIMIX_host_get_nb_pstates(sg_host_t host) {
	  return surf_host_get_nb_pstates(host);
}


void SIMIX_host_set_pstate(sg_host_t host, int pstate_index) {
	  surf_host_set_pstate(host, pstate_index);
}
int SIMIX_host_get_pstate(sg_host_t host) {
	  return surf_host_get_pstate(host);
}

double SIMIX_host_get_consumed_energy(sg_host_t host) {
	  return surf_host_get_consumed_energy(host);
}
double SIMIX_host_get_wattmin_at(sg_host_t host,int pstate) {
	  return surf_host_get_wattmin_at(host,pstate);
}
double SIMIX_host_get_wattmax_at(sg_host_t host,int pstate) {
	  return surf_host_get_wattmax_at(host,pstate);
}

int SIMIX_host_get_state(sg_host_t host){
  return surf_host_get_state(surf_host_resource_priv(host));
}

void _SIMIX_host_free_process_arg(void *data)
{
  smx_process_arg_t arg = *(void**)data;
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

  if( SIMIX_host_get_state(host) == SURF_RESOURCE_OFF
      && !xbt_dict_get_or_null(watched_hosts_lib,sg_host_name(host))){
    xbt_dict_set(watched_hosts_lib,sg_host_name(host),host,NULL);
    XBT_DEBUG("Have pushed host %s to watched_hosts_lib because state == SURF_RESOURCE_OFF",sg_host_name(host));
  }
  xbt_dynar_push_as(sg_host_simix(host)->auto_restart_processes,smx_process_arg_t,arg);
}
/**
 * \brief Restart the list of processes that have been registered to the host
 */
void SIMIX_host_restart_processes(sg_host_t host)
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
                             arg->code,
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

void SIMIX_host_autorestart(sg_host_t host)
{
  if(simix_global->autorestart)
    simix_global->autorestart(host);
  else
    xbt_die("No function for simix_global->autorestart");
}
smx_synchro_t simcall_HANDLER_process_execute(smx_simcall_t simcall,
		const char* name, double flops_amount, double priority, double bound, unsigned long affinity_mask) {
	return SIMIX_process_execute(simcall->issuer, name,flops_amount,priority,bound,affinity_mask);
}
smx_synchro_t SIMIX_process_execute(smx_process_t issuer, const char *name,
     double flops_amount, double priority, double bound, unsigned long affinity_mask){

  /* alloc structures and initialize */
  smx_synchro_t synchro = xbt_mallocator_get(simix_global->synchro_mallocator);
  synchro->type = SIMIX_SYNC_EXECUTE;
  synchro->name = xbt_strdup(name);
  synchro->state = SIMIX_RUNNING;
  synchro->execution.host = issuer->host;
  synchro->category = NULL;

  /* set surf's action */
  if (!MC_is_active() && !MC_record_replay_is_active()) {

    synchro->execution.surf_exec = surf_host_execute(issuer->host, flops_amount);
    surf_action_set_data(synchro->execution.surf_exec, synchro);
    surf_action_set_priority(synchro->execution.surf_exec, priority);

    /* Note (hypervisor): for multicore, the bound value being passed to the
     * surf layer should not be zero (i.e., unlimited). It should be the
     * capacity of a CPU core. */
    if (bound == 0)
      surf_cpu_action_set_bound(synchro->execution.surf_exec, SIMIX_host_get_speed(issuer->host));
    else
      surf_cpu_action_set_bound(synchro->execution.surf_exec, bound);

    if (affinity_mask != 0) {
      /* just a double check to confirm that this host is the host where this task is running. */
      xbt_assert(synchro->execution.host == issuer->host);
      surf_cpu_action_set_affinity(synchro->execution.surf_exec, issuer->host, affinity_mask);
    }
  }

  XBT_DEBUG("Create execute synchro %p: %s", synchro, synchro->name);

  return synchro;
}

smx_synchro_t SIMIX_process_parallel_execute(const char *name,
    int host_nb, sg_host_t *host_list,
    double *flops_amount, double *bytes_amount,
    double amount, double rate){

  sg_host_t*host_list_cpy = NULL;
  int i;

  /* alloc structures and initialize */
  smx_synchro_t synchro = xbt_mallocator_get(simix_global->synchro_mallocator);
  synchro->type = SIMIX_SYNC_PARALLEL_EXECUTE;
  synchro->name = xbt_strdup(name);
  synchro->state = SIMIX_RUNNING;
  synchro->execution.host = NULL; /* FIXME: do we need the list of hosts? */
  synchro->category = NULL;

  /* set surf's synchro */
  host_list_cpy = xbt_new0(sg_host_t, host_nb);
  for (i = 0; i < host_nb; i++)
    host_list_cpy[i] = host_list[i];


  /* FIXME: what happens if host_list contains VMs and PMs. If
   * execute_parallel_task() does not change the state of the model, we can mix
   * them. */
  surf_model_t ws_model = surf_resource_model(host_list[0], SURF_HOST_LEVEL);
  for (i = 1; i < host_nb; i++) {
    surf_model_t ws_model_tmp = surf_resource_model(host_list[i], SURF_HOST_LEVEL);
    if (ws_model_tmp != ws_model) {
      XBT_CRITICAL("mixing VMs and PMs is not supported");
      DIE_IMPOSSIBLE;
    }
  }

  /* set surf's synchro */
  if (!MC_is_active() && !MC_record_replay_is_active()) {
    synchro->execution.surf_exec =
      surf_host_model_execute_parallel_task(surf_host_model,
    		  host_nb, host_list_cpy, flops_amount, bytes_amount, rate);

    surf_action_set_data(synchro->execution.surf_exec, synchro);
  }
  XBT_DEBUG("Create parallel execute synchro %p", synchro);

  return synchro;
}

void SIMIX_process_execution_destroy(smx_synchro_t synchro){
  XBT_DEBUG("Destroy synchro %p", synchro);

  if (synchro->execution.surf_exec) {
    surf_action_unref(synchro->execution.surf_exec);
    synchro->execution.surf_exec = NULL;
  }
  xbt_free(synchro->name);
  xbt_mallocator_release(simix_global->synchro_mallocator, synchro);
}

void SIMIX_process_execution_cancel(smx_synchro_t synchro){
  XBT_DEBUG("Cancel synchro %p", synchro);

  if (synchro->execution.surf_exec)
    surf_action_cancel(synchro->execution.surf_exec);
}

double SIMIX_process_execution_get_remains(smx_synchro_t synchro){
  double result = 0.0;

  if (synchro->state == SIMIX_RUNNING)
    result = surf_action_get_remains(synchro->execution.surf_exec);

  return result;
}

e_smx_state_t SIMIX_process_execution_get_state(smx_synchro_t synchro){
  return synchro->state;
}

void SIMIX_process_execution_set_priority(smx_synchro_t synchro, double priority){

  if(synchro->execution.surf_exec)
	surf_action_set_priority(synchro->execution.surf_exec, priority);
}

void SIMIX_process_execution_set_bound(smx_synchro_t synchro, double bound){

  if(synchro->execution.surf_exec)
	surf_cpu_action_set_bound(synchro->execution.surf_exec, bound);
}

void SIMIX_process_execution_set_affinity(smx_synchro_t synchro, sg_host_t host, unsigned long mask){
  xbt_assert(synchro->type == SIMIX_SYNC_EXECUTE);

  if (synchro->execution.surf_exec) {
    /* just a double check to confirm that this host is the host where this task is running. */
    xbt_assert(synchro->execution.host == host);
    surf_cpu_action_set_affinity(synchro->execution.surf_exec, host, mask);
  }
}

void simcall_HANDLER_process_execution_wait(smx_simcall_t simcall, smx_synchro_t synchro){

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

void SIMIX_host_execution_suspend(smx_synchro_t synchro)
{
  if(synchro->execution.surf_exec)
    surf_action_suspend(synchro->execution.surf_exec);
}

void SIMIX_host_execution_resume(smx_synchro_t synchro)
{
  if(synchro->execution.surf_exec)
    surf_action_resume(synchro->execution.surf_exec);
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
        XBT_DEBUG("SIMIX_execution_finished: host '%s' failed", sg_host_name(simcall->issuer->host));
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
    if (surf_host_get_state(surf_host_resource_priv(simcall->issuer->host)) != SURF_RESOURCE_ON) {
      simcall->issuer->context->iwannadie = 1;
    }

    simcall->issuer->waiting_synchro =    NULL;
    simcall_process_execution_wait__set__result(simcall, synchro->state);
    SIMIX_simcall_answer(simcall);
  }

  /* We no longer need it */
  SIMIX_process_execution_destroy(synchro);
}


void SIMIX_post_host_execute(smx_synchro_t synchro)
{
  if (synchro->type == SIMIX_SYNC_EXECUTE && /* FIMXE: handle resource failure
                                               * for parallel tasks too */
      surf_host_get_state(surf_host_resource_priv(synchro->execution.host)) == SURF_RESOURCE_OFF) {
    /* If the host running the synchro failed, notice it so that the asking
     * process can be killed if it runs on that host itself */
    synchro->state = SIMIX_FAILED;
  } else if (surf_action_get_state(synchro->execution.surf_exec) == SURF_ACTION_FAILED) {
    /* If the host running the synchro didn't fail, then the synchro was
     * canceled */
    synchro->state = SIMIX_CANCELED;
  } else {
    synchro->state = SIMIX_DONE;
  }

  if (synchro->execution.surf_exec) {
    surf_action_unref(synchro->execution.surf_exec);
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
    surf_action_set_category(synchro->execution.surf_exec, category);
  }else if (synchro->type == SIMIX_SYNC_COMMUNICATE){
    surf_action_set_category(synchro->comm.surf_comm, category);
  }
}

/**
 * \brief Function to get the parameters of the given the SIMIX host.
 *
 * \param host the host to get_phys_host (a sg_host_t)
 * \param param the parameter object space to be overwritten (a ws_params_t)
 */
void SIMIX_host_get_params(sg_host_t ind_vm, vm_params_t params)
{
  /* jump to ws_get_params(). */
  surf_host_get_params(ind_vm, params);
}

void SIMIX_host_set_params(sg_host_t ind_vm, vm_params_t params)
{
  /* jump to ws_set_params(). */
  surf_host_set_params(ind_vm, params);
}

xbt_dict_t SIMIX_host_get_mounted_storage_list(sg_host_t host){
  xbt_assert((host != NULL), "Invalid parameters (simix host is NULL)");

  return surf_host_get_mounted_storage_list(host);
}

xbt_dynar_t SIMIX_host_get_attached_storage_list(sg_host_t host){
  xbt_assert((host != NULL), "Invalid parameters (simix host is NULL)");

  return surf_host_get_attached_storage_list(host);
}
