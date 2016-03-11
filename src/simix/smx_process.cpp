/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/surf_interface.hpp"
#include "smx_private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/dict.h"
#include "mc/mc.h"
#include "src/mc/mc_replay.h"
#include "src/mc/Client.hpp"
#include "src/simix/smx_private.hpp"
#include "src/msg/msg_private.h"

#ifdef HAVE_SMPI
#include "src/smpi/private.h"
#endif

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
smx_process_t SIMIX_process_self(void)
{
  smx_context_t self_context = SIMIX_context_self();

  return self_context ? SIMIX_context_get_process(self_context) : NULL;
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
  XBT_DEBUG("Cleanup process %s (%p), waiting synchro %p",
      process->name, process, process->waiting_synchro);

  SIMIX_process_on_exit_runall(process);

  /* Unregister from the kill timer if any */
  if (process->kill_timer != NULL)
      SIMIX_timer_remove(process->kill_timer);

  xbt_os_mutex_acquire(simix_global->mutex);

  /* cancel non-blocking communications */
  smx_synchro_t synchro;
  while ((synchro = (smx_synchro_t) xbt_fifo_pop(process->comms))) {

    /* make sure no one will finish the comm after this process is destroyed,
     * because src_proc or dst_proc would be an invalid pointer */
    SIMIX_comm_cancel(synchro);

    if (synchro->comm.src_proc == process) {
      XBT_DEBUG("Found an unfinished send comm %p (detached = %d), state %d, src = %p, dst = %p",
          synchro, synchro->comm.detached, (int)synchro->state, synchro->comm.src_proc, synchro->comm.dst_proc);
      synchro->comm.src_proc = NULL;

      /* I'm not supposed to destroy a detached comm from the sender side, */
      if (!synchro->comm.detached)
        SIMIX_comm_destroy(synchro);
      else
        XBT_DEBUG("Don't destroy it since it's a detached comm");

    }
    else if (synchro->comm.dst_proc == process){
      XBT_DEBUG("Found an unfinished recv comm %p, state %d, src = %p, dst = %p",
          synchro, (int)synchro->state, synchro->comm.src_proc, synchro->comm.dst_proc);
      synchro->comm.dst_proc = NULL;

      if (synchro->comm.detached && synchro->comm.refcount == 1
          && synchro->comm.src_proc != NULL) {
        /* the comm will be freed right now, remove it from the sender */
        xbt_fifo_remove(synchro->comm.src_proc->comms, synchro);
      }
      SIMIX_comm_destroy(synchro);
    }
    else {
      xbt_die("Communication synchro %p is in my list but I'm not the sender "
          "or the receiver", synchro);
    }
  }

  XBT_DEBUG("%p should not be run anymore",process);
  xbt_swag_remove(process, simix_global->process_list);
  if (process->host)
    xbt_swag_remove(process, sg_host_simix(process->host)->process_list);
  xbt_swag_insert(process, simix_global->process_to_destroy);
  process->context->iwannadie = 0;

  xbt_os_mutex_release(simix_global->mutex);
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

  while ((process = (smx_process_t) xbt_swag_extract(simix_global->process_to_destroy))) {
    XBT_DEBUG("Getting rid of %p",process);

    SIMIX_context_free(process->context);

    /* Free the exception allocated at creation time */
    free(process->running_ctx);
    xbt_dict_free(&process->properties);

    xbt_fifo_free(process->comms);

    xbt_dynar_free(&process->on_exit);

    xbt_free(process->name);
    xbt_free(process);
  }
}

namespace simgrid {
namespace simix {

void create_maestro(std::function<void()> code)
{
  smx_process_t maestro = NULL;
  /* Create maestro process and intilialize it */
  maestro = xbt_new0(s_smx_process_t, 1);
  maestro->pid = simix_process_maxpid++;
  maestro->ppid = -1;
  maestro->name = (char*) "";
  maestro->data = nullptr;
  maestro->running_ctx = (xbt_running_ctx_t*) xbt_malloc0(sizeof(xbt_running_ctx_t));
  XBT_RUNNING_CTX_INITIALIZE(maestro->running_ctx);

  if (!code) {
    maestro->context = SIMIX_context_new(NULL, 0, nullptr, NULL, maestro);
  } else {
    if (!simix_global)
      xbt_die("simix is not initialized, please call MSG_init first");
    maestro->context =
      simix_global->context_factory->create_maestro(code, maestro);
  }

  maestro->simcall.issuer = maestro;
  simix_global->maestro_process = maestro;
}

}
}

/**
 * \brief Creates and runs the maestro process
 */
void SIMIX_maestro_create(void (*code)(void*), void* data)
{
  simgrid::simix::create_maestro(std::bind(code, data));
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
  /* Add the process to the list of process to restart, only if the host is down */
  if (arg->auto_restart && arg->host->isOff()) {
    SIMIX_host_add_auto_restart_process(arg->host,arg->name,arg->code, arg->data,
                                        sg_host_get_name(arg->host),
                                        SIMIX_timer_get_date(arg->kill_timer),
                                        arg->argc,arg->argv,arg->properties,
                                        arg->auto_restart);
  }
  XBT_DEBUG("Process %s (%s) is dead",arg->name,sg_host_get_name(arg->host));
  /* stop the context */
  SIMIX_context_stop(arg->context);
}

/**
 * \brief Same as SIMIX_process_create() but with only one argument (used by timers).
 * This function frees the argument.
 * \return the process created
 */
smx_process_t SIMIX_process_create_from_wrapper(smx_process_arg_t args) {

  smx_process_t process = simix_global->create_process_function(
                                        args->name,
                                        args->code,
                                        args->data,
                                        args->hostname,
                                        args->kill_time,
                                        args->argc,
                                        args->argv,
                                        args->properties,
                                        args->auto_restart,
                                        NULL);
  xbt_free(args);
  return process;
}


void* simcall_HANDLER_process_create(smx_simcall_t simcall,
                          const char *name,
                          xbt_main_func_t code,
                          void *data,
                          const char *hostname,
                          double kill_time,
                          int argc, char **argv,
                          xbt_dict_t properties,
                          int auto_restart){
  return (void*)SIMIX_process_create(name, code, data, hostname,
                       kill_time, argc, argv, properties, auto_restart,
                       simcall->issuer);
}

static void kill_process(void* process)
{
  simix_global->kill_process_function((smx_process_t) process);
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
smx_process_t SIMIX_process_create(
                          const char *name,
                          xbt_main_func_t code,
                          void *data,
                          const char *hostname,
                          double kill_time,
                          int argc, char **argv,
                          xbt_dict_t properties,
                          int auto_restart,
                          smx_process_t parent_process)
{
  smx_process_t process = NULL;
  sg_host_t host = sg_host_by_name(hostname);

  XBT_DEBUG("Start process %s on host '%s'", name, hostname);

  if (host->isOff()) {
    int i;
    XBT_WARN("Cannot launch process '%s' on failed host '%s'", name,
          hostname);
    for (i = 0; i < argc; i++)
      xbt_free(argv[i]);
    xbt_free(argv);
  }
  else {
    process = xbt_new0(s_smx_process_t, 1);

    xbt_assert(((code != NULL) && (host != NULL)), "Invalid parameters");
    /* Process data */
    process->pid = simix_process_maxpid++;
    process->name = xbt_strdup(name);
    process->host = host;
    process->data = data;
    process->comms = xbt_fifo_new();
    process->simcall.issuer = process;
    /* Initiliaze data segment to default value */
    SIMIX_segment_index_set(process, -1);

     if (parent_process != NULL) {
       process->ppid = SIMIX_process_get_PID(parent_process);
       /* SMPI process have their own data segment and
          each other inherit from their father */
#if HAVE_SMPI
       if(smpi_privatize_global_variables){
         if( parent_process->pid != 0){
           SIMIX_segment_index_set(process, parent_process->segment_index);
         } else {
           SIMIX_segment_index_set(process, process->pid - 1);
         }
       }
#endif
     } else {
       process->ppid = -1;
     }

    /* Process data for auto-restart */
    process->auto_restart = auto_restart;
    process->code = code;
    process->argc = argc;
    process->argv = argv;


    XBT_VERB("Create context %s", process->name);
    process->context = SIMIX_context_new(code, argc, argv, simix_global->cleanup_process_function, process);

    process->running_ctx = (xbt_running_ctx_t*) xbt_malloc0(sizeof(xbt_running_ctx_t));
    XBT_RUNNING_CTX_INITIALIZE(process->running_ctx);

    if(MC_is_active()){
      MC_ignore_heap(process->running_ctx, sizeof(*process->running_ctx));
    }

    /* Add properties */
    process->properties = properties;

    /* Add the process to it's host process list */
    xbt_swag_insert(process, sg_host_simix(host)->process_list);

    XBT_DEBUG("Start context '%s'", process->name);

    /* Now insert it in the global process list and in the process to run list */
    xbt_swag_insert(process, simix_global->process_list);
    XBT_DEBUG("Inserting %s(%s) in the to_run list", process->name, sg_host_get_name(host));
    xbt_dynar_push_as(simix_global->process_to_run, smx_process_t, process);

    if (kill_time > SIMIX_get_clock() && simix_global->kill_process_function) {
      XBT_DEBUG("Process %s(%s) will be kill at time %f", process->name,
          sg_host_get_name(process->host), kill_time);
      process->kill_timer = SIMIX_timer_set(kill_time, kill_process, process);
    }

    /* Tracing the process creation */
    TRACE_msg_process_create(process->name, process->pid, process->host);
  }
  return process;
}

smx_process_t SIMIX_process_attach(
  const char* name,
  void *data,
  const char* hostname,
  xbt_dict_t properties,
  smx_process_t parent_process)
{
  // This is mostly a copy/paste from SIMIX_process_new(),
  // it'd be nice to share some code between those two functions.

  sg_host_t host = sg_host_by_name(hostname);
  XBT_DEBUG("Attach process %s on host '%s'", name, hostname);

  if (host->isOff()) {
    XBT_WARN("Cannot launch process '%s' on failed host '%s'",
      name, hostname);
    return nullptr;
  }

  smx_process_t process = xbt_new0(s_smx_process_t, 1);
  /* Process data */
  process->pid = simix_process_maxpid++;
  process->name = xbt_strdup(name);
  process->host = host;
  process->data = data;
  process->comms = xbt_fifo_new();
  process->simcall.issuer = process;
  process->ppid = -1;
  /* Initiliaze data segment to default value */
  SIMIX_segment_index_set(process, -1);
  if (parent_process != NULL) {
    process->ppid = SIMIX_process_get_PID(parent_process);
   /* SMPI process have their own data segment and
      each other inherit from their father */
  #if HAVE_SMPI
    if(smpi_privatize_global_variables){
      if(parent_process->pid != 0){
        SIMIX_segment_index_set(process, parent_process->segment_index);
      } else {
        SIMIX_segment_index_set(process, process->pid - 1);
      }
    }
  #endif
  }

  /* Process data for auto-restart */
  process->auto_restart = false;
  process->code = nullptr;
  process->argc = 0;
  process->argv = nullptr;

  XBT_VERB("Create context %s", process->name);
  if (!simix_global)
    xbt_die("simix is not initialized, please call MSG_init first");
  process->context = simix_global->context_factory->attach(
    simix_global->cleanup_process_function, process);

  process->running_ctx = (xbt_running_ctx_t*) xbt_malloc0(sizeof(xbt_running_ctx_t));
  XBT_RUNNING_CTX_INITIALIZE(process->running_ctx);

  if(MC_is_active()){
    MC_ignore_heap(process->running_ctx, sizeof(*process->running_ctx));
  }

  /* Add properties */
  process->properties = properties;

  /* Add the process to it's host process list */
  xbt_swag_insert(process, sg_host_simix(host)->process_list);

  /* Now insert it in the global process list and in the process to run list */
  xbt_swag_insert(process, simix_global->process_list);
  XBT_DEBUG("Inserting %s(%s) in the to_run list", process->name, sg_host_get_name(host));
  xbt_dynar_push_as(simix_global->process_to_run, smx_process_t, process);

  /* Tracing the process creation */
  TRACE_msg_process_create(process->name, process->pid, process->host);

  auto context = dynamic_cast<simgrid::simix::AttachContext*>(process->context);
  if (!context)
    xbt_die("Not a suitable context");

  context->attach_start();
  return process;
}

void SIMIX_process_detach(void)
{
  auto context = dynamic_cast<simgrid::simix::AttachContext*>(SIMIX_context_self());
  if (!context)
    xbt_die("Not a suitable context");

  simix_global->cleanup_process_function(context->process());

  // Let maestro ignore we are still alive:
  // xbt_swag_remove(context->process(), simix_global->process_list);

  // TODDO, Remove from proces list:
  //   xbt_swag_remove(process, sg_host_simix(host)->process_list);

  context->attach_stop();
  // delete context;
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

void simcall_HANDLER_process_kill(smx_simcall_t simcall, smx_process_t process) {
  SIMIX_process_kill(process, simcall->issuer);
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

  XBT_DEBUG("Killing process %s on %s", process->name, sg_host_get_name(process->host));

  process->context->iwannadie = 1;
  process->blocked = 0;
  process->suspended = 0;
  process->doexception = 0;

  /* destroy the blocking synchro if any */
  if (process->waiting_synchro) {

    switch (process->waiting_synchro->type) {

    case SIMIX_SYNC_EXECUTE:
    case SIMIX_SYNC_PARALLEL_EXECUTE:
      SIMIX_execution_destroy(process->waiting_synchro);
      break;

    case SIMIX_SYNC_COMMUNICATE:
      xbt_fifo_remove(process->comms, process->waiting_synchro);
      SIMIX_comm_cancel(process->waiting_synchro);
      xbt_fifo_remove(process->waiting_synchro->simcalls, &process->simcall);
      SIMIX_comm_destroy(process->waiting_synchro);
      break;

    case SIMIX_SYNC_SLEEP:
      SIMIX_process_sleep_destroy(process->waiting_synchro);
      break;

    case SIMIX_SYNC_JOIN:
      SIMIX_process_sleep_destroy(process->waiting_synchro);
      break;

    case SIMIX_SYNC_SYNCHRO:
      SIMIX_synchro_stop_waiting(process, &process->simcall);
      SIMIX_synchro_destroy(process->waiting_synchro);
      break;

    case SIMIX_SYNC_IO:
      SIMIX_io_destroy(process->waiting_synchro);
      break;

    }

    process->waiting_synchro = NULL;
  }
  if(!xbt_dynar_member(simix_global->process_to_run, &(process)) && process != issuer) {
    XBT_DEBUG("Inserting %s in the to_run list", process->name);
    xbt_dynar_push_as(simix_global->process_to_run, smx_process_t, process);
  }

}

/** @brief Ask another process to raise the given exception
 *
 * @param cat category of exception
 * @param value value associated to the exception
 * @param msg string information associated to the exception
 */
void SIMIX_process_throw(smx_process_t process, xbt_errcat_t cat, int value, const char *msg) {
  SMX_EXCEPTION(process, cat, value, msg);

  if (process->suspended)
    SIMIX_process_resume(process,SIMIX_process_self());

  /* cancel the blocking synchro if any */
  if (process->waiting_synchro) {

    switch (process->waiting_synchro->type) {

    case SIMIX_SYNC_EXECUTE:
    case SIMIX_SYNC_PARALLEL_EXECUTE:
      SIMIX_execution_cancel(process->waiting_synchro);
      break;

    case SIMIX_SYNC_COMMUNICATE:
      xbt_fifo_remove(process->comms, process->waiting_synchro);
      SIMIX_comm_cancel(process->waiting_synchro);
      break;

    case SIMIX_SYNC_SLEEP:
    case SIMIX_SYNC_JOIN:
      SIMIX_process_sleep_destroy(process->waiting_synchro);
      if (!xbt_dynar_member(simix_global->process_to_run, &(process)) && process != SIMIX_process_self()) {
        XBT_DEBUG("Inserting %s in the to_run list", process->name);
        xbt_dynar_push_as(simix_global->process_to_run, smx_process_t, process);
      }
      break;

    case SIMIX_SYNC_SYNCHRO:
      SIMIX_synchro_stop_waiting(process, &process->simcall);
      break;

    case SIMIX_SYNC_IO:
      SIMIX_io_destroy(process->waiting_synchro);
      break;

    }
  }
  process->waiting_synchro = NULL;

}

void simcall_HANDLER_process_killall(smx_simcall_t simcall, int reset_pid) {
  SIMIX_process_killall(simcall->issuer, reset_pid);
}
/**
 * \brief Kills all running processes.
 * \param issuer this one will not be killed
 */
void SIMIX_process_killall(smx_process_t issuer, int reset_pid)
{
  smx_process_t p = NULL;

  while ((p = (smx_process_t) xbt_swag_extract(simix_global->process_list))) {
    if (p != issuer) {
      SIMIX_process_kill(p,issuer);
    }
  }

  if (reset_pid > 0)
    simix_process_maxpid = reset_pid;

  SIMIX_context_runall();

  SIMIX_process_empty_trash();
}

void simcall_HANDLER_process_set_host(smx_simcall_t simcall, smx_process_t process, sg_host_t dest)
{
  process->new_host = dest;
}
void SIMIX_process_change_host(smx_process_t process,
             sg_host_t dest)
{
  xbt_assert((process != NULL), "Invalid parameters");
  xbt_swag_remove(process, sg_host_simix(process->host)->process_list);
  process->host = dest;
  xbt_swag_insert(process, sg_host_simix(dest)->process_list);
}


void simcall_HANDLER_process_suspend(smx_simcall_t simcall, smx_process_t process)
{
  smx_synchro_t sync_suspend =
      SIMIX_process_suspend(process, simcall->issuer);

  if (process != simcall->issuer) {
    SIMIX_simcall_answer(simcall);
  } else {
    xbt_fifo_push(sync_suspend->simcalls, simcall);
    process->waiting_synchro = sync_suspend;
    SIMIX_execution_suspend(process->waiting_synchro);
  }
  /* If we are suspending ourselves, then just do not finish the simcall now */
}

smx_synchro_t SIMIX_process_suspend(smx_process_t process, smx_process_t issuer)
{
  xbt_assert((process != NULL), "Invalid parameters");

  if (process->suspended) {
    XBT_DEBUG("Process '%s' is already suspended", process->name);
    return NULL;
  }

  process->suspended = 1;

  /* If we are suspending another process, and it is waiting on a sync,
     suspend its synchronization. */
  if (process != issuer) {

    if (process->waiting_synchro) {

      switch (process->waiting_synchro->type) {

        case SIMIX_SYNC_EXECUTE:
        case SIMIX_SYNC_PARALLEL_EXECUTE:
          SIMIX_execution_suspend(process->waiting_synchro);
          break;

        case SIMIX_SYNC_COMMUNICATE:
          SIMIX_comm_suspend(process->waiting_synchro);
          break;

        case SIMIX_SYNC_SLEEP:
          SIMIX_process_sleep_suspend(process->waiting_synchro);
          break;

        case SIMIX_SYNC_SYNCHRO:
          /* Suspension is delayed to when the process is rescheduled. */
          break;

        default:
          xbt_die("Internal error in SIMIX_process_suspend: unexpected synchronization type %d",
              (int)process->waiting_synchro->type);
      }
      return NULL;
    } else {
      /* Suspension is delayed to when the process is rescheduled. */
      return NULL;
    }
  } else {
    /* FIXME: computation size is zero. Is it okay that bound is zero ? */
    return SIMIX_execution_start(process, "suspend", 0.0, 1.0, 0.0, 0);
  }
}

void simcall_HANDLER_process_resume(smx_simcall_t simcall, smx_process_t process){
  SIMIX_process_resume(process, simcall->issuer);
}

void SIMIX_process_resume(smx_process_t process, smx_process_t issuer)
{
  XBT_IN("process = %p, issuer = %p", process, issuer);

  if(process->context->iwannadie) {
    XBT_VERB("Ignoring request to suspend a process that is currently dying.");
    return;
  }

  if(!process->suspended) return;
  process->suspended = 0;

  /* If we are resuming another process, resume the synchronization it was waiting for
     if any. Otherwise add it to the list of process to run in the next round. */
  if (process != issuer) {

    if (process->waiting_synchro) {

      switch (process->waiting_synchro->type) {

        case SIMIX_SYNC_EXECUTE:
        case SIMIX_SYNC_PARALLEL_EXECUTE:
          SIMIX_execution_resume(process->waiting_synchro);
          break;

        case SIMIX_SYNC_COMMUNICATE:
          SIMIX_comm_resume(process->waiting_synchro);
          break;

        case SIMIX_SYNC_SLEEP:
          SIMIX_process_sleep_resume(process->waiting_synchro);
          break;

        case SIMIX_SYNC_SYNCHRO:
          /* I cannot resume it now. This is delayed to when the process is rescheduled at
           * the end of the synchro. */
          break;

        default:
          xbt_die("Internal error in SIMIX_process_resume: unexpected synchronization type %d",
              (int)process->waiting_synchro->type);
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

int SIMIX_process_get_PID(smx_process_t self){
  if (self == NULL)
    return 0;
  else
    return self->pid;
}

int SIMIX_process_get_PPID(smx_process_t self){
  if (self == NULL)
    return 0;
  else
    return self->ppid;
}

void* SIMIX_process_self_get_data()
{
  smx_process_t self = SIMIX_process_self();

  if (!self) {
    return NULL;
  }
  return SIMIX_process_get_data(self);
}

void SIMIX_process_self_set_data(void *data)
{
  smx_process_t self = SIMIX_process_self();

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

sg_host_t SIMIX_process_get_host(smx_process_t process)
{
  return process->host;
}

xbt_main_func_t SIMIX_process_get_code(void){
  return SIMIX_process_self()->code;
}

/* needs to be public and without simcall because it is called
   by exceptions and logging events */
const char* SIMIX_process_self_get_name(void) {

  smx_process_t process = SIMIX_process_self();
  if (process == NULL || process == simix_global->maestro_process)
    return "maestro";

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

void simcall_HANDLER_process_join(smx_simcall_t simcall, smx_process_t process, double timeout)
{
  smx_synchro_t sync = SIMIX_process_join(simcall->issuer, process, timeout);
  xbt_fifo_push(sync->simcalls, simcall);
  simcall->issuer->waiting_synchro = sync;
}

static int SIMIX_process_join_finish(smx_process_exit_status_t status, smx_synchro_t sync){
  if (sync->sleep.surf_sleep) {
    sync->sleep.surf_sleep->cancel();

    smx_simcall_t simcall;
    while ((simcall = (smx_simcall_t) xbt_fifo_shift(sync->simcalls))) {
      simcall_process_sleep__set__result(simcall, SIMIX_DONE);
      simcall->issuer->waiting_synchro = NULL;
      if (simcall->issuer->suspended) {
        XBT_DEBUG("Wait! This process is suspended and can't wake up now.");
        simcall->issuer->suspended = 0;
        simcall_HANDLER_process_suspend(simcall, simcall->issuer);
      } else {
        SIMIX_simcall_answer(simcall);
      }
    }
    sync->sleep.surf_sleep->unref();
    sync->sleep.surf_sleep = NULL;
  }
  xbt_mallocator_release(simix_global->synchro_mallocator, sync);
  return 0;
}

smx_synchro_t SIMIX_process_join(smx_process_t issuer, smx_process_t process, double timeout)
{
  smx_synchro_t res = SIMIX_process_sleep(issuer, timeout);
  res->type = SIMIX_SYNC_JOIN;
  SIMIX_process_on_exit(process, (int_f_pvoid_pvoid_t)SIMIX_process_join_finish, res);
  return res;
}

void simcall_HANDLER_process_sleep(smx_simcall_t simcall, double duration)
{
  if (MC_is_active() || MC_record_replay_is_active()) {
    MC_process_clock_add(simcall->issuer, duration);
    simcall_process_sleep__set__result(simcall, SIMIX_DONE);
    SIMIX_simcall_answer(simcall);
    return;
  }
  smx_synchro_t sync = SIMIX_process_sleep(simcall->issuer, duration);
  xbt_fifo_push(sync->simcalls, simcall);
  simcall->issuer->waiting_synchro = sync;
}

smx_synchro_t SIMIX_process_sleep(smx_process_t process, double duration)
{
  sg_host_t host = process->host;

  /* check if the host is active */
  if (host->isOff()) {
    THROWF(host_error, 0, "Host %s failed, you cannot call this function",
           sg_host_get_name(host));
  }

  smx_synchro_t synchro = (smx_synchro_t) xbt_mallocator_get(simix_global->synchro_mallocator);
  synchro->type = SIMIX_SYNC_SLEEP;
  synchro->name = NULL;
  synchro->category = NULL;

  synchro->sleep.host = host;
  synchro->sleep.surf_sleep = surf_host_sleep(host, duration);

  synchro->sleep.surf_sleep->setData(synchro);
  XBT_DEBUG("Create sleep synchronization %p", synchro);

  return synchro;
}

void SIMIX_post_process_sleep(smx_synchro_t synchro)
{
  smx_simcall_t simcall;
  e_smx_state_t state;
  xbt_assert(synchro->type == SIMIX_SYNC_SLEEP || synchro->type == SIMIX_SYNC_JOIN);

  while ((simcall = (smx_simcall_t) xbt_fifo_shift(synchro->simcalls))) {

    switch (synchro->sleep.surf_sleep->getState()){
      case SURF_ACTION_FAILED:
        simcall->issuer->context->iwannadie = 1;
        //SMX_EXCEPTION(simcall->issuer, host_error, 0, "Host failed");
        state = SIMIX_SRC_HOST_FAILURE;
        break;

      case SURF_ACTION_DONE:
        state = SIMIX_DONE;
        break;

      default:
        THROW_IMPOSSIBLE;
        break;
    }
    if (simcall->issuer->host->isOff()) {
      simcall->issuer->context->iwannadie = 1;
    }
    simcall_process_sleep__set__result(simcall, state);
    simcall->issuer->waiting_synchro = NULL;
    if (simcall->issuer->suspended) {
      XBT_DEBUG("Wait! This process is suspended and can't wake up now.");
      simcall->issuer->suspended = 0;
      simcall_HANDLER_process_suspend(simcall, simcall->issuer);
    } else {
      SIMIX_simcall_answer(simcall);
    }
  }

  SIMIX_process_sleep_destroy(synchro);
}

void SIMIX_process_sleep_destroy(smx_synchro_t synchro)
{
  XBT_DEBUG("Destroy synchro %p", synchro);
  xbt_assert(synchro->type == SIMIX_SYNC_SLEEP || synchro->type == SIMIX_SYNC_JOIN);

  if (synchro->sleep.surf_sleep) {
    synchro->sleep.surf_sleep->unref();
    synchro->sleep.surf_sleep = NULL;
  }
  if (synchro->type == SIMIX_SYNC_SLEEP)
    xbt_mallocator_release(simix_global->synchro_mallocator, synchro);
}

void SIMIX_process_sleep_suspend(smx_synchro_t synchro)
{
  xbt_assert(synchro->type == SIMIX_SYNC_SLEEP);
  synchro->sleep.surf_sleep->suspend();
}

void SIMIX_process_sleep_resume(smx_synchro_t synchro)
{
  XBT_DEBUG("Synchro state is %d on process_sleep_resume.", synchro->state);
  xbt_assert(synchro->type == SIMIX_SYNC_SLEEP);
  synchro->sleep.surf_sleep->resume();
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

  if (self->suspended) {
    XBT_DEBUG("Hey! I'm suspended.");
    xbt_assert(!self->doexception, "Gasp! This exception may be lost by subsequent calls.");
    self->suspended = 0;
    SIMIX_process_suspend(self, self);
  }

  if (self->doexception) {
    XBT_DEBUG("Wait, maestro left me an exception");
    self->doexception = 0;
    SMX_THROW();
  }

  if(SMPI_switch_data_segment && self->segment_index != -1){
    SMPI_switch_data_segment(self->segment_index);
  }
}

/* callback: context fetching */
xbt_running_ctx_t *SIMIX_process_get_running_context(void)
{
  smx_process_t process = SIMIX_process_self();
  if (process)
    return process->running_ctx;
  else
    return nullptr;
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
  xbt_swag_foreach(proc, simix_global->process_list) {
   if (proc->pid == (unsigned long) PID)
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
  smx_process_exit_status_t exit_status = (process->context->iwannadie) ?
                                         SMX_EXIT_FAILURE : SMX_EXIT_SUCCESS;
  while (!xbt_dynar_is_empty(process->on_exit)) {
    exit_fun = xbt_dynar_pop_as(process->on_exit,s_smx_process_exit_fun_t);
    (exit_fun.fun)((void*)exit_status, exit_fun.arg);
  }
}

void SIMIX_process_on_exit(smx_process_t process, int_f_pvoid_pvoid_t fun, void *data) {
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

smx_process_t simcall_HANDLER_process_restart(smx_simcall_t simcall, smx_process_t process) {
  return SIMIX_process_restart(process, simcall->issuer);
}
/** @brief Restart a process, starting it again from the beginning. */
smx_process_t SIMIX_process_restart(smx_process_t process, smx_process_t issuer) {
  XBT_DEBUG("Restarting process %s on %s", process->name, sg_host_get_name(process->host));
  //retrieve the arguments of the old process
  //FIXME: Factorize this with SIMIX_host_add_auto_restart_process ?
  s_smx_process_arg_t arg;
  arg.code = process->code;
  arg.hostname = sg_host_get_name(process->host);
  arg.kill_time = SIMIX_timer_get_date(process->kill_timer);
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
    new_process = simix_global->create_process_function(
                                          arg.argv[0],
                                          arg.code,
                                          arg.data,
                                          arg.hostname,
                                          arg.kill_time,
                                          arg.argc,
                                          arg.argv,
                                          arg.properties,
                                          arg.auto_restart,
                                          NULL);
  } else {
    new_process = simcall_process_create(
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

void SIMIX_segment_index_set(smx_process_t proc, int index){
  proc->segment_index = index;
}
