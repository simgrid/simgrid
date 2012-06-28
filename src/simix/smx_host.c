/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smx_private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/dict.h"
#include "mc/mc.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_host, simix,
                                "Logging specific to SIMIX (hosts)");


static void SIMIX_execution_finish(smx_action_t action);

/**
 * \brief Internal function to create a SIMIX host.
 * \param name name of the host to create
 * \param workstation the SURF workstation to encapsulate
 * \param data some user data (may be NULL)
 */
smx_host_t SIMIX_host_create(const char *name,
                               void *workstation, void *data)
{
  smx_host_t smx_host = xbt_new0(s_smx_host_t, 1);
  s_smx_process_t proc;

  /* Host structure */
  smx_host->name = xbt_strdup(name);
  smx_host->data = data;
  smx_host->host = workstation;
  smx_host->process_list =
      xbt_swag_new(xbt_swag_offset(proc, host_proc_hookup));

  /* Update global variables */
  xbt_lib_set(host_lib,smx_host->name,SIMIX_HOST_LEVEL,smx_host);

  return smx_host;
}

/**
 * \brief Internal function to destroy a SIMIX host.
 *
 * \param h the host to destroy (a smx_host_t)
 */
void SIMIX_host_destroy(void *h)
{
  smx_host_t host = (smx_host_t) h;

  xbt_assert((host != NULL), "Invalid parameters");

  /* Clean Simulator data */
  if (xbt_swag_size(host->process_list) != 0) {
    char *msg =
        bprintf("Shutting down host %s, but it's not empty:", host->name);
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
  xbt_swag_free(host->process_list);

  /* Clean host structure */
  free(host->name);
  free(host);

  return;
}

/**
 * \brief Returns a dict of all hosts.
 *
 * \return List of all hosts (as a #xbt_dict_t)
 */
xbt_dict_t SIMIX_host_get_dict(void)
{
  xbt_dict_t host_dict = xbt_dict_new_homogeneous(NULL);
  xbt_lib_cursor_t cursor = NULL;
  char *name = NULL;
  void **host = NULL;

  xbt_lib_foreach(host_lib, cursor, name, host){
    if(host[SIMIX_HOST_LEVEL])
            xbt_dict_set(host_dict,name,host[SIMIX_HOST_LEVEL], NULL);
  }
  return host_dict;
}

smx_host_t SIMIX_host_get_by_name(const char *name)
{
  xbt_assert(((simix_global != NULL)
               && (host_lib != NULL)),
              "Environment not set yet");

  return xbt_lib_get_or_null(host_lib, name, SIMIX_HOST_LEVEL);
}

smx_host_t SIMIX_host_self(void)
{
  smx_process_t process = SIMIX_process_self();
  return (process == NULL) ? NULL : SIMIX_process_get_host(process);
}

/* needs to be public and without simcall because it is called
   by exceptions and logging events */
const char* SIMIX_host_self_get_name(void)
{
  smx_host_t host = SIMIX_host_self();
  if (host == NULL || SIMIX_process_self() == simix_global->maestro_process)
    return "";

  return SIMIX_host_get_name(host);
}

const char* SIMIX_host_get_name(smx_host_t host)
{
  xbt_assert((host != NULL), "Invalid parameters");

  return host->name;
}

xbt_dict_t SIMIX_host_get_properties(smx_host_t host)
{
  xbt_assert((host != NULL), "Invalid parameters (simix host is NULL)");

  return surf_workstation_model->extension.workstation.get_properties(host->host);
}

double SIMIX_host_get_speed(smx_host_t host)
{
  xbt_assert((host != NULL), "Invalid parameters (simix host is NULL)");

  return surf_workstation_model->extension.workstation.
      get_speed(host->host, 1.0);
}

double SIMIX_host_get_available_speed(smx_host_t host)
{
  xbt_assert((host != NULL), "Invalid parameters (simix host is NULL)");

  return surf_workstation_model->extension.workstation.
      get_available_speed(host->host);
}

int SIMIX_host_get_state(smx_host_t host)
{
  xbt_assert((host != NULL), "Invalid parameters (simix host is NULL)");

  return surf_workstation_model->extension.workstation.
      get_state(host->host);
}

void* SIMIX_host_self_get_data(void)
{
  return SIMIX_host_get_data(SIMIX_host_self());
}

void SIMIX_host_self_set_data(void *data)
{
  SIMIX_host_set_data(SIMIX_host_self(), data);
}

void* SIMIX_host_get_data(smx_host_t host)
{
  xbt_assert((host != NULL), "Invalid parameters (simix host is NULL)");

  return host->data;
}
void _SIMIX_host_free_process_arg(void *);
void _SIMIX_host_free_process_arg(void *data) {
  smx_process_arg_t arg = *(void**)data;
  int i;
  xbt_free(arg->name);
  for (i = 0; i < arg->argc; i++) {
    xbt_free(arg->argv[i]);
  }
  xbt_free(arg->argv);
  xbt_free(arg);
}
void SIMIX_host_add_auto_restart_process(smx_host_t host,
                                         const char *name,
                                         xbt_main_func_t code,
                                         void *data,
                                         const char *hostname,
                                         double kill_time,
                                         int argc, char **argv,
                                         xbt_dict_t properties,
                                         int auto_restart) {
  if (!host->auto_restart_processes) {
    host->auto_restart_processes = xbt_dynar_new(sizeof(smx_process_arg_t),_SIMIX_host_free_process_arg);
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

  arg->properties = properties;
  arg->auto_restart = auto_restart;

  xbt_dynar_push_as(host->auto_restart_processes,smx_process_arg_t,arg);
}


void SIMIX_host_set_data(smx_host_t host, void *data)
{
  xbt_assert((host != NULL), "Invalid parameters");
  xbt_assert((host->data == NULL), "Data already set");

  host->data = data;
}

smx_action_t SIMIX_host_execute(const char *name, smx_host_t host,
                                double computation_amount,
                                double priority)
{
  /* alloc structures and initialize */
  smx_action_t action = xbt_mallocator_get(simix_global->action_mallocator);
  action->type = SIMIX_ACTION_EXECUTE;
  action->name = xbt_strdup(name);
  action->state = SIMIX_RUNNING;
  action->execution.host = host;

#ifdef HAVE_TRACING
  action->category = NULL;
#endif

  /* set surf's action */
  if (!MC_IS_ENABLED) {
    action->execution.surf_exec =
      surf_workstation_model->extension.workstation.execute(host->host,
    computation_amount);
    surf_workstation_model->action_data_set(action->execution.surf_exec, action);
    surf_workstation_model->set_priority(action->execution.surf_exec, priority);
  }

  XBT_DEBUG("Create execute action %p", action);

  return action;
}

smx_action_t SIMIX_host_parallel_execute( const char *name,
    int host_nb, smx_host_t *host_list,
    double *computation_amount, double *communication_amount,
    double amount, double rate)
{
  void **workstation_list = NULL;
  int i;

  /* alloc structures and initialize */
  smx_action_t action = xbt_mallocator_get(simix_global->action_mallocator);
  action->type = SIMIX_ACTION_PARALLEL_EXECUTE;
  action->name = xbt_strdup(name);
  action->state = SIMIX_RUNNING;
  action->execution.host = NULL; /* FIXME: do we need the list of hosts? */

#ifdef HAVE_TRACING
  action->category = NULL;
#endif

  /* set surf's action */
  workstation_list = xbt_new0(void *, host_nb);
  for (i = 0; i < host_nb; i++)
    workstation_list[i] = host_list[i]->host;

  /* set surf's action */
  if (!MC_IS_ENABLED) {
    action->execution.surf_exec =
      surf_workstation_model->extension.workstation.
      execute_parallel_task(host_nb, workstation_list, computation_amount,
                      communication_amount, rate);

    surf_workstation_model->action_data_set(action->execution.surf_exec, action);
  }
  XBT_DEBUG("Create parallel execute action %p", action);

  return action;
}

void SIMIX_host_execution_destroy(smx_action_t action)
{
  int destroyed=0;
  XBT_DEBUG("Destroy action %p", action);


  if (action->execution.surf_exec) {
    destroyed = surf_workstation_model->action_unref(action->execution.surf_exec);
    action->execution.surf_exec = NULL;
  }

  if (destroyed) {
    xbt_free(action->name);
    xbt_mallocator_release(simix_global->action_mallocator, action);
  }
}

void SIMIX_host_execution_cancel(smx_action_t action)
{
  XBT_DEBUG("Cancel action %p", action);

  if (action->execution.surf_exec)
    surf_workstation_model->action_cancel(action->execution.surf_exec);
}

double SIMIX_host_execution_get_remains(smx_action_t action)
{
  double result = 0.0;

  if (action->state == SIMIX_RUNNING)
    result = surf_workstation_model->get_remains(action->execution.surf_exec);

  return result;
}

e_smx_state_t SIMIX_host_execution_get_state(smx_action_t action)
{
  return action->state;
}

void SIMIX_host_execution_set_priority(smx_action_t action, double priority)
{
  if(action->execution.surf_exec)
    surf_workstation_model->set_priority(action->execution.surf_exec, priority);
}

void SIMIX_pre_host_execution_wait(smx_simcall_t simcall)
{
  smx_action_t action = simcall->host_execution_wait.execution;

  XBT_DEBUG("Wait for execution of action %p, state %d", action, (int)action->state);

  /* Associate this simcall to the action */
  xbt_fifo_push(action->simcalls, simcall);
  simcall->issuer->waiting_action = action;

  /* set surf's action */
  if (MC_IS_ENABLED) {
    action->state = SIMIX_DONE;
    SIMIX_execution_finish(action);
    return;
  }

  /* If the action is already finished then perform the error handling */
  if (action->state != SIMIX_RUNNING)
    SIMIX_execution_finish(action);
}

void SIMIX_host_execution_suspend(smx_action_t action)
{
  if(action->execution.surf_exec)
    surf_workstation_model->suspend(action->execution.surf_exec);
}

void SIMIX_host_execution_resume(smx_action_t action)
{
  if(action->execution.surf_exec)
    surf_workstation_model->resume(action->execution.surf_exec);
}

void SIMIX_execution_finish(smx_action_t action)
{
  xbt_fifo_item_t item;
  smx_simcall_t simcall;

  xbt_fifo_foreach(action->simcalls, item, simcall, smx_simcall_t) {

    switch (action->state) {

      case SIMIX_DONE:
        /* do nothing, action done */
  XBT_DEBUG("SIMIX_execution_finished: execution successful");
        break;

      case SIMIX_FAILED:
        XBT_DEBUG("SIMIX_execution_finished: host '%s' failed", simcall->issuer->smx_host->name);
        simcall->issuer->context->iwannadie = 1;
        //SMX_EXCEPTION(simcall->issuer, host_error, 0, "Host failed");
        break;

      case SIMIX_CANCELED:
        XBT_DEBUG("SIMIX_execution_finished: execution canceled");
        SMX_EXCEPTION(simcall->issuer, cancel_error, 0, "Canceled");
        break;

      default:
        xbt_die("Internal error in SIMIX_execution_finish: unexpected action state %d",
            (int)action->state);
    }
    /* check if the host is down */
    if (surf_workstation_model->extension.
        workstation.get_state(simcall->issuer->smx_host->host) != SURF_RESOURCE_ON) {
      simcall->issuer->context->iwannadie = 1;
    }

    simcall->issuer->waiting_action =    NULL;
    simcall->host_execution_wait.result = action->state;
    SIMIX_simcall_answer(simcall);
  }

  /* We no longer need it */
  SIMIX_host_execution_destroy(action);
}

void SIMIX_post_host_execute(smx_action_t action)
{
  if (action->type == SIMIX_ACTION_EXECUTE && /* FIMXE: handle resource failure
                                               * for parallel tasks too */
      surf_workstation_model->extension.workstation.get_state(action->execution.host->host) == SURF_RESOURCE_OFF) {
    /* If the host running the action failed, notice it so that the asking
     * process can be killed if it runs on that host itself */
    action->state = SIMIX_FAILED;
  } else if (surf_workstation_model->action_state_get(action->execution.surf_exec) == SURF_ACTION_FAILED) {
    /* If the host running the action didn't fail, then the action was
     * canceled */
    action->state = SIMIX_CANCELED;
  } else {
    action->state = SIMIX_DONE;
  }

  if (action->execution.surf_exec) {
    surf_workstation_model->action_unref(action->execution.surf_exec);
    action->execution.surf_exec = NULL;
  }

  /* If there are simcalls associated with the action, then answer them */
  if (xbt_fifo_size(action->simcalls)) {
    SIMIX_execution_finish(action);
  }
}


#ifdef HAVE_TRACING
void SIMIX_set_category(smx_action_t action, const char *category)
{
  if (action->state != SIMIX_RUNNING) return;
  if (action->type == SIMIX_ACTION_EXECUTE){
    surf_workstation_model->set_category(action->execution.surf_exec, category);
  }else if (action->type == SIMIX_ACTION_COMMUNICATE){
    surf_workstation_model->set_category(action->comm.surf_comm, category);
  }
}
#endif

