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

smx_action_t SIMIX_communicate(smx_host_t sender,smx_host_t receiver, double size)
{
	return xbt_new0(s_smx_action_t,1);
}

smx_action_t SIMIX_execute(smx_host_t host,double amount)
{
	return xbt_new0(s_smx_action_t,1);
}

SIMIX_error_t SIMIX_action_cancel(smx_action_t action)
{

	return SIMIX_OK;
}

void SIMIX_action_set_priority(smx_action_t action, double priority)
{
	return;
}

SIMIX_error_t SIMIX_action_destroy(smx_action_t action)
{
	return SIMIX_OK;
}

void SIMIX_create_link(smx_action_t action, smx_cond_t cond)
{

}

SIMIX_error_t __SIMIX_wait_for_action(smx_process_t process, smx_action_t action)
{
	e_surf_action_state_t state = SURF_ACTION_NOT_IN_THE_SYSTEM;
	simdata_action_t simdata = action->simdata;

	xbt_assert0(((process != NULL) && (action != NULL) && (action->simdata != NULL)), "Invalid parameters");
	
	/* change context while the action is running  */
	do {
		xbt_context_yield();
		state=surf_workstation_resource->common_public->action_get_state(simdata->surf_action);
	} while (state==SURF_ACTION_RUNNING);
	
	/* action finished, we can continue */

	if(state == SURF_ACTION_DONE) {
		if(surf_workstation_resource->common_public->action_free(simdata->surf_action)) 
			simdata->surf_action = NULL;
		SIMIX_RETURN(SIMIX_OK);
	} else if(surf_workstation_resource->extension_public->
			get_state(SIMIX_process_get_host(process)->simdata->host) 
			== SURF_CPU_OFF) {
		if(surf_workstation_resource->common_public->action_free(simdata->surf_action)) 
			simdata->surf_action = NULL;
		SIMIX_RETURN(SIMIX_HOST_FAILURE);
	} else {
		if(surf_workstation_resource->common_public->action_free(simdata->surf_action)) 
			simdata->surf_action = NULL;
		SIMIX_RETURN(SIMIX_ACTION_CANCELLED);
	}

}
