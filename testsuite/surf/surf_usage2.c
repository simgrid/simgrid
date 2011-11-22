/* A few basic tests for the surf library                                   */

/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include <stdio.h>
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
  void *workstationA = NULL;
  void *workstationB = NULL;
  double now = -1.0;
  int running;

  xbt_cfg_set_parse(_surf_cfg_set, "workstation/model:CLM03");
  parse_platform_file(platform);

  /*********************** WORKSTATION ***********************************/
  workstationA =
      surf_workstation_resource_by_name("Cpu A");
  workstationB =
      surf_workstation_resource_by_name("Cpu B");

  /* Let's check that those two processors exist */
  XBT_DEBUG("%s : %p", surf_resource_name(workstationA), workstationA);
  XBT_DEBUG("%s : %p", surf_resource_name(workstationB), workstationB);

  /* Let's do something on it */
  surf_workstation_model->extension.workstation.execute(workstationA, 1000.0);
  surf_workstation_model->extension.workstation.execute(workstationB, 1000.0);
      surf_workstation_model->extension.workstation.sleep(workstationB, 7.32);

  surf_workstation_model->extension.workstation.
      communicate(workstationA, workstationB, 150.0, -1.0);

  surf_solve(-1.0);                 /* Takes traces into account. Returns 0.0 */
  do {
    surf_action_t action = NULL;
    unsigned int iter;
    surf_model_t model = NULL;
    running = 0;

    now = surf_get_clock();
    XBT_DEBUG("Next Event : %g", now);

    xbt_dynar_foreach(model_list, iter, model) {
      XBT_DEBUG("\t %s actions", model->name);
      while ((action = xbt_swag_extract(model->states.failed_action_set))) {
        XBT_DEBUG("\t * Failed : %p", action);
        model->action_unref(action);
      }
      while ((action = xbt_swag_extract(model->states.done_action_set))) {
        XBT_DEBUG("\t * Done : %p", action);
        model->action_unref(action);
      }
      if (xbt_swag_size(model->states.running_action_set)) {
        XBT_DEBUG("running %s", model->name);
        running = 1;
      }
    }
  } while (running && surf_solve(-1.0) >= 0.0);

  XBT_DEBUG("Simulation Terminated");

}

#ifdef __BORLANDC__
#pragma argsused
#endif

int main(int argc, char **argv)
{
  surf_init(&argc, argv);       /* Initialize some common structures */
  if (argc == 1) {
    fprintf(stderr, "Usage : %s platform.txt\n", argv[0]);
    surf_exit();
    return 1;
  }
  test(argv[1]);

  surf_exit();
  return 0;
}
