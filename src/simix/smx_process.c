/* Copyright (c) 2007-2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smx_private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/dict.h"
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
 * \brief Returns whether a process has pending asynchronous communications.
 * \return true if there are asynchronous communications in this process
 */
int SIMIX_process_has_pending_comms(smx_process_t process) {

  return xbt_fifo_size(process->comms) > 0;
}

/**
 * \brief Moves a process to the list of processes to destroy.
 */
void SIMIX_process_cleanup(smx_process_t process)
{
  XBT_DEBUG("Cleanup process %s (%p), waiting action %p",
      process->name, process, process->waiting_action);

  /* cancel non-blocking communications */
  smx_action_t action;
  while ((action = xbt_fifo_pop(process->comms))) {

    /* make sure no one will finish the comm after this process is destroyed,
     * because src_proc or dst_proc would be an invalid pointer */
    SIMIX_comm_cancel(action);

    if (action->comm.src_proc == process) {
      XBT_DEBUG("Found an unfinished send comm %p (detached = %d), state %d, src = %p, dst = %p",
          action, action->comm.detached, (int)action->state, action->comm.src_proc, action->comm.dst_proc);
      action->comm.src_proc = NULL;

      if (action->comm.detached) {
         if (action->comm.refcount == 0) {
           XBT_DEBUG("Increase the refcount before destroying it since it's detached");
           /* I'm not supposed to destroy a detached comm from the sender side,
            * unless there is no receiver matching the rdv */
           action->comm.refcount++;
           SIMIX_comm_destroy(action);
         }
         else {
           XBT_DEBUG("Don't destroy it since its refcount is %d", action->comm.refcount);
         }
      } else {
        SIMIX_comm_destroy(action);
      }
    }
    else if (action->comm.dst_proc == process){
      XBT_DEBUG("Found an unfinished recv comm %p, state %d, src = %p, dst = %p",
          action, (int)action->state, action->comm.src_proc, action->comm.dst_proc);
      action->comm.dst_proc = NULL;

      if (action->comm.detached && action->comm.refcount == 1
          && action->comm.src_proc != NULL) {
        /* the comm will be freed right now, remove it from the sender */
        xbt_fifo_remove(action->comm.src_proc->comms, action);
      }
      SIMIX_comm_destroy(action);
    }
    else {
      xbt_die("Communication action %p is in my list but I'm not the sender "
          "or the receiver", action);
    }
  }

  /*xbt_swag_remove(process, simix_global->process_to_run);*/
  xbt_swag_remove(process, simix_global->process_list);
  xbt_swag_remove(process, process->smx_host->process_list);
  xbt_swag_insert(process, simix_global->process_to_destroy);
  process->context->iwannadie = 0;
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
    free(process->running_ctx);
    xbt_dict_free(&process->properties);

    xbt_fifo_free(process->comms);

    xbt_dynar_free(&process->on_exit);

    free(process->name);
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
  maestro->simcall.issuer = maestro;

  simix_global->maestro_process = maestro;
  return;
}
/**
 * \brief Stops a process.
 *
 * Stops the process, execute all the registered on_exit functions,
 * register it to the list of the process to restart if needed
 * and stops its context.
 */
void SIMIX_process_stop(smx_process_t arg) {
  /* execute the on_exit functions */
  SIMIX_process_on_exit_runall(arg);
  /* Add the process to the list of process to restart, only if
   * the host is down
   */
  if (arg->auto_restart && !SIMIX_host_get_state(arg->smx_host)) {
    SIMIX_host_add_auto_restart_process(arg->smx_host,arg->name,arg->code, arg->data,
                                        arg->smx_host->name,
                                        arg->kill_time,
                                        arg->argc,arg->argv,arg->properties,
                                        arg->auto_restart);
  }
  XBT_DEBUG("Process %s (%s) is dead",arg->name,arg->smx_host->name);
  /* stop the context */
  SIMIX_context_stop(arg->context);
}

/**
 * \brief Same as SIMIX_process_create() but with only one argument (used by timers).
 * This function frees the argument.
 * \return the process created
 */
smx_process_t SIMIX_process_create_from_wrapper(smx_process_arg_t args) {

  smx_process_t process;
  simix_global->create_process_function(
      &process,
      args->name,
      args->code,
      args->data,
      args->hostname,
      args->kill_time,
      args->argc,
      args->argv,
      args->properties,
      args->auto_restart);
  xbt_free(args);
  return process;
}

/**
 * \brief Internal function to create a process.
 *
 * This function actually creates the process.
 * It may be called when a SIMCALL_PROCESS_CREATE simcall occurs,
 * or directly for SIMIX internal purposes. The sure thing is that it's called from maestro context.
 *
 * \return the process created
 */
void SIMIX_process_create(smx_process_t *process,
                          const char *name,
                          xbt_main_func_t code,
                          void *data,
                          const char *hostname,
                          double kill_time,
                          int argc, char **argv,
                          xbt_dict_t properties,
                          int auto_restart) {

  *process = NULL;
  smx_host_t host = SIMIX_host_get_by_name(hostname);

  XBT_DEBUG("Start process %s on host '%s'", name, hostname);

  if (!SIMIX_host_get_state(host)) {
    XBT_WARN("Cannot launch process '%s' on failed host '%s'", name,
          hostname);
  }
  else {
    *process = xbt_new0(s_smx_process_t, 1);

    xbt_assert(((code != NULL) && (host != NULL)), "Invalid parameters");
    /* Process data */
    (*process)->pid = simix_process_maxpid++;
    (*process)->name = xbt_strdup(name);
    (*process)->smx_host = host;
    (*process)->data = data;
    (*process)->comms = xbt_fifo_new();
    (*process)->simcall.issuer = *process;
    /* Process data for auto-restart */
    (*process)->auto_restart = auto_restart;
    (*process)->code = code;
    (*process)->argc = argc;
    (*process)->argv = argv;
    (*process)->kill_time = kill_time;


    XBT_VERB("Create context %s", (*process)->name);
    (*process)->context = SIMIX_context_new(code, argc, argv,
      simix_global->cleanup_process_function, *process);

    (*process)->running_ctx = xbt_new(xbt_running_ctx_t, 1);
    XBT_RUNNING_CTX_INITIALIZE((*process)->running_ctx);

    /* Add properties */
    (*process)->properties = properties;

    /* Add the process to it's host process list */
    xbt_swag_insert(*process, host->process_list);

    XBT_DEBUG("Start context '%s'", (*process)->name);

    /* Now insert it in the global process list and in the process to run list */
    xbt_swag_insert(*process, simix_global->process_list);
    XBT_DEBUG("Inserting %s(%s) in the to_run list", (*process)->name, host->name);
    xbt_dynar_push_as(simix_global->process_to_run, smx_process_t, *process);
  }

  if (kill_time > SIMIX_get_clock()) {
    if (simix_global->kill_process_function) {
      XBT_DEBUG("Process %s(%s) will be kill at time %f", (*process)->name,
          (*process)->smx_host->name, kill_time);
      SIMIX_timer_set(kill_time, simix_global->kill_process_function, *process);
    }
  }
}

/**
 * \brief Executes the processes from simix_global->process_to_run.
 *
 * The processes of simix_global->process_to_run are run (in parallel if
 * possible).  On exit, simix_global->process_to_run is empty, and
 * simix_global->process_that_ran contains the list of processes that just ran.
 * The two lists are swapped so, be careful when using them before and after a
 * call to this function.
 */
void SIMIX_process_runall(void)
{
  SIMIX_context_runall();

  xbt_dynar_t tmp = simix_global->process_that_ran;
  simix_global->process_that_ran = simix_global->process_to_run;
  simix_global->process_to_run = tmp;
  xbt_dynar_reset(simix_global->process_to_run);
}

/**
 * \brief Internal function to kill a SIMIX process.
 *
 * This function may be called when a SIMCALL_PROCESS_KILL simcall occurs,
 * or directly for SIMIX internal purposes.
 *
 * \param process poor victim
 * \param issuer the process which has sent the PROCESS_KILL. Important to not schedule twice the same process.
 */
void SIMIX_process_kill(smx_process_t process, smx_process_t issuer) {

  XBT_DEBUG("Killing process %s on %s", process->name, process->smx_host->name);

  process->context->iwannadie = 1;
  process->blocked = 0;
  process->suspended = 0;
  /* FIXME: set doexception to 0 also? */

  /* destroy the blocking action if any */
  if (process->waiting_action) {

    switch (process->waiting_action->type) {

      case SIMIX_ACTION_EXECUTE:          
      case SIMIX_ACTION_PARALLEL_EXECUTE:
        SIMIX_host_execution_destroy(process->waiting_action);
        break;

      case SIMIX_ACTION_COMMUNICATE:
        xbt_fifo_remove(process->comms, process->waiting_action);
        SIMIX_comm_cancel(process->waiting_action);
        break;

        case SIMIX_ACTION_SLEEP:
          SIMIX_process_sleep_destroy(process->waiting_action);
          break;

        case SIMIX_ACTION_SYNCHRO:
          SIMIX_synchro_stop_waiting(process, &process->simcall);
          SIMIX_synchro_destroy(process->waiting_action);
          break;

        case SIMIX_ACTION_IO:
          SIMIX_io_destroy(process->waiting_action);
          break;

        /* **************************************/
        /* TUTORIAL: New API                    */
        case SIMIX_ACTION_NEW_API:
          SIMIX_new_api_destroy(process->waiting_action);
          break;
        /* **************************************/

    }
  }
  if(!xbt_dynar_member(simix_global->process_to_run, &(process)) && process != issuer) {
    xbt_dynar_push_as(simix_global->process_to_run, smx_process_t, process);
  }

}

/**
 * \brief Kills all running processes.
 * \param issuer this one will not be killed
 */
void SIMIX_process_killall(smx_process_t issuer)
{
  smx_process_t p = NULL;

  while ((p = xbt_swag_extract(simix_global->process_list))) {
    if (p != issuer) {
      SIMIX_process_kill(p,issuer);
    }
  }

  SIMIX_context_runall();

  SIMIX_process_empty_trash();
}

void SIMIX_process_change_host(smx_process_t process,
             smx_host_t dest)
{
  xbt_assert((process != NULL), "Invalid parameters");
  xbt_swag_remove(process, process->smx_host->process_list);
  process->smx_host = dest;
  xbt_swag_insert(process, dest->process_list);
}

void SIMIX_pre_process_change_host(smx_process_t process, smx_host_t dest)
{
  process->new_host = dest;
}

void SIMIX_pre_process_suspend(smx_simcall_t simcall)
{
  smx_process_t process = simcall->process_suspend.process;
  smx_action_t action_suspend =
      SIMIX_process_suspend(process, simcall->issuer);

  if (process != simcall->issuer) {
    SIMIX_simcall_answer(simcall);
  } else {
    xbt_fifo_push(action_suspend->simcalls, simcall);
    process->waiting_action = action_suspend;
    SIMIX_host_execution_suspend(process->waiting_action);
  }
  /* If we are suspending ourselves, then just do not finish the simcall now */
}

smx_action_t SIMIX_process_suspend(smx_process_t process, smx_process_t issuer)
{
  xbt_assert((process != NULL), "Invalid parameters");

  if (process->suspended) {
    XBT_DEBUG("Process '%s' is already suspended", process->name);
    return NULL;
  }

  process->suspended = 1;

  /* If we are suspending another process, and it is waiting on an action,
     suspend its action. */
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

        case SIMIX_ACTION_SYNCHRO:
          /* Suspension is delayed to when the process is rescheduled. */
          break;

        default:
          xbt_die("Internal error in SIMIX_process_suspend: unexpected action type %d",
              (int)process->waiting_action->type);
      }
      return NULL;
    } else {
      /* Suspension is delayed to when the process is rescheduled. */
      return NULL;
    }
  } else {
    return SIMIX_host_execute("suspend", process->smx_host, 0.0, 1.0);
  }
}

void SIMIX_process_resume(smx_process_t process, smx_process_t issuer)
{
  xbt_assert((process != NULL), "Invalid parameters");

  XBT_IN("process = %p, issuer = %p", process, issuer);

  if(process->context->iwannadie) {
    XBT_VERB("Ignoring request to suspend a process that is currently dying.");
    return;
  }

  if(!process->suspended) return;
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

        case SIMIX_ACTION_SYNCHRO:
          /* I cannot resume it now. This is delayed to when the process is rescheduled at
           * the end of the synchro. */
          break;

        default:
          xbt_die("Internal error in SIMIX_process_resume: unexpected action type %d",
              (int)process->waiting_action->type);
      }
    }
  } else XBT_WARN("Strange. Process %p is trying to resume himself.", issuer);

  XBT_OUT();
}

int SIMIX_process_get_maxpid(void) {
  return simix_process_maxpid;
}

int SIMIX_process_count(void)
{
  return xbt_swag_size(simix_global->process_list);
}

void* SIMIX_process_self_get_data(smx_process_t self)
{
  xbt_assert(self == SIMIX_process_self(), "This is not the current process");

  if (!self) {
    return NULL;
  }
  return SIMIX_process_get_data(self);
}

void SIMIX_process_self_set_data(smx_process_t self, void *data)
{
  xbt_assert(self == SIMIX_process_self(), "This is not the current process");

  SIMIX_process_set_data(self, data);
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

/* needs to be public and without simcall because it is called
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

smx_process_t SIMIX_process_get_by_name(const char* name)
{
  smx_process_t proc;

  xbt_swag_foreach(proc, simix_global->process_list)
  {
    if(!strcmp(name, proc->name))
      return proc;
  }
  return NULL;
}

int SIMIX_process_is_suspended(smx_process_t process)
{
  return process->suspended;
}

xbt_dict_t SIMIX_process_get_properties(smx_process_t process)
{
  return process->properties;
}

void SIMIX_pre_process_sleep(smx_simcall_t simcall)
{
  if (MC_is_active()) {
    MC_process_clock_add(simcall->issuer, simcall->process_sleep.duration);
    simcall->process_sleep.result = SIMIX_DONE;
    SIMIX_simcall_answer(simcall);
    return;
  }
  smx_action_t action = SIMIX_process_sleep(simcall->issuer, simcall->process_sleep.duration);
  xbt_fifo_push(action->simcalls, simcall);
  simcall->issuer->waiting_action = action;
}

smx_action_t SIMIX_process_sleep(smx_process_t process, double duration)
{
  smx_action_t action;
  smx_host_t host = process->smx_host;

  /* check if the host is active */
  if (surf_workstation_model->extension.
      workstation.get_state(host->host) != SURF_RESOURCE_ON) {
    THROWF(host_error, 0, "Host %s failed, you cannot call this function",
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
  XBT_DEBUG("Create sleep action %p", action);

  return action;
}

void SIMIX_post_process_sleep(smx_action_t action)
{
  smx_simcall_t simcall;
  e_smx_state_t state;

  while ((simcall = xbt_fifo_shift(action->simcalls))) {

    switch(surf_workstation_model->action_state_get(action->sleep.surf_sleep)){
      case SURF_ACTION_FAILED:
        simcall->issuer->context->iwannadie = 1;
        //SMX_EXCEPTION(simcall->issuer, host_error, 0, "Host failed");
        break;

      case SURF_ACTION_DONE:
        state = SIMIX_DONE;
        break;

      default:
        THROW_IMPOSSIBLE;
        break;
    }
    if (surf_workstation_model->extension.
        workstation.get_state(simcall->issuer->smx_host->host) != SURF_RESOURCE_ON) {
      simcall->issuer->context->iwannadie = 1;
    }
    simcall->process_sleep.result = state;
    simcall->issuer->waiting_action = NULL;
    SIMIX_simcall_answer(simcall);

  }
  SIMIX_process_sleep_destroy(action);
}

void SIMIX_process_sleep_destroy(smx_action_t action)
{
  XBT_DEBUG("Destroy action %p", action);
  if (action->sleep.surf_sleep)
    action->sleep.surf_sleep->model_type->action_unref(action->sleep.surf_sleep);
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
 * \brief Calling this function makes the process to yield.
 *
 * Only the current process can call this function, giving back the control to
 * maestro.
 *
 * \param self the current process
 */
void SIMIX_process_yield(smx_process_t self)
{
  XBT_DEBUG("Yield process '%s'", self->name);

  /* Go into sleep and return control to maestro */
  SIMIX_context_suspend(self->context);

  /* Ok, maestro returned control to us */
  XBT_DEBUG("Control returned to me: '%s'", self->name);

  if (self->new_host) {
    SIMIX_process_change_host(self, self->new_host);
    self->new_host = NULL;
  }

  if (self->context->iwannadie){
    XBT_DEBUG("I wanna die!");
    SIMIX_process_stop(self);
  }

  if(self->suspended) {
    xbt_assert(!self->doexception, "Gloups! This exception may be lost by subsequent calls.");
    self->suspended = 0;
    SIMIX_process_suspend(self,self);
  }

  if (self->doexception) {
    XBT_DEBUG("Wait, maestro left me an exception");
    self->doexception = 0;
    SMX_THROW();
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
  xbt_abort();
}

smx_context_t SIMIX_process_get_context(smx_process_t p) {
  return p->context;
}

void SIMIX_process_set_context(smx_process_t p,smx_context_t c) {
  p->context = c;
}

/**
 * \brief Returns the list of processes to run.
 */
xbt_dynar_t SIMIX_process_get_runnable(void)
{
  return simix_global->process_to_run;
}

/**
 * \brief Returns the process from PID.
 */
smx_process_t SIMIX_process_from_PID(int PID)
{
  smx_process_t proc;
  xbt_swag_foreach(proc, simix_global->process_list)
  {
   if(proc->pid == PID)
   return proc;
  }
  return NULL;
}

/** @brief returns a dynar containg all currently existing processes */
xbt_dynar_t SIMIX_processes_as_dynar(void) {
  smx_process_t proc;
  xbt_dynar_t res = xbt_dynar_new(sizeof(smx_process_t),NULL);
  xbt_swag_foreach(proc, simix_global->process_list) {
    xbt_dynar_push(res,&proc);
  }
  return res;
}
void SIMIX_process_on_exit_runall(smx_process_t process) {
  s_smx_process_exit_fun_t exit_fun;

  while (!xbt_dynar_is_empty(process->on_exit)) {
    exit_fun = xbt_dynar_pop_as(process->on_exit,s_smx_process_exit_fun_t);
    (exit_fun.fun)(exit_fun.arg);
  }
}
void SIMIX_process_on_exit(smx_process_t process, int_f_pvoid_t fun, void *data) {
  xbt_assert(process, "current process not found: are you in maestro context ?");

  if (!process->on_exit) {
    process->on_exit = xbt_dynar_new(sizeof(s_smx_process_exit_fun_t), NULL);
  }

  s_smx_process_exit_fun_t exit_fun = {fun, data};

  xbt_dynar_push_as(process->on_exit,s_smx_process_exit_fun_t,exit_fun);
}
/**
 * \brief Sets the auto-restart status of the process.
 * If set to 1, the process will be automatically restarted when its host
 * comes back.
 */
void SIMIX_process_auto_restart_set(smx_process_t process, int auto_restart) {
  process->auto_restart = auto_restart;
}
/**
 * \brief Restart a process.
 * Restart a process, starting it again from the beginning.
 */
smx_process_t SIMIX_process_restart(smx_process_t process, smx_process_t issuer) {
  XBT_DEBUG("Restarting process %s on %s", process->name, process->smx_host->name);
  //retrieve the arguments of the old process
  //FIXME: Factorise this with SIMIX_host_add_auto_restart_process ?
  s_smx_process_arg_t arg;
  arg.code = process->code;
  arg.hostname = process->smx_host->name;
  arg.kill_time = process->kill_time;
  arg.argc = process->argc;
  arg.data = process->data;
  int i;
  arg.argv = xbt_new(char*,process->argc + 1);
  for (i = 0; i < arg.argc; i++) {
    arg.argv[i] = xbt_strdup(process->argv[i]);
  }
  arg.argv[process->argc] = NULL;
  arg.properties = NULL;
  arg.auto_restart = process->auto_restart;
  //kill the old process
  SIMIX_process_kill(process,issuer);
  //start the new process
  smx_process_t new_process;
  if (simix_global->create_process_function) {
    simix_global->create_process_function(&new_process,
                                          arg.argv[0],
                                          arg.code,
                                          arg.data,
                                          arg.hostname,
                                          arg.kill_time,
                                          arg.argc,
                                          arg.argv,
                                          arg.properties,
                                          arg.auto_restart);
  }
  else {
    simcall_process_create(&new_process,
                                          arg.argv[0],
                                          arg.code,
                                          arg.data,
                                          arg.hostname,
                                          arg.kill_time,
                                          arg.argc,
                                          arg.argv,
                                          arg.properties,
                                          arg.auto_restart);

  }
  return new_process;
}
