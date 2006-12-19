/* 	$Id$	 */

/* A few basic tests for the surf library                                   */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#pragma hdrstop

#include <stdio.h>
#include "surf/surf.h"

#include "xbt/log.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(surf_test,"Messages specific for surf example");

const char *string_action(e_surf_action_state_t state);
const char *string_action(e_surf_action_state_t state)
{
	switch (state) 
	{
		case (SURF_ACTION_READY):
		return "SURF_ACTION_READY";
		
		case (SURF_ACTION_RUNNING):
		return "SURF_ACTION_RUNNING";
		
		case (SURF_ACTION_FAILED):
		return "SURF_ACTION_FAILED";
		
		case (SURF_ACTION_DONE):
		return "SURF_ACTION_DONE";
		
		case (SURF_ACTION_NOT_IN_THE_SYSTEM):
		return "SURF_ACTION_NOT_IN_THE_SYSTEM";
		
		default:
		return "INVALID STATE";
	}
}


void test(char *platform);
void test(char *platform)
{
	void *cpuA = NULL;
	void *cpuB = NULL;
	void *cardA = NULL;
	void *cardB = NULL;
	surf_action_t actionA = NULL;
	surf_action_t actionB = NULL;
	surf_action_t actionC = NULL;
	surf_action_t commAB = NULL;
	e_surf_action_state_t stateActionA;
	e_surf_action_state_t stateActionB;
	e_surf_action_state_t stateActionC;
	double now = -1.0;
	
	surf_cpu_resource_init_Cas01(platform);	/* Now it is possible to use CPUs */
	surf_network_resource_init_CM02(platform);	/* Now it is possible to use eth0 */
	
	/*********************** CPU ***********************************/
	DEBUG1("%p \n", surf_cpu_resource);
	cpuA = surf_cpu_resource->common_public->name_service("Cpu A");
	cpuB = surf_cpu_resource->common_public->name_service("Cpu B");
	
	/* Let's check that those two processors exist */
	DEBUG2("%s : %p\n",
	surf_cpu_resource->common_public->get_resource_name(cpuA), cpuA);
	DEBUG2("%s : %p\n",
	surf_cpu_resource->common_public->get_resource_name(cpuB), cpuB);
	
	/* Let's do something on it */
	actionA = surf_cpu_resource->extension_public->execute(cpuA, 1000.0);
	actionB = surf_cpu_resource->extension_public->execute(cpuB, 1000.0);
	actionC = surf_cpu_resource->extension_public->sleep(cpuB, 7.32);
	
	/* Use whatever calling style you want... */
	stateActionA = surf_cpu_resource->common_public->action_get_state(actionA);	/* When you know actionA resource type */
	stateActionB = actionB->resource_type->common_public->action_get_state(actionB);	/* If you're unsure about it's resource type */
	stateActionC = surf_cpu_resource->common_public->action_get_state(actionC);	/* When you know actionA resource type */
	
	/* And just look at the state of these tasks */
	DEBUG2("actionA : %p (%s)\n", actionA, string_action(stateActionA));
	DEBUG2("actionB : %p (%s)\n", actionB, string_action(stateActionB));
	DEBUG2("actionC : %p (%s)\n", actionB, string_action(stateActionC));
	
	/*********************** Network *******************************/
	DEBUG1("%p \n", surf_network_resource);
	cardA = surf_network_resource->common_public->name_service("Cpu A");
	cardB = surf_network_resource->common_public->name_service("Cpu B");
	
	/* Let's check that those two processors exist */
	DEBUG2("%s : %p\n",
	surf_network_resource->common_public->get_resource_name(cardA),cardA);
	DEBUG2("%s : %p\n",
	surf_network_resource->common_public->get_resource_name(cardB),cardB);
	
	/* Let's do something on it */
	commAB =surf_network_resource->extension_public->communicate(cardA, cardB,150.0,-1.0);
	
	surf_solve();			/* Takes traces into account. Returns 0.0 */
	
	do 
	{
		surf_action_t action = NULL;
		now = surf_get_clock();
		DEBUG1("Next Event : " "%g" "\n", now);
		DEBUG0("\t CPU actions\n");
		
		while ((action =xbt_swag_extract(surf_cpu_resource->common_public->states.failed_action_set))) 
		{
			DEBUG1("\t * Failed : %p\n", action);
			action->resource_type->common_public->action_free(action);
		}
		
		while ((action =xbt_swag_extract(surf_cpu_resource->common_public->states.done_action_set))) 
		{
			DEBUG1("\t * Done : %p\n", action);
			action->resource_type->common_public->action_free(action);
		}
		
		DEBUG0("\t Network actions\n");
		
		while ((action =xbt_swag_extract(surf_network_resource->common_public->states.failed_action_set))) 
		{
			DEBUG1("\t * Failed : %p\n", action);
			action->resource_type->common_public->action_free(action);
		}
		
		while ((action =xbt_swag_extract(surf_network_resource->common_public->states.done_action_set))) 
		{
			DEBUG1("\t * Done : %p\n", action);
			action->resource_type->common_public->action_free(action);
		}
	
	}while (surf_solve()>=0.0);
	
	DEBUG0("Simulation Terminated\n");
}

#pragma argsused
int main(int argc, char **argv)
{
	surf_init(&argc, argv);	/* Initialize some common structures */
	
	if(argc==1) 
	{
		fprintf(stderr,"Usage : %s platform.txt\n",argv[0]);
		return 1;
	}
	
	test(argv[1]);
	surf_exit();
	
	return 0;
}
