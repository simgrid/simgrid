/* A few basic tests for the surf library                                   */

/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "surf/surf.h"

const char *string_action(e_surf_action_state_t state);
const char *string_action(e_surf_action_state_t state)
{
  switch (state) {
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


void test(void);
void test(void)
{
  void *cpuA = NULL;
  void *cpuB = NULL;
  surf_action_t actionA = NULL;
  surf_action_t actionB = NULL;
  e_surf_action_state_t stateActionA;
  e_surf_action_state_t stateActionB;
  xbt_maxmin_float_t now = -1.0;

  surf_init(); /* Initialize some common structures */
  surf_cpu_resource_init("platform.txt"); /* Now it is possible to use CPUs */
  surf_network_resource_init("platform.txt"); /* Now it is possible to use CPUs */

  printf("%p \n", surf_cpu_resource);
  cpuA = surf_cpu_resource->common_public->name_service("Cpu A");
  cpuB = surf_cpu_resource->common_public->name_service("Cpu B");

  /* Let's check that those two processors exist */
  printf("%s : %p\n", surf_cpu_resource->common_public->get_resource_name(cpuA), cpuA);
  printf("%s : %p\n", surf_cpu_resource->common_public->get_resource_name(cpuB), cpuB);

  /* Let's do something on it */
  actionA = surf_cpu_resource->extension_public->execute(cpuA, 1000.0);
  actionB = surf_cpu_resource->extension_public->execute(cpuB, 1000.0);

  /* Use whatever calling style you want... */
  stateActionA = surf_cpu_resource->common_public->action_get_state(actionA);	/* When you know actionA resource type */
  stateActionB = actionB->resource_type->common_public->action_get_state(actionB);	/* If you're unsure about it's resource type */

  /* And just look at the stat of these tasks */
  printf("actionA : %p (%s)\n", actionA, string_action(stateActionA));
  printf("actionB : %p (%s)\n", actionB, string_action(stateActionB));

  surf_solve(); /* Takes traces into account. Returns 0.0 */
  do {
    surf_action_t action = NULL;    
    now = surf_get_clock();
    printf("Next Event : " XBT_HEAP_FLOAT_T "\n", now);
    while(action=xbt_swag_extract(surf_cpu_resource->common_public->states.failed_action_set)) {
      printf("\tFailed : %p\n", action);
      action->resource_type->common_public->action_free(action);
    }
    while(action=xbt_swag_extract(surf_cpu_resource->common_public->states.done_action_set)) {
      printf("\tDone : %p\n", action);
      action->resource_type->common_public->action_free(action);
    }
  } while(surf_solve());

  printf("Simulation Terminated\n");

  surf_finalize();
}


int main(int argc, char **argv)
{
  test();
  return 0;
}
