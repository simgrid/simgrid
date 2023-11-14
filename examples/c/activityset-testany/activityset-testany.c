/* Copyright (c) 2010-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/activity_set.h"
#include "simgrid/actor.h"
#include "simgrid/comm.h"
#include "simgrid/engine.h"
#include "simgrid/exec.h"
#include "simgrid/host.h"
#include "simgrid/mailbox.h"

#include "xbt/log.h"
#include "xbt/sysdep.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_activity_testany, "Messages specific for this s4u example");

static void bob(int argc, char* argv[])
{
  XBT_INFO("Create my asynchronous activities");
  sg_exec_t exec = sg_actor_exec_init(5e9);
  sg_exec_start(exec);

  sg_mailbox_t mbox = sg_mailbox_by_name("mbox");
  void* payload     = NULL;
  sg_comm_t comm    = sg_mailbox_get_async(mbox, &payload);

  sg_activity_set_t pending_activities = sg_activity_set_init();
  sg_activity_set_push(pending_activities, (sg_activity_t)exec);
  sg_activity_set_push(pending_activities, (sg_activity_t)comm);

  XBT_INFO("Sleep_for a while");
  sg_actor_sleep_for(1);

  XBT_INFO("Test for completed activities");
  while (!sg_activity_set_empty(pending_activities)) {
    sg_activity_t completed_one = sg_activity_set_test_any(pending_activities);
    if (completed_one != NULL) {
      if (sg_comm_isinstance(completed_one))
        XBT_INFO("Completed a Comm");
      if (sg_exec_isinstance(completed_one))
        XBT_INFO("Completed an Exec");
      sg_activity_unref(completed_one);
    } else {
      XBT_INFO("Nothing matches, test again in 0.5s");
      sg_actor_sleep_for(.5);
    }
  }
  XBT_INFO("Last activity is complete");
  free(payload);
}

static void alice(int argc, char* argv[])
{
  char* payload = xbt_strdup("Message");
  XBT_INFO("Send '%s'", payload);
  sg_mailbox_put(sg_mailbox_by_name("mbox"), payload, 6e8);
}

int main(int argc, char* argv[])
{
  simgrid_init(&argc, argv);
  xbt_assert(argc > 1,
             "Usage: %s platform_file\n"
             "\tExample: %s hosts_with_disks.xml\n",
             argv[0], argv[0]);

  simgrid_load_platform(argv[1]);

  sg_actor_create("alice", sg_host_by_name("alice"), alice, 0, NULL);
  sg_actor_create("bob", sg_host_by_name("bob"), bob, 0, NULL);

  simgrid_run();

  return 0;
}
