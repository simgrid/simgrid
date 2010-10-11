/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/dict.h"
#include "msg/mailbox.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_process, simix,
                                "Logging specific to SIMIX (process)");

/**
 * \brief Move a process to the list of process to destroy. *
 */
void SIMIX_process_cleanup(void *arg)
{
  xbt_swag_remove(arg, simix_global->process_to_run);
  xbt_swag_remove(arg, simix_global->process_list);
  xbt_swag_remove(arg, ((smx_process_t) arg)->smx_host->process_list);
  xbt_swag_insert(arg, simix_global->process_to_destroy);
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
    if (process->exception)
      free(process->exception);
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
  smx_process_t process = NULL;
  process = xbt_new0(s_smx_process_t, 1);

  /* Process data */
  process->name = (char *) "";

  process->exception = xbt_new(ex_ctx_t, 1);
  XBT_CTX_INITIALIZE(process->exception);

  /* Create a dummy context for maestro */
  process->context = SIMIX_context_new(NULL, 0, NULL, NULL, NULL);

  /* Set it as the maestro process */
  simix_global->maestro_process = process;
  simix_global->current_process = process;

  return;
}

/**
 * \brief Creates and runs a new #smx_process_t.
 *
 * A constructor for #m_process_t taking four arguments and returning the corresponding object. The structure (and the corresponding thread) is created, and put in the list of ready process.
 *
 * \param name a name for the object. It is for user-level information and can be NULL.
 * \param data a pointer to any data one may want to attach to the new object.  It is for user-level information and can be NULL. It can be retrieved with the function \ref MSG_process_get_data.
 * \param host the location where the new agent is executed.
 * \param argc first argument passed to \a code
 * \param argv second argument passed to \a code
 * \param clean_process_function The cleanup function of user process. It will be called when the process finish. This function have to call the SIMIX_process_cleanup.
 * \see smx_process_t
 * \return The new corresponding object.
 */
smx_process_t SIMIX_process_create(const char *name,
                                   xbt_main_func_t code, void *data,
                                   const char *hostname, int argc,
                                   char **argv, xbt_dict_t properties)
{
  smx_process_t process = NULL;
  smx_host_t host = SIMIX_host_get_by_name(hostname);

  DEBUG2("Start process %s on host %s", name, hostname);

  if (!SIMIX_host_get_state(host)) {
    WARN2("Cannot launch process '%s' on failed host '%s'", name,
          hostname);
    return NULL;
  }
  process = xbt_new0(s_smx_process_t, 1);

  xbt_assert0(((code != NULL) && (host != NULL)), "Invalid parameters");

  /* Process data */
  process->name = xbt_strdup(name);
  process->smx_host = host;
  process->mutex = NULL;
  process->cond = NULL;
  process->iwannadie = 0;
  process->data = data;

  VERB1("Create context %s", process->name);
  process->context = SIMIX_context_new(code, argc, argv,
                                       simix_global->cleanup_process_function,
                                       process);

  process->exception = xbt_new(ex_ctx_t, 1);
  XBT_CTX_INITIALIZE(process->exception);

  /* Add properties */
  process->properties = properties;

  /* Add the process to it's host process list */
  xbt_swag_insert(process, host->process_list);

  DEBUG1("Start context '%s'", process->name);

  /* Now insert it in the global process list and in the process to run list */
  xbt_swag_insert(process, simix_global->process_list);
  DEBUG2("Inserting %s(%s) in the to_run list", process->name, host->name);
  xbt_swag_insert(process, simix_global->process_to_run);

  return process;
}

/** \brief Kill a SIMIX process
 *
 * This function simply kills a \a process... scarry isn't it ? :).
 * \param process poor victim
 *
 */
void SIMIX_process_kill(smx_process_t process)
{
  DEBUG2("Killing process %s on %s", process->name,
         process->smx_host->name);

  process->iwannadie = 1;

  /* If I'm killing myself then stop otherwise schedule the process to kill
   * Two different behaviors, if I'm killing my self, remove from mutex and condition and stop. Otherwise, first we must schedule the process, wait its ending and after remove it from mutex and condition */
  if (process == SIMIX_process_self()) {
    /* Cleanup if we were waiting for something */
    if (process->mutex)
      xbt_swag_remove(process, process->mutex->sleeping);

    if (process->cond)
      xbt_swag_remove(process, process->cond->sleeping);
    if (process->waiting_action) {
      SIMIX_unregister_action_to_condition(process->waiting_action,
                                           process->cond);
      SIMIX_action_destroy(process->waiting_action);
    }

    if (process->sem) {
      xbt_fifo_remove(process->sem->sleeping, process);

      if (process->waiting_action) {
        SIMIX_unregister_action_to_semaphore(process->waiting_action,
                                             process->sem);
        SIMIX_action_destroy(process->waiting_action);
      }
    }

    SIMIX_context_stop(process->context);

  } else {
    DEBUG4("%s(%p) here! killing %s(%p)",
           simix_global->current_process->name,
           simix_global->current_process, process->name, process);

    /* Cleanup if it were waiting for something */
    if (process->mutex) {
      xbt_swag_remove(process, process->mutex->sleeping);
      process->mutex = NULL;
    }

    if (process->cond) {
      xbt_swag_remove(process, process->cond->sleeping);

      if (process->waiting_action) {
        SIMIX_unregister_action_to_condition(process->waiting_action,
                                             process->cond);
        SIMIX_action_destroy(process->waiting_action);
      }
      process->cond = NULL;
    }

    if (process->sem) {
      xbt_fifo_remove(process->sem->sleeping, process);

      if (process->waiting_action) {
        SIMIX_unregister_action_to_semaphore(process->waiting_action,
                                             process->sem);
        SIMIX_action_destroy(process->waiting_action);
      }
      process->sem = NULL;
    }

    /* make sure that the process gets awake soon enough, now that we've set its iwannadie to 1 */
    process->blocked = 0;
    process->suspended = 0;
    xbt_swag_insert(process, simix_global->process_to_run);
  }
}

/**
 * \brief Return the user data of a #smx_process_t.
 *
 * This functions checks whether \a process is a valid pointer or not and return the user data associated to \a process if it is possible.
 * \param process SIMIX process
 * \return A void pointer to the user data
 */
XBT_INLINE void *SIMIX_process_get_data(smx_process_t process)
{
  xbt_assert0((process != NULL), "Invalid parameters");
  return (process->data);
}

/**
 * \brief Set the user data of a #m_process_t.
 *
 * This functions checks whether \a process is a valid pointer or not and set the user data associated to \a process if it is possible.
 * \param process SIMIX process
 * \param data User data
 */
XBT_INLINE void SIMIX_process_set_data(smx_process_t process, void *data)
{
  xbt_assert0((process != NULL), "Invalid parameters");

  process->data = data;
  return;
}

/**
 * \brief Return the location on which an agent is running.
 *
 * This functions checks whether \a process is a valid pointer or not and return the m_host_t corresponding to the location on which \a process is running.
 * \param process SIMIX process
 * \return SIMIX host
 */
XBT_INLINE smx_host_t SIMIX_process_get_host(smx_process_t process)
{
  xbt_assert0((process != NULL), "Invalid parameters");
  return (process->smx_host);
}

/**
 * \brief Return the name of an agent.
 *
 * This functions checks whether \a process is a valid pointer or not and return its name.
 * \param process SIMIX process
 * \return The process name
 */
XBT_INLINE const char *SIMIX_process_get_name(smx_process_t process)
{
  xbt_assert0((process != NULL), "Invalid parameters");
  return (process->name);
}

/**
 * \brief Changes the name of an agent.
 *
 * This functions checks whether \a process is a valid pointer or not and return its name.
 * \param process SIMIX process
 * \param name The new process name
 */
XBT_INLINE void SIMIX_process_set_name(smx_process_t process, char *name)
{
  xbt_assert0((process != NULL), "Invalid parameters");
  process->name = name;
}

/** \ingroup m_process_management
 * \brief Return the properties
 *
 * This functions returns the properties associated with this process
 */
XBT_INLINE xbt_dict_t SIMIX_process_get_properties(smx_process_t process)
{
  return process->properties;
}

/**
 * \brief Return the current agent.
 *
 * This functions returns the currently running #smx_process_t.
 * \return The SIMIX process
 */
XBT_INLINE smx_process_t SIMIX_process_self(void)
{
  if (simix_global)
    return simix_global->current_process;
  return NULL;
}

/**
 * \brief Suspend the process.
 *
 * This functions suspend the process by suspending the action on
 * which it was waiting for the completion.
 *
 * \param process SIMIX process
 */
void SIMIX_process_suspend(smx_process_t process)
{
  xbt_assert0(process, "Invalid parameters");

  if (process != SIMIX_process_self()) {

    if (process->mutex) {
      /* process blocked on a mutex or sem, only set suspend=1 */
      process->suspended = 1;
    } else if (process->cond) {
      /* process blocked cond, suspend all actions */

      /* temporaries variables */
      smx_cond_t c;
      xbt_fifo_item_t i;
      smx_action_t act;

      process->suspended = 1;
      c = process->cond;
      xbt_fifo_foreach(c->actions, i, act, smx_action_t) {
        SIMIX_action_suspend(act);
      }
    } else if (process->sem) {
      smx_sem_t s;
      xbt_fifo_item_t i;
      smx_action_t act;

      process->suspended = 1;
      s = process->sem;
      xbt_fifo_foreach(s->actions, i, act, smx_action_t) {
        SIMIX_action_suspend(act);
      }
    } else {
      process->suspended = 1;
    }
  } else {
    /* process executing, I can create an action and suspend it */
    smx_action_t dummy;
    smx_cond_t cond;
    char name[] = "dummy";
    process->suspended = 1;

    cond = SIMIX_cond_init();
    dummy = SIMIX_action_execute(SIMIX_process_get_host(process), name, 0);
    SIMIX_process_self()->waiting_action = dummy;
    SIMIX_action_suspend(dummy);
    SIMIX_register_action_to_condition(dummy, cond);
    __SIMIX_cond_wait(cond);
    SIMIX_process_self()->waiting_action = NULL;
    SIMIX_unregister_action_to_condition(dummy, cond);
    SIMIX_action_destroy(dummy);
    SIMIX_cond_destroy(cond);
  }
  return;
}

/**
 * \brief Resume a suspended process.
 *
 * This functions resume a suspended process by resuming the task on which it was waiting for the completion.
 * \param process SIMIX process
 */
void SIMIX_process_resume(smx_process_t process)
{
  xbt_assert0((process != NULL), "Invalid parameters");
  SIMIX_CHECK_HOST();

  if (process == SIMIX_process_self())
    return;

  if (process->mutex) {
    DEBUG0("Resume process blocked on a mutex or semaphore");
    process->suspended = 0;     /* It'll wake up by itself when mutex releases */
    return;
  } else if (process->cond) {
    /* temporaries variables */
    smx_cond_t c;
    xbt_fifo_item_t i;
    smx_action_t act;
    DEBUG0("Resume process blocked on a conditional");
    process->suspended = 0;
    c = process->cond;
    xbt_fifo_foreach(c->actions, i, act, smx_action_t) {
      SIMIX_action_resume(act);
    }
    SIMIX_cond_signal(c);
    return;
  } else if (process->sem) {
    /* temporaries variables */
    smx_sem_t s;
    xbt_fifo_item_t i;
    smx_action_t act;
    DEBUG0("Resume process blocked on a semaphore");
    process->suspended = 0;
    s = process->sem;
    xbt_fifo_foreach(s->actions, i, act, smx_action_t) {
      SIMIX_action_resume(act);
    }
    return;
  } else {
    process->suspended = 0;
    xbt_swag_insert(process, simix_global->process_to_run);
  }
}

/**
 * \brief Migrates an agent to another location.
 *
 * This function changes the value of the host on which \a process is running.
 */
void SIMIX_process_change_host(smx_process_t process, char *source,
                               char *dest)
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

/**
 * \brief Returns true if the process is suspended .
 *
 * This checks whether a process is suspended or not by inspecting the task on which it was waiting for the completion.
 * \param process SIMIX process
 * \return 1, if the process is suspended, else 0.
 */
XBT_INLINE int SIMIX_process_is_suspended(smx_process_t process)
{
  xbt_assert0((process != NULL), "Invalid parameters");

  return (process->suspended);
}

/**
 * \brief Returns the amount of SIMIX processes in the system
 *
 * Maestro internal process is not counted, only user code processes are
 */
XBT_INLINE int SIMIX_process_count()
{
  return xbt_swag_size(simix_global->process_list);
}

/** 
 * Calling this function makes the process process to yield. The process
 * that scheduled it returns from __SIMIX_process_schedule as if nothing
 * had happened.
 * 
 * Only the processes can call this function, giving back the control
 * to the maestro
 */
void SIMIX_process_yield(void)
{
  DEBUG1("Yield process '%s'", simix_global->current_process->name);
  xbt_assert0((simix_global->current_process !=
               simix_global->maestro_process),
              "You are not supposed to run this function in maestro context!");


  /* Go into sleep and return control to maestro */
  SIMIX_context_suspend(simix_global->current_process->context);
  /* Ok, maestro returned control to us */

  if (simix_global->current_process->iwannadie)
    SIMIX_context_stop(simix_global->current_process->context);
}

void SIMIX_process_schedule(smx_process_t new_process)
{
  xbt_assert0(simix_global->current_process ==
              simix_global->maestro_process,
              "This function can only be called from maestro context");
  DEBUG1("Scheduling context: '%s'", new_process->name);

  /* update the current process */
  simix_global->current_process = new_process;

  /* schedule the context */
  SIMIX_context_resume(new_process->context);
  DEBUG1("Resumed from scheduling context: '%s'", new_process->name);

  /* restore the current process to the previously saved process */
  simix_global->current_process = simix_global->maestro_process;
}

/* callback: context fetching */
ex_ctx_t *SIMIX_process_get_exception(void)
{
  return simix_global->current_process->exception;
}

/* callback: termination */
void SIMIX_process_exception_terminate(xbt_ex_t * e)
{
  xbt_ex_display(e);
  abort();
}
