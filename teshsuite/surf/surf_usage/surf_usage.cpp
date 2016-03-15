/* A few basic tests for the surf library                                   */

/* Copyright (c) 2004-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "simgrid/sg_config.h"
#include "simgrid/host.h"
#include "surf/surf.h"
#include "src/surf/surf_interface.hpp"
#include "src/surf/cpu_interface.hpp"

#include "xbt/log.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(surf_test, "Messages specific for surf example");

static const char *string_action(e_surf_action_state_t state)
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

int main(int argc, char **argv)
{
  double now = -1.0;
  surf_init(&argc, argv);       /* Initialize some common structures */
  xbt_cfg_set_parse(_sg_cfg_set, "cpu/model:Cas01");
  xbt_cfg_set_parse(_sg_cfg_set, "network/model:CM02");

  xbt_assert(argc >1, "Usage : %s platform.txt\n", argv[0]);
  parse_platform_file(argv[1]);

  XBT_DEBUG("CPU model: %p", surf_cpu_model_pm);
  XBT_DEBUG("Network model: %p", surf_network_model);
  sg_host_t hostA = sg_host_by_name("Cpu A");
  sg_host_t hostB = sg_host_by_name("Cpu B");

  /* Let's check that those two processors exist */
  XBT_DEBUG("%s : %p", sg_host_get_name(hostA), hostA);
  XBT_DEBUG("%s : %p", sg_host_get_name(hostB), hostB);

  /* Let's do something on it */
  surf_action_t actionA = hostA->pimpl_cpu->execution_start(1000.0);
  surf_action_t actionB = hostB->pimpl_cpu->execution_start(1000.0);
  surf_action_t actionC = surf_host_sleep(hostB, 7.32);

  /* Use whatever calling style you want... */
  e_surf_action_state_t stateActionA = actionA->getState(); /* When you know actionA model type */
  e_surf_action_state_t stateActionB = actionB->getState(); /* If you're unsure about it's model type */
  e_surf_action_state_t stateActionC = actionC->getState(); /* When you know actionA model type */

  /* And just look at the state of these tasks */
  XBT_INFO("actionA state: %s", string_action(stateActionA));
  XBT_INFO("actionB state: %s", string_action(stateActionB));
  XBT_INFO("actionC state: %s", string_action(stateActionC));


  /* Let's do something on it */
  surf_network_model_communicate(surf_network_model, hostA, hostB, 150.0, -1.0);

  surf_solve(-1.0);
  do {
    surf_action_t action = NULL;
    now = surf_get_clock();
    XBT_INFO("Next Event : %g", now);
    XBT_DEBUG("\t CPU actions");
    while ((action = surf_model_extract_failed_action_set((surf_model_t)surf_cpu_model_pm))) {
       XBT_INFO("   CPU Failed action");
       XBT_DEBUG("\t * Failed : %p", action);
       action->unref();
    }
    while ((action = surf_model_extract_done_action_set((surf_model_t)surf_cpu_model_pm))) {
      XBT_INFO("   CPU Done action");
      XBT_DEBUG("\t * Done : %p", action);
      action->unref();
    }
    XBT_DEBUG("\t Network actions");
    while ((action = surf_model_extract_failed_action_set((surf_model_t)surf_network_model))) {
      XBT_INFO("   Network Failed action");
      XBT_DEBUG("\t * Failed : %p", action);
      action->unref();
    }
    while ((action = surf_model_extract_done_action_set((surf_model_t)surf_network_model))) {
      XBT_INFO("   Network Failed action");
      XBT_DEBUG("\t * Done : %p", action);
      action->unref();
    }

  } while ((surf_model_running_action_set_size((surf_model_t)surf_network_model) ||
            surf_model_running_action_set_size((surf_model_t)surf_cpu_model_pm)) && surf_solve(-1.0) >= 0.0);

  XBT_DEBUG("Simulation Terminated");

  surf_exit();
  return 0;
}
