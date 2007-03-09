/* 	$Id$	 */

/* Copyright (c) 2007 Arnaud Legrand, Bruno Donnassolo.
   All rights reserved.                                          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_action, simix,
				"Logging specific to SIMIX (action)");

/************************************* Actions *********************************/

smx_action_t SIMIX_action_communicate(smx_host_t sender,smx_host_t receiver,char * name, double size, double rate)
{
	/* check if the host is active */
 	if ( surf_workstation_resource->extension_public->get_state(sender->simdata->host)!=SURF_CPU_ON) {
		THROW1(1,1,"Host %s failed, you cannot call this function",sender->name);
	}
	if ( surf_workstation_resource->extension_public->get_state(receiver->simdata->host)!=SURF_CPU_ON) {
		THROW1(1,1,"Host %s failed, you cannot call this function",receiver->name);
	}

	/* alloc structures */
	smx_action_t act = xbt_new0(s_smx_action_t,1);
	act->simdata = xbt_new0(s_simdata_action_t,1);
	simdata_action_t simdata = act->simdata;
	simdata->cond_list = xbt_fifo_new();
	
	/* initialize them */
	act->name = xbt_strdup(name);


	simdata->surf_action = surf_workstation_resource->extension_public->
		communicate(sender->simdata->host,
				receiver->simdata->host, size, rate);
	surf_workstation_resource->common_public->action_set_data(simdata->surf_action,act);

	return act;
}

smx_action_t SIMIX_action_execute(smx_host_t host, char * name, double amount)
{
	/* check if the host is active */
 	if ( surf_workstation_resource->extension_public->get_state(host->simdata->host)!=SURF_CPU_ON) {
		THROW1(1,1,"Host %s failed, you cannot call this function",host->name);
	}

	/* alloc structures */
	smx_action_t act = xbt_new0(s_smx_action_t,1);
	act->simdata = xbt_new0(s_simdata_action_t,1);
	simdata_action_t simdata = act->simdata;
	simdata->cond_list = xbt_fifo_new();
	
	/* initialize them */
	simdata->source = host;
	act-> name = xbt_strdup(name);

	/* set communication */
	simdata->surf_action = surf_workstation_resource->extension_public->
		execute(host->simdata->host, amount);

	surf_workstation_resource->common_public->action_set_data(simdata->surf_action,act);

	return act;
}


smx_action_t SIMIX_action_sleep(smx_host_t host,  double duration)
{
	char name[] = "sleep";
	/* alloc structures */
	smx_action_t act = xbt_new0(s_smx_action_t,1);
	act->simdata = xbt_new0(s_simdata_action_t,1);
	simdata_action_t simdata = act->simdata;
	simdata->cond_list = xbt_fifo_new();
	
	/* initialize them */
	simdata->source = host;
	act->name = xbt_strdup(name);

	simdata->surf_action = surf_workstation_resource->extension_public->
		sleep(host->simdata->host, duration);

	surf_workstation_resource->common_public->action_set_data(simdata->surf_action,act);

	return act;
}


void SIMIX_action_cancel(smx_action_t action)
{
  xbt_assert0((action != NULL), "Invalid parameter");

  if(action->simdata->surf_action) {
    surf_workstation_resource->common_public->action_cancel(action->simdata->surf_action);
  }
  return;
}

void SIMIX_action_set_priority(smx_action_t action, double priority)
{
	xbt_assert0( (action != NULL) && (action->simdata != NULL), "Invalid parameter" );

	surf_workstation_resource->common_public->
		set_priority(action->simdata->surf_action, priority);
	return;
}

void SIMIX_action_destroy(smx_action_t action)
{

	xbt_assert0((action != NULL), "Invalid parameter");

	xbt_assert1((xbt_fifo_size(action->simdata->cond_list)==0), 
			"Conditional list not empty %d. There is a problem. Cannot destroy it now!", xbt_fifo_size(action->simdata->cond_list));

	if(action->name) free(action->name);

	xbt_fifo_free(action->simdata->cond_list);

	if(action->simdata->surf_action) 
		action->simdata->surf_action->resource_type->common_public->action_free(action->simdata->surf_action);

	return;
}

void SIMIX_register_action_to_condition(smx_action_t action, smx_cond_t cond)
{
	xbt_assert0( (action != NULL) && (cond != NULL), "Invalid parameters");

	xbt_fifo_push(cond->actions,action);
}

