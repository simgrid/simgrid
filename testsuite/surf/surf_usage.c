/* A few basic tests for the surf library                                   */

/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include <stdio.h>
#include "simgrid/sg_config.h"
#include "surf/surf.h"
#include "surf/surf_resource.h"
#include "surf/surfxml_parse.h" // for reset callback

#include "xbt/log.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(surf_test,
                             "Messages specific for surf example");

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
  e_surf_action_state_t stateActionA;
  e_surf_action_state_t stateActionB;
  e_surf_action_state_t stateActionC;
  double now = -1.0;
  xbt_cfg_set_parse(_sg_cfg_set, "cpu/model:Cas01");
  xbt_cfg_set_parse(_sg_cfg_set, "network/model:CM02");
  parse_platform_file(platform);

  /*********************** CPU ***********************************/
  XBT_DEBUG("%p", surf_cpu_model_pm);
  cpuA = surf_cpu_resource_by_name("Cpu A");
  cpuB = surf_cpu_resource_by_name("Cpu B");

  /* Let's check that those two processors exist */
  XBT_DEBUG("%s : %p", surf_resource_name(cpuA), cpuA);
  XBT_DEBUG("%s : %p", surf_resource_name(cpuB), cpuB);

  /* Let's do something on it */
  actionA = surf_cpu_model_pm->extension.cpu.execute(cpuA, 1000.0);
  actionB = surf_cpu_model_pm->extension.cpu.execute(cpuB, 1000.0);
  actionC = surf_cpu_model_pm->extension.cpu.sleep(cpuB, 7.32);

  /* Use whatever calling style you want... */
  stateActionA = surf_cpu_model_pm->action_state_get(actionA);     /* When you know actionA model type */
  stateActionB = actionB->model_obj->action_state_get(actionB);        /* If you're unsure about it's model type */
  stateActionC = surf_cpu_model_pm->action_state_get(actionC);     /* When you know actionA model type */

  /* And just look at the state of these tasks */
  XBT_DEBUG("actionA : %p (%s)", actionA, string_action(stateActionA));
  XBT_DEBUG("actionB : %p (%s)", actionB, string_action(stateActionB));
  XBT_DEBUG("actionC : %p (%s)", actionB, string_action(stateActionC));

  /*********************** Network *******************************/
  XBT_DEBUG("%p", surf_network_model);
  cardA = sg_routing_edge_by_name_or_null("Cpu A");
  cardB = sg_routing_edge_by_name_or_null("Cpu B");

  /* Let's check that those two processors exist */
  XBT_DEBUG("%s : %p", surf_resource_name(cardA), cardA);
  XBT_DEBUG("%s : %p", surf_resource_name(cardB), cardB);

  /* Let's do something on it */
  surf_network_model->extension.network.communicate(cardA, cardB,
                                                    150.0, -1.0);

  surf_solve(-1.0);                 /* Takes traces into account. Returns 0.0 */
  do {
    surf_action_t action = NULL;
    now = surf_get_clock();
    XBT_DEBUG("Next Event : %g", now);
    XBT_DEBUG("\t CPU actions");
    while ((action =
            xbt_swag_extract(surf_cpu_model_pm->states.failed_action_set))) {
      XBT_DEBUG("\t * Failed : %p", action);
      action->model_obj->action_unref(action);
    }
    while ((action =
            xbt_swag_extract(surf_cpu_model_pm->states.done_action_set))) {
      XBT_DEBUG("\t * Done : %p", action);
      action->model_obj->action_unref(action);
    }
    XBT_DEBUG("\t Network actions");
    while ((action =
            xbt_swag_extract(surf_network_model->states.
                             failed_action_set))) {
      XBT_DEBUG("\t * Failed : %p", action);
      action->model_obj->action_unref(action);
    }
    while ((action =
            xbt_swag_extract(surf_network_model->states.
                             done_action_set))) {
      XBT_DEBUG("\t * Done : %p", action);
      action->model_obj->action_unref(action);
    }

  } while ((xbt_swag_size(surf_network_model->states.running_action_set) ||
            xbt_swag_size(surf_cpu_model_pm->states.running_action_set)) &&
           surf_solve(-1.0) >= 0.0);

  XBT_DEBUG("Simulation Terminated");
}

#ifdef __BORLANDC__
#pragma argsused
#endif

int main(int argc, char **argv)
{
  surf_init(&argc, argv);       /* Initialize some common structures */
  if (argc == 1) {
    fprintf(stderr, "Usage : %s platform.xml\n", argv[0]);
    return 1;
  }
  test(argv[1]);

  surf_exit();
  return 0;
}
