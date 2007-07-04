/* 	$Id$	 */

/* Copyright (c) 2002,2003,2004 Arnaud Legrand. All rights reserved.        */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_process, simix,
				"Logging specific to SIMIX (process)");

/******************************** Process ************************************/
/**
 * \brief Creates and runs a new #smx_process_t.
 *
 * Does exactly the same as #SIMIX_process_create_with_arguments but without 
   providing standard arguments (\a argc, \a argv).
 * \see SIMIX_process_create_with_arguments
 */


void SIMIX_process_cleanup(void *arg)
{
  xbt_swag_remove(arg, simix_global->process_list);
  xbt_swag_remove(arg, simix_global->process_to_run);
  xbt_swag_remove(arg, ((smx_process_t) arg)->simdata->host->simdata->process_list);
  free(((smx_process_t) arg)->name);
  ((smx_process_t) arg)->name = NULL;

  free(((smx_process_t) arg)->simdata);
  ((smx_process_t) arg)->simdata = NULL;
  free(arg);
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
					      smx_process_code_t code, void *data,
					      const char * hostname, int argc, char **argv, void * clean_process_function)
{
  smx_simdata_process_t simdata = xbt_new0(s_smx_simdata_process_t,1);
  smx_process_t process = xbt_new0(s_smx_process_t,1);
  smx_process_t self = NULL;
	smx_host_t host = SIMIX_host_get_by_name(hostname);

  xbt_assert0(((code != NULL) && (host != NULL)), "Invalid parameters");
  /* Simulator Data */

  simdata->host = host;
	simdata->mutex = NULL;
	simdata->cond = NULL;
  simdata->argc = argc;
  simdata->argv = argv;
	if (clean_process_function) {
  	simdata->context = xbt_context_new(code, NULL, NULL, 
					     clean_process_function, process, 
					     simdata->argc, simdata->argv);
	}
	else {
  	simdata->context = xbt_context_new(code, NULL, NULL, 
					     SIMIX_process_cleanup, process, 
					     simdata->argc, simdata->argv);
	}

  /* Process structure */
  process->name = xbt_strdup(name);
  process->simdata = simdata;
  process->data = data;

  xbt_swag_insert(process, host->simdata->process_list);

  /* *************** FIX du current_process !!! *************** */
  self = simix_global->current_process;
  xbt_context_start(process->simdata->context);
  simix_global->current_process = self;

  xbt_swag_insert(process,simix_global->process_list);
  DEBUG2("Inserting %s(%s) in the to_run list",process->name,
	 host->name);
  xbt_swag_insert(process,simix_global->process_to_run);

  return process;
}


/** \brief Kill a SIMIX process
 *
 * This function simply kills a \a process... scarry isn't it ? :). Removes it from the mutex or condition that it can be blocked.
 * \param process poor victim
 *
 */
void SIMIX_process_kill(smx_process_t process)
{	
  smx_simdata_process_t p_simdata = process->simdata;

  DEBUG2("Killing %s on %s",process->name, p_simdata->host->name);
  
	if (p_simdata->mutex) {
		xbt_swag_remove(process,p_simdata->mutex->sleeping);
	}
	if (p_simdata->cond) {
		xbt_swag_remove(process,p_simdata->cond->sleeping);
	}
  xbt_swag_remove(process,simix_global->process_to_run);
  xbt_swag_remove(process,simix_global->process_list);
  xbt_context_kill(process->simdata->context);

  if(process==SIMIX_process_self()) {
    /* I just killed myself */
    xbt_context_yield();
  }
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
void SIMIX_process_set_data(smx_process_t process,void *data)
{
  xbt_assert0((process != NULL), "Invalid parameters");
  xbt_assert0((process->data == NULL), "Data already set");
  
  process->data = data;
   
  return ;
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
  xbt_assert0(((process != NULL) && (process->simdata)), "Invalid parameters");

  return (process->simdata->host);
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
  xbt_assert0(((process != NULL) && (process->simdata)), "Invalid parameters");

  return (process->name);
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
 * This functions suspend the process by suspending the action on which it was waiting for the completion.
 *	\param process SIMIX process
 */
void SIMIX_process_suspend(smx_process_t process)
{
  smx_simdata_process_t simdata = NULL;
	
  xbt_assert0(((process) && (process->simdata)), "Invalid parameters");

  if(process!=SIMIX_process_self()) {
    simdata = process->simdata;
    
		if (simdata->mutex) {
			/* process blocked on a mutex, only set suspend=1 */
			simdata->suspended = 1;
		}
		else if (simdata->cond){
			/* process blocked cond, suspend all actions */

			/* temporaries variables */ 
			smx_cond_t c;
			xbt_fifo_item_t i;
			smx_action_t act;

			simdata->suspended = 1;
			c = simdata->cond;
			xbt_fifo_foreach(c->actions,i,act, smx_action_t) {
				surf_workstation_resource->common_public->suspend(act->simdata->surf_action);
			}
		}
		else {
			simdata->suspended = 1;
		}
  }
	else {
		/* process executing, I can create an action and suspend it */
		process->simdata->suspended = 1;
		smx_action_t dummy;
		smx_cond_t cond;
		char name[] = "dummy";

		cond = SIMIX_cond_init();
		dummy = SIMIX_action_execute(SIMIX_process_get_host(process), name, 0);
		surf_workstation_resource->common_public->set_priority(dummy->simdata->surf_action,0.0);
		SIMIX_register_condition_to_action(dummy,cond);
		SIMIX_register_action_to_condition(dummy,cond);
		__SIMIX_cond_wait(cond);	
		//SIMIX_action_destroy(dummy);
		//SIMIX_cond_destroy(cond);
	}
  return ;
}

/**
 * \brief Resume a suspended process.
 *
 * This functions resume a suspended process by resuming the task on which it was waiting for the completion.
 * \param process SIMIX process
 */
void SIMIX_process_resume(smx_process_t process)
{
  smx_simdata_process_t simdata = NULL;

  xbt_assert0(((process != NULL) && (process->simdata)), "Invalid parameters");
  SIMIX_CHECK_HOST();

  if(process == SIMIX_process_self()) {
 		return; 
  }

  simdata = process->simdata;
  if(simdata->mutex) {
		DEBUG0("Resume process blocked on a mutex");
    simdata->suspended = 0; /* He'll wake up by itself */
 		return; 
  }
	else if (simdata->cond) {
		DEBUG0("Resume process blocked on a conditional");
		/* temporaries variables */ 
		smx_cond_t c;
		xbt_fifo_item_t i;
		smx_action_t act;
		simdata->suspended = 0;
		c = simdata->cond;
		xbt_fifo_foreach(c->actions,i,act, smx_action_t) {
			surf_workstation_resource->common_public->resume(act->simdata->surf_action);
		}
		return;
	}
	else {
		simdata->suspended = 0;
		xbt_swag_insert(process,simix_global->process_to_run);
	}

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
  xbt_assert0(((process != NULL) && (process->simdata)), "Invalid parameters");

  return (process->simdata->suspended);
}



