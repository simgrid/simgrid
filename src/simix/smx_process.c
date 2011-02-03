/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/dict.h"
#include "msg/mailbox.h"
#include "mc/mc.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_process, simix,
                                "Logging specific to SIMIX (process)");

unsigned long simix_process_maxpid = 0;

/**
 * \brief Returns the current agent.
 *
 * This functions returns the currently running SIMIX process.
 *
 * \return The SIMIX process
 */
XBT_INLINE smx_process_t SIMIX_process_self(void)
{
  smx_context_t self_context = SIMIX_context_self();

  return self_context ? SIMIX_context_get_data(self_context) : NULL;
}

/**
 * \brief Move a process to the list of processes to destroy.
 */
void SIMIX_process_cleanup(smx_process_t process)
{
  DEBUG1("Cleanup process %s", process->name);
  /*xbt_swag_remove(process, simix_global->process_to_run);*/
  xbt_swag_remove(process, simix_global->process_list);
  xbt_swag_remove(process, process->smx_host->process_list);
  xbt_swag_insert(process, simix_global->process_to_destroy);
}

/** 
 * Garbage collection
 *
 * Should be called some time to time to free the memory allocated for processes
 * that have finished (or killed).
 */
void SIMIX_process_empty_trash(void)
{
  smx_process_t process = NULL;

  while ((process = xbt_swag_extract(simix_global->process_to_destroy))) {
    SIMIX_context_free(process->context);

    /* Free the exception allocated at creation time */
    if (process->running_ctx)
      free(process->running_ctx);
    if (process->properties)
      xbt_dict_free(&process->properties);

    free(process->name);
    process->name = NULL;
    free(process);
  }
}

/**
 * \brief Creates and runs the maestro process
 */
void SIMIX_create_maestro_process()
{
  smx_process_t maestro = NULL;

  /* Create maestro process and intilialize it */
  maestro = xbt_new0(s_smx_process_t, 1);
  maestro->pid = simix_process_maxpid++;
  maestro->name = (char *) "";
  maestro->running_ctx = xbt_new(xbt_running_ctx_t, 1);
  XBT_RUNNING_CTX_INITIALIZE(maestro->running_ctx);
  maestro->context = SIMIX_context_new(NULL, 0, NULL, NULL, maestro);
  maestro->request.issuer = maestro;

  simix_global->maestro_process = maestro;
  return;
}

/**
 * \brief Same as SIMIX_process_create() but with only one argument (used by timers).
 * \return the process created
 */
smx_process_t SIMIX_process_create_from_wrapper(smx_process_arg_t args) {

  smx_process_t process;

  if (simix_global->create_process_function) {
    simix_global->create_process_function(
        &process,
        args->name,
	args->code,
	args->data,
	args->hostname,
	args->argc,
	args->argv,
	args->properties);
  }
  else {
    SIMIX_process_create(
        &process,
        args->name,
	args->code,
	args->data,
	args->hostname,
	args->argc,
	args->argv,
	args->properties);
  }
  // FIXME: to simplify this, simix_global->create_process_function could just
  // be SIMIX_process_create() by default (and the same thing in smx_deployment.c)

  return process;
}

/**
 * \brief Internal function to create a process.
 *
 * This function actually creates the process.
 * It may be called when a REQ_PROCESS_CREATE request occurs,
 * or directly for SIMIX internal purposes.
 *
 * \return the process created
 */
void SIMIX_process_create(smx_process_t *process,
                          const char *name,
                          xbt_main_func_t code,
                          void *data,
                          const char *hostname,
                          int argc, char **argv,
                          xbt_dict_t properties) {

  *process = NULL;
  smx_host_t host = SIMIX_host_get_by_name(hostname);

  DEBUG2("Start process %s on host %s", name, hostname);

  if (!SIMIX_host_get_state(host)) {
    WARN2("Cannot launch process '%s' on failed host '%s'", name,
          hostname);
  }
  else {
    *process = xbt_new0(s_smx_process_t, 1);

    xbt_assert0(((code != NULL) && (host != NULL)), "Invalid parameters");

    /* Process data */
    (*process)->pid = simix_process_maxpid++;
    (*process)->name = xbt_strdup(name);
    (*process)->smx_host = host;
    (*process)->data = data;

    VERB1("Create context %s", (*process)->name);
    (*process)->context = SIMIX_context_new(code, argc, argv,
    	simix_global->cleanup_process_function, *process);

    (*process)->running_ctx = xbt_new(xbt_running_ctx_t, 1);
    XBT_RUNNING_CTX_INITIALIZE((*process)->running_ctx);

    /* Add properties */
    (*process)->properties = properties;

    /* Add the process to it's host process list */
    xbt_swag_insert(*process, host->process_list);

    DEBUG1("Start context '%s'", (*process)->name);

    /* Now insert it in the global process list and in the process to run list */
    xbt_swag_insert(*process, simix_global->process_list);
    DEBUG2("Inserting %s(%s) in the to_run list", (*process)->name, host->name);
    xbt_dynar_push_as(simix_global->process_to_run, smx_process_t, *process);
  }
}

/**
 * \brief Internal function to kill a SIMIX process.
 *
 * This function may be called when a REQ_PROCESS_KILL request occurs,
 * or directly for SIMIX internal purposes.
 *
 * \param process poor victim
 */
void SIMIX_process_kill(smx_process_t process, smx_process_t killer) {

  DEBUG2("Killing process %s on %s", process->name, process->smx_host->name);

  process->context->iwannadie = 1;
  process->blocked = 0;
  process->suspended = 0;
  /* FIXME: set doexception to 0 also? */

  if (process->waiting_action) {

    switch (process->waiting_action->type) {

      case SIMIX_ACTION_EXECUTE:          
      case SIMIX_ACTION_PARALLEL_EXECUTE:
        SIMIX_host_execution_destroy(process->waiting_action);
        break;

      case SIMIX_ACTION_COMMUNICATE:
        SIMIX_comm_destroy(process->waiting_action);
        break;

      case SIMIX_ACTION_SLEEP:
	SIMIX_process_sleep_destroy(process->waiting_action);
	break;

      case SIMIX_ACTION_SYNCHRO:
	SIMIX_synchro_stop_waiting(process, &process->request);
	SIMIX_synchro_destroy(process->waiting_action);
	break;

      case SIMIX_ACTION_IO:
	THROW_UNIMPLEMENTED;
	break;
    }
  }

  /* If I'm killing myself then stop, otherwise schedule the process to kill. */
  if (process == killer) {
    SIMIX_context_stop(process->context);
  }
  else {
    xbt_dynar_push_as(simix_global->process_to_run, smx_process_t, process);
  }
}

/**
 * \brief Kills all running processes.
 *
 * Only maestro can kill everyone.
 */
void SIMIX_process_killall(void)
{
  smx_process_t p = NULL;

  while ((p = xbt_swag_extract(simix_global->process_list)))
    SIMIX_process_kill(p, SIMIX_process_self());

  SIMIX_context_runall(simix_global->process_to_run);

  SIMIX_process_empty_trash();
}

void SIMIX_process_change_host(smx_process_t process,
    const char *source, const char *dest)
{
  smx_host_t h1 = NULL;
  smx_host_t h2 = NULL;
  xbt_assert0((process != NULL), "Invalid parameters");
  h1 = SIMIX_host_get_by_name(source);
  h2 = SIMIX_host_get_by_name(dest);
  process->smx_host = h2;
  xbt_swag_remove(process, h1->process_list);
  xbt_swag_insert(process, h2->process_list);
}

void SIMIX_pre_process_suspend(smx_req_t req)
{
  smx_process_t process = req->process_suspend.process;
  SIMIX_process_suspend(process, req->issuer);

  if (process != req->issuer) {
    SIMIX_request_answer(req);
  }
  /* If we are suspending ourselves, then just do not replay the request. */
}

void SIMIX_process_suspend(smx_process_t process, smx_process_t issuer)
{
  process->suspended = 1;

  /* If we are suspending another process, and it is waiting on an action,
     suspend it's action. */
  if (process != issuer) {

    if (process->waiting_action) {

      switch (process->waiting_action->type) {

        case SIMIX_ACTION_EXECUTE:
        case SIMIX_ACTION_PARALLEL_EXECUTE:
          SIMIX_host_execution_suspend(process->waiting_action);
          break;

        case SIMIX_ACTION_COMMUNICATE:
          SIMIX_comm_suspend(process->waiting_action);
          break;

        case SIMIX_ACTION_SLEEP:
          SIMIX_process_sleep_suspend(process->waiting_action);
          break;

        default:
          THROW_IMPOSSIBLE;
      }
    }
  }
}

void SIMIX_process_resume(smx_process_t process, smx_process_t issuer)
{
  xbt_assert0((process != NULL), "Invalid parameters");

  process->suspended = 0;

  /* If we are resuming another process, resume the action it was waiting for
     if any. Otherwise add it to the list of process to run in the next round. */
  if (process != issuer) {

    if (process->waiting_action) {

      switch (process->waiting_action->type) {

        case SIMIX_ACTION_EXECUTE:          
        case SIMIX_ACTION_PARALLEL_EXECUTE:
          SIMIX_host_execution_resume(process->waiting_action);
          break;

        case SIMIX_ACTION_COMMUNICATE:
          SIMIX_comm_resume(process->waiting_action);
          break;

        case SIMIX_ACTION_SLEEP:
          SIMIX_process_sleep_resume(process->waiting_action);
          break;

        default:
          THROW_IMPOSSIBLE;
      }
    }
    else {
      xbt_dynar_push_as(simix_global->process_to_run, smx_process_t, process);
    }
  }
}

int SIMIX_process_get_maxpid(void) {
  return simix_process_maxpid;
}
int SIMIX_process_count(void)
{
  return xbt_swag_size(simix_global->process_list);
}

void* SIMIX_process_self_get_data(void)
{
  smx_process_t me = SIMIX_process_self();
  if (!me) {
    return NULL;
  }
  return SIMIX_process_get_data(me);
}

void SIMIX_process_self_set_data(void *data)
{
  SIMIX_process_set_data(SIMIX_process_self(), data);
}

void* SIMIX_process_get_data(smx_process_t process)
{
  return process->data;
}

void SIMIX_process_set_data(smx_process_t process, void *data)
{
  process->data = data;
}

smx_host_t SIMIX_process_get_host(smx_process_t process)
{
  return process->smx_host;
}

/* needs to be public and without request because it is called
   by exceptions and logging events */
const char* SIMIX_process_self_get_name(void) {

  smx_process_t process = SIMIX_process_self();
  if (process == NULL || process == simix_global->maestro_process)
    return "";

  return SIMIX_process_get_name(process);
}

const char* SIMIX_process_get_name(smx_process_t process)
{
  return process->name;
}

int SIMIX_process_is_suspended(smx_process_t process)
{
  return process->suspended;
}

xbt_dict_t SIMIX_process_get_properties(smx_process_t process)
{
  return process->properties;
}

void SIMIX_pre_process_sleep(smx_req_t req)
{
  if (MC_IS_ENABLED) {
    MC_process_clock_add(req->issuer, req->process_sleep.duration);
    req->process_sleep.result = SIMIX_DONE;
    SIMIX_request_answer(req);
    return;
  }
  smx_action_t action = SIMIX_process_sleep(req->issuer, req->process_sleep.duration);
  xbt_fifo_push(action->request_list, req);
  req->issuer->waiting_action = action;
}

smx_action_t SIMIX_process_sleep(smx_process_t process, double duration)
{
  smx_action_t action;
  smx_host_t host = process->smx_host;

  /* check if the host is active */
  if (surf_workstation_model->extension.
      workstation.get_state(host->host) != SURF_RESOURCE_ON) {
    THROW1(host_error, 0, "Host %s failed, you cannot call this function",
           host->name);
  }

  action = xbt_mallocator_get(simix_global->action_mallocator);
  action->type = SIMIX_ACTION_SLEEP;
  action->name = NULL;
#ifdef HAVE_TRACING
  action->category = NULL;
#endif

  action->sleep.host = host;
  action->sleep.surf_sleep =
      surf_workstation_model->extension.workstation.sleep(host->host, duration);

  surf_workstation_model->action_data_set(action->sleep.surf_sleep, action);
  DEBUG1("Create sleep action %p", action);

  return action;
}

void SIMIX_post_process_sleep(smx_action_t action)
{
  smx_req_t req;
  e_smx_state_t state;

  while ((req = xbt_fifo_shift(action->request_list))) {

    switch(surf_workstation_model->action_state_get(action->sleep.surf_sleep)){
      case SURF_ACTION_FAILED:
        state = SIMIX_SRC_HOST_FAILURE;
        break;

      case SURF_ACTION_DONE:
        state = SIMIX_DONE;
        break;

      default:
        THROW_IMPOSSIBLE;
        break;
    }
    req->process_sleep.result = state;
    req->issuer->waiting_action = NULL;
    SIMIX_request_answer(req);
  }
  SIMIX_process_sleep_destroy(action);
}

void SIMIX_process_sleep_destroy(smx_action_t action)
{
  DEBUG1("Destroy action %p", action);
  xbt_free(action->name);
  if (action->sleep.surf_sleep)
    action->sleep.surf_sleep->model_type->action_unref(action->sleep.surf_sleep);
#ifdef HAVE_TRACING
  TRACE_smx_action_destroy(action);
#endif
  xbt_mallocator_release(simix_global->action_mallocator, action);
}

void SIMIX_process_sleep_suspend(smx_action_t action)
{
  surf_workstation_model->suspend(action->sleep.surf_sleep);
}

void SIMIX_process_sleep_resume(smx_action_t action)
{
  surf_workstation_model->resume(action->sleep.surf_sleep);
}

/** 
 * Calling this function makes the process to yield.
 * Only the processes can call this function, giving back the control to maestro
 */
void SIMIX_process_yield(void)
{
  smx_process_t self = SIMIX_process_self();

  DEBUG1("Yield process '%s'", self->name);

  /* Go into sleep and return control to maestro */
  SIMIX_context_suspend(self->context);

  /* Ok, maestro returned control to us */
  DEBUG1("Maestro returned control to me: '%s'", self->name);

  if (self->context->iwannadie){
    DEBUG0("I wanna die!");
    SIMIX_context_stop(self->context);
  }

  if (self->doexception) {
    DEBUG0("Wait, maestro left me an exception");
    self->doexception = 0;
    RETHROW;
  }
}

/* callback: context fetching */
xbt_running_ctx_t *SIMIX_process_get_running_context(void)
{
  return SIMIX_process_self()->running_ctx;
}

/* callback: termination */
void SIMIX_process_exception_terminate(xbt_ex_t * e)
{
  xbt_ex_display(e);
  abort();
}

smx_context_t SIMIX_process_get_context(smx_process_t p) {
  return p->context;
}
void SIMIX_process_set_context(smx_process_t p,smx_context_t c) {
  p->context = c;
}
