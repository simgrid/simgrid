/* 	$Id$	 */

/* Copyright (c) 2007 Arnaud Legrand, Bruno Donnassolo.
   All rights reserved.                                          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/log.h"
#include "xbt/ex.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_action, simix,
				"Logging specific to SIMIX (action)");

/************************************* Actions *********************************/
/** \brief Creates a new SIMIX action to communicate two hosts.
 *
 * 	This function creates a SURF action and allocates the data necessary to create the SIMIX action. It can raise a network_error exception if the host is unavailable. 
 *	\param sender SIMIX host sender
 * 	\param receiver SIMIX host receiver
 * 	\param name Action name
 * 	\param size Communication size (in bytes)
 * 	\param rate Communication rate between hosts.
 * 	\return A new SIMIX action
 * */
smx_action_t SIMIX_action_communicate(smx_host_t sender,smx_host_t receiver,char * name, double size, double rate)
{
	/* check if the host is active */
 	if ( surf_workstation_resource->extension_public->get_state(sender->simdata->host)!=SURF_CPU_ON) {
		THROW1(network_error,0,"Host %s failed, you cannot call this function",sender->name);
	}
	if ( surf_workstation_resource->extension_public->get_state(receiver->simdata->host)!=SURF_CPU_ON) {
		THROW1(network_error,0,"Host %s failed, you cannot call this function",receiver->name);
	}

	/* alloc structures */
	smx_action_t act = xbt_new0(s_smx_action_t,1);
	act->simdata = xbt_new0(s_smx_simdata_action_t,1);
	smx_simdata_action_t simdata = act->simdata;
	act->cond_list = xbt_fifo_new();
	
	/* initialize them */
	act->name = xbt_strdup(name);
	simdata->source = sender;


	simdata->surf_action = surf_workstation_resource->extension_public->
		communicate(sender->simdata->host,
				receiver->simdata->host, size, rate);
	surf_workstation_resource->common_public->action_set_data(simdata->surf_action,act);

	return act;
}

/** \brief Creates a new SIMIX action to execute an action.
 *
 * 	This function creates a SURF action and allocates the data necessary to create the SIMIX action. It can raise a host_error exception if the host crashed. 
 *	\param host SIMIX host where the action will be executed
 * 	\param name Action name
 * 	\param amount Task amount (in bytes)
 * 	\return A new SIMIX action
 * */
smx_action_t SIMIX_action_execute(smx_host_t host, char * name, double amount)
{
	/* check if the host is active */
 	if ( surf_workstation_resource->extension_public->get_state(host->simdata->host)!=SURF_CPU_ON) {
		THROW1(host_error,0,"Host %s failed, you cannot call this function",host->name);
	}

	/* alloc structures */
	smx_action_t act = xbt_new0(s_smx_action_t,1);
	act->simdata = xbt_new0(s_smx_simdata_action_t,1);
	smx_simdata_action_t simdata = act->simdata;
	act->cond_list = xbt_fifo_new();
	
	/* initialize them */
	simdata->source = host;
	act-> name = xbt_strdup(name);

	/* set communication */
	simdata->surf_action = surf_workstation_resource->extension_public->
		execute(host->simdata->host, amount);

	surf_workstation_resource->common_public->action_set_data(simdata->surf_action,act);

	return act;
}

/** \brief Creates a new sleep SIMIX action.
 *
 * 	This function creates a SURF action and allocates the data necessary to create the SIMIX action. It can raise a host_error exception if the host crashed. The default SIMIX name of the action is "sleep".  
 *	\param host SIMIX host where the sleep will run.
 * 	\param duration Time duration of the sleep.
 * 	\return A new SIMIX action
 * */
smx_action_t SIMIX_action_sleep(smx_host_t host,  double duration)
{	
	char name[] = "sleep";

	/* check if the host is active */
 	if ( surf_workstation_resource->extension_public->get_state(host->simdata->host)!=SURF_CPU_ON) {
		THROW1(host_error,0,"Host %s failed, you cannot call this function",host->name);
	}

	/* alloc structures */
	smx_action_t act = xbt_new0(s_smx_action_t,1);
	act->simdata = xbt_new0(s_smx_simdata_action_t,1);
	smx_simdata_action_t simdata = act->simdata;
	act->cond_list = xbt_fifo_new();
	
	/* initialize them */
	simdata->source = host;
	act->name = xbt_strdup(name);

	simdata->surf_action = surf_workstation_resource->extension_public->
		sleep(host->simdata->host, duration);

	surf_workstation_resource->common_public->action_set_data(simdata->surf_action,act);

	return act;
}

/**
 * 	\brief Cancels an action.
 *
 *	This functions stops the execution of an action. It calls a surf functions.
 *	\param action The SIMIX action
 */
void SIMIX_action_cancel(smx_action_t action)
{
  xbt_assert0((action != NULL), "Invalid parameter");

  if(action->simdata->surf_action) {
    surf_workstation_resource->common_public->action_cancel(action->simdata->surf_action);
  }
  return;
}

/**
 * 	\brief Changes the action's priority
 *
 *	This functions changes the priority only. It calls a surf functions.
 *	\param action The SIMIX action
 *	\param priority The new priority
 */
void SIMIX_action_set_priority(smx_action_t action, double priority)
{
	xbt_assert0( (action != NULL) && (action->simdata != NULL), "Invalid parameter" );

	surf_workstation_resource->common_public->
		set_priority(action->simdata->surf_action, priority);
	return;
}

/**
 * 	\brief Destroys an action
 *
 *	Destroys an action, freing its memory. This function cannot be called if there are a conditional waiting for it. 
 *	\param action The SIMIX action
 */
void SIMIX_action_destroy(smx_action_t action)
{

	xbt_assert0((action != NULL), "Invalid parameter");

	xbt_assert1((xbt_fifo_size(action->cond_list)==0), 
			"Conditional list not empty %d. There is a problem. Cannot destroy it now!", xbt_fifo_size(action->cond_list));

	if(action->name) xbt_free(action->name);

	xbt_fifo_free(action->cond_list);

	if(action->simdata->surf_action) 
		action->simdata->surf_action->resource_type->common_public->action_free(action->simdata->surf_action);

	xbt_free(action->simdata);
	xbt_free(action);
	return;
}

/**
 * 	\brief Set an action to a condition
 *
 * 	Creates the "link" between an action and a condition. You have to call this function when you create an action and want to wait its ending. 
 *	\param action SIMIX action
 *	\param cond SIMIX cond
 */
void SIMIX_register_action_to_condition(smx_action_t action, smx_cond_t cond)
{
	xbt_assert0( (action != NULL) && (cond != NULL), "Invalid parameters");

	xbt_fifo_push(cond->actions,action);
}

/**
 * 	\brief Return how much remais to be done in the action.
 *
 * 	\param action The SIMIX action
 *	\return Remains cost
 */
double SIMIX_action_get_remains(smx_action_t action) 
{
  xbt_assert0((action != NULL), "Invalid parameter");
	return action->simdata->surf_action->remains;
}
