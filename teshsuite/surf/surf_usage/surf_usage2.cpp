/* A few basic tests for the surf library                                   */

/* Copyright (c) 2004-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/sg_config.h"
#include "simgrid/host.h"
#include "surf/surf.h"
#include "src/surf/surf_interface.hpp"
#include "src/surf/cpu_interface.hpp"

#include "xbt/log.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(surf_test, "Messages specific for surf example");

int main(int argc, char **argv)
{
  sg_host_t hostA = NULL;
  sg_host_t hostB = NULL;
  double now = -1.0;
  int running;

  surf_init(&argc, argv);       /* Initialize some common structures */

  xbt_cfg_set_parse(_sg_cfg_set, "network/model:CM02");
  xbt_cfg_set_parse(_sg_cfg_set, "cpu/model:Cas01");

  xbt_assert(argc >1, "Usage : %s platform.txt\n", argv[0]);
  parse_platform_file(argv[1]);

  /*********************** HOST ***********************************/
  hostA = sg_host_by_name("Cpu A");
  hostB = sg_host_by_name("Cpu B");

  /* Let's check that those two processors exist */
  XBT_DEBUG("%s : %p", sg_host_get_name(hostA), hostA);
  XBT_DEBUG("%s : %p", sg_host_get_name(hostB), hostB);

  /* Let's do something on it */
  hostA->pimpl_cpu->execution_start(1000.0);
  hostB->pimpl_cpu->execution_start(1000.0);
  surf_host_sleep(hostB, 7.32);

  surf_network_model_communicate(surf_network_model, hostA, hostB, 150.0, -1.0);

  surf_solve(-1.0);                 /* Takes traces into account. Returns 0.0 */
  do {
    surf_action_t action = NULL;
    unsigned int iter;
    surf_model_t model = NULL;
    running = 0;

    now = surf_get_clock();
    XBT_INFO("Next Event : %g", now);

    xbt_dynar_foreach(all_existing_models, iter, model) {
      if (surf_model_running_action_set_size((surf_model_t)model)) {
        XBT_DEBUG("\t Running that model");
        running = 1;
      }
      while ((action = surf_model_extract_failed_action_set((surf_model_t)model))) {
        XBT_INFO("   * Done Action");
        XBT_DEBUG("\t * Failed Action: %p", action);
        action->unref();
      }
      while ((action = surf_model_extract_done_action_set((surf_model_t)model))) {
        XBT_INFO("   * Done Action");
        XBT_DEBUG("\t * Done Action: %p", action);
        action->unref();
      }
    }
  } while (running && surf_solve(-1.0) >= 0.0);

  XBT_INFO("Simulation Terminated");
  surf_exit();
  return 0;
}
