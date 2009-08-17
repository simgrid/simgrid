//*     $Id$     */

/* Copyright (c) 2002,2003,2004 Arnaud Legrand. All rights reserved.        */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/dict.h"
#include "msg/mailbox.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_process, simix,
                                "Logging specific to SIMIX (process)");

/******************************** Process ************************************/
/**
 * \brief Move a process to the list of process to destroy. *
 */

void SIMIX_process_cleanup(void *arg)
{
  xbt_swag_remove(arg, simix_global->process_to_run);
  xbt_swag_remove(arg, simix_global->process_list);
  xbt_swag_remove(arg, ((smx_process_t)arg)->smx_host->process_list);
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
  int i;  

  while ((process = xbt_swag_extract(simix_global->process_to_destroy))){
    free(process->name);
    process->name = NULL;
  
    if (process->argv) {
      for (i = 0; i < process->argc; i++)
        if (process->argv[i])
          free(process->argv[i]);

      free(process->argv);
    }
  
    free(process);
  }
}

/**
 * \brief Creates and runs the maestro process
 *
 */
void __SIMIX_create_maestro_process()
{
  smx_process_t process = NULL;
  process = xbt_new0(s_smx_process_t, 1);

  /* Process data */
  process->name = (char *)"";

  /* Create the right context type */
  process->context = SIMIX_context_create_maestro();

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
    WARN2("Cannot launch process '%s' on failed host '%s'", name, hostname);
    return NULL;
  }
  process = xbt_new0(s_smx_process_t, 1);

  xbt_assert0(((code != NULL) && (host != NULL)), "Invalid parameters");

  /* Process data */
  process->name = xbt_strdup(name);
  process->smx_host = host;
  process->argc = argc;
  process->argv = argv;
  process->mutex = NULL;
  process->cond = NULL;
  process->iwannadie = 0;

  VERB1("Create context %s", process->name);
  process->context = SIMIX_context_new(code);

  process->data = data;
  process->cleanup_func = simix_global->cleanup_process_function;
  process->cleanup_arg = process;

  /* Add properties */
  process->properties = properties;

  /* Add the process to it's host process list */
  xbt_swag_insert(process, host->process_list);

  DEBUG1("Start context '%s'", process->name);
  SIMIX_context_start(process->context);
   
  /* Now insert it in the global process list and in the process to run list */
  xbt_swag_insert(process, simix_global->process_list);
  DEBUG2("Inserting %s(%s) in the to_run list", process->name, host->name);
  xbt_swag_insert(process, simix_global->process_to_run);

  return process;
}

/**
 * \brief Creates and runs a new #smx_process_t hosting a JAVA thread
 *
 * Warning: this should only be used in libsimgrid4java, since it create
 * a context with no code, which leads to segfaults in plain libsimgrid
 */
void SIMIX_jprocess_create(const char *name, smx_host_t host,
                           void *data,
                           void *jprocess, void *jenv, smx_process_t * res)
{
  smx_process_t process = xbt_new0(s_smx_process_t, 1);
  smx_process_t self = NULL;

  /* HACK: We need this trick because when we xbt_context_new() do
     syncronization stuff, the s_process field in the m_process needs
     to have a valid value, and we call xbt_context_new() before
     returning, of course, ie, before providing a right value to the
     caller (Java_simgrid_msg_Msg_processCreate) have time to store it
     in place. This way, we initialize the m_process->simdata->s_process
     field ourself ASAP.

     All this would be much simpler if the synchronization stuff would be done
     in the JAVA world, I think.
   */
  *res = process;

  DEBUG5("jprocess_create(name=%s,host=%p,data=%p,jproc=%p,jenv=%p)",
         name, host, data, jprocess, jenv);
  xbt_assert0(host, "Invalid parameters");

  /* Process data */
  process->name = xbt_strdup(name);
  process->smx_host = host;
  process->argc = 0;
  process->argv = NULL;
  process->mutex = NULL;
  process->cond = NULL;
  SIMIX_context_new(jprocess);
  process->data = data;

  /* Add the process to it's host process list */
  xbt_swag_insert(&process, host->process_list);

  /* fix current_process, about which xbt_context_start mocks around */
  self = simix_global->current_process;
  SIMIX_context_start(process->context);
  simix_global->current_process = self;

  /* Now insert it in the global process list and in the process to run list */
  xbt_swag_insert(process, simix_global->process_list);
  DEBUG2("Inserting %s(%s) in the to_run list", process->name, host->name);
  xbt_swag_insert(process, simix_global->process_to_run);
}

/** \brief Kill a SIMIX process
 *
 * This function simply kills a \a process... scarry isn't it ? :).
 * \param process poor victim
 *
 */
void SIMIX_process_kill(smx_process_t process)
{
  DEBUG2("Killing process %s on %s", process->name, process->smx_host->name);

  /* Cleanup if we were waiting for something */
  if (process->mutex)
    xbt_swag_remove(process, process->mutex->sleeping);

  if (process->cond)
    xbt_swag_remove(process, process->cond->sleeping);

  DEBUG2("%p here! killing %p", simix_global->current_process, process);

  process->iwannadie = 1;

  /* If I'm killing myself then stop otherwise schedule the process to kill */
  if (process == SIMIX_process_self())
    SIMIX_context_stop(1);
  else
    __SIMIX_process_schedule(process);
  
}

/**
 * \brief Return the user data of a #smx_process_t.
 *
 * This functions checks whether \a process is a valid pointer or not and return the user data associated to \a process if it is possible.
 * \param process SIMIX process
 * \return A void pointer to the user data
 */
void *SIMIX_process_get_data(smx_process_t process)
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
void SIMIX_process_set_data(smx_process_t process, void *data)
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
smx_host_t SIMIX_process_get_host(smx_process_t process)
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
const char *SIMIX_process_get_name(smx_process_t process)
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
void SIMIX_process_set_name(smx_process_t process, char *name)
{
  xbt_assert0((process != NULL), "Invalid parameters");
  process->name = name;
}

/** \ingroup m_process_management
 * \brief Return the properties
 *
 * This functions returns the properties associated with this process
 */
xbt_dict_t SIMIX_process_get_properties(smx_process_t process)
{
  return process->properties;
}

/**
 * \brief Return the current agent.
 *
 * This functions returns the currently running #smx_process_t.
 * \return The SIMIX process
 */
smx_process_t SIMIX_process_self(void)
{
  return simix_global ? simix_global->current_process : NULL;
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
      /* process blocked on a mutex, only set suspend=1 */
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
	 surf_workstation_model->suspend(act->surf_action);
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
    surf_workstation_model->suspend(dummy->surf_action);
    SIMIX_register_action_to_condition(dummy, cond);
    __SIMIX_cond_wait(cond);
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
    DEBUG0("Resume process blocked on a mutex");
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
      surf_workstation_model->resume(act->surf_action);
    }
    SIMIX_cond_signal(c);
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
void SIMIX_process_change_host(smx_process_t process, char *source, char *dest)
{
  xbt_assert0((process != NULL), "Invalid parameters");
  smx_host_t h1 = SIMIX_host_get_by_name(source);
  smx_host_t h2 = SIMIX_host_get_by_name(dest);
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
int SIMIX_process_is_suspended(smx_process_t process)
{
  xbt_assert0((process != NULL), "Invalid parameters");

  return (process->suspended);
}

/**
 * \brief Returns the amount of SIMIX processes in the system
 *
 * Maestro internal process is not counted, only user code processes are
 */
int SIMIX_process_count()
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
void __SIMIX_process_yield(void)
{
  DEBUG1("Yield process '%s'", simix_global->current_process->name);
  xbt_assert0((simix_global->current_process != simix_global->maestro_process),
              "You are not supposed to run this function here!");

  SIMIX_context_suspend(simix_global->current_process->context);

  if (simix_global->current_process->iwannadie)
    SIMIX_context_stop(1);
}

void __SIMIX_process_schedule(smx_process_t new_process)
{
  DEBUG1("Scheduling context: '%s'", new_process->name);

  /* save the current process */
  smx_process_t old_process = simix_global->current_process;

  /* update the current process */
  simix_global->current_process = new_process;

  /* schedule the context */
  SIMIX_context_resume(old_process->context, new_process->context);

  /* restore the current process to the previously saved process */
  simix_global->current_process = old_process;
}
