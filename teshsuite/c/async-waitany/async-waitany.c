/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/comm.h"
#include "simgrid/engine.h"
#include "simgrid/forward.h"
#include "simgrid/mailbox.h"
#include "xbt/asserts.h"
#include "xbt/log.h"
#include "xbt/str.h"

#include <stdio.h> /* snprintf */

XBT_LOG_NEW_DEFAULT_CATEGORY(async_waitany, "Messages specific for this example");

static int sender(int argc, char* argv[])
{
  xbt_assert(argc == 4, "Expecting 3 parameters from the XML deployment file but got %d", argc);
  long messages_count  = xbt_str_parse_int(argv[1], "Invalid amount of tasks: %s");
  long msg_size        = xbt_str_parse_int(argv[2], "Invalid communication size: %s");
  long receivers_count = xbt_str_parse_int(argv[3], "Invalid amount of receivers: %s");

  /* Dynar in which we store all ongoing communications */
  xbt_dynar_t pending_comms = xbt_dynar_new(sizeof(sg_comm_t), NULL);
  ;

  /* Make a dynar of the mailboxes to use */
  xbt_dynar_t mboxes = xbt_dynar_new(sizeof(sg_mailbox_t), NULL);
  for (long i = 0; i < receivers_count; i++) {
    char mailbox_name[80];
    snprintf(mailbox_name, 79, "receiver-%ld", (i));
    xbt_dynar_push_as(mboxes, sg_mailbox_t, sg_mailbox_by_name(mailbox_name));
  }

  /* Start dispatching all messages to receivers, in a round robin fashion */
  for (int i = 0; i < messages_count; i++) {
    char msg_content[80];
    snprintf(msg_content, 79, "Message_%d", i);
    sg_mailbox_t mbox = (sg_mailbox_t)xbt_dynar_get_ptr(mboxes, i % receivers_count);

    XBT_INFO("Send '%s' to '%s'", msg_content, sg_mailbox_get_name(mbox));

    sg_comm_t comm = sg_mailbox_put_async(mbox, xbt_strdup(msg_content), msg_size);
    xbt_dynar_push_as(pending_comms, sg_comm_t, comm);
  }
  /* Start sending messages to let the workers know that they should stop */
  for (int i = 0; i < receivers_count; i++) {
    XBT_INFO("Send 'finalize' to 'receiver-%d'", i);
    char* end_msg  = xbt_strdup("finalize");
    sg_comm_t comm = sg_mailbox_put_async((sg_mailbox_t)xbt_dynar_get_ptr(mboxes, i % receivers_count), end_msg, 0);
    xbt_dynar_push_as(pending_comms, sg_comm_t, comm);
    xbt_free(end_msg);
  }

  XBT_INFO("Done dispatching all messages");

  /* Now that all message exchanges were initiated, wait for their completion, in order of termination.
   *
   * This loop waits for first terminating message with wait_any() and remove it with erase(), until all comms are
   * terminated
   * Even in this simple example, the pending comms do not terminate in the exact same order of creation.
   */
  while (!xbt_dynar_is_empty(pending_comms)) {
    int changed_pos = sg_comm_wait_any_for(pending_comms, -1);
    xbt_dynar_remove_at(pending_comms, changed_pos, NULL);
    if (changed_pos != 0)
      XBT_INFO("Remove the %dth pending comm: it terminated earlier than another comm that was initiated first.",
               changed_pos);
  }

  xbt_dynar_free(&pending_comms);
  xbt_dynar_free(&mboxes);

  XBT_INFO("Goodbye now!");
  return 0;
}

static int receiver(int argc, char* argv[])
{
  xbt_assert(argc == 2, "Expecting one parameter from the XML deployment file but got %d", argc);
  int id = xbt_str_parse_int(argv[1], "ID should be numerical, not %s");
  char mailbox_name[80];
  snprintf(mailbox_name, 79, "receiver-%d", id);
  sg_mailbox_t mbox = sg_mailbox_by_name(mailbox_name);
  XBT_INFO("Wait for my first message on '%s'", mailbox_name);
  while (1) {
    char* received = (char*)sg_mailbox_get(mbox);
    XBT_INFO("I got a '%s'.", received);
    if (!strcmp(received, "finalize")) { // If it's a finalize message, we're done
      xbt_free(received);
      break;
    }
    xbt_free(received);
  }

  XBT_INFO("I'm done. See you!");
  return 0;
}

int main(int argc, char* argv[])
{
  simgrid_init(&argc, argv);
  xbt_assert(argc > 2,
             "Usage: %s platform_file deployment_file\n"
             "\tExample: %s msg_platform.xml msg_deployment.xml\n",
             argv[0], argv[0]);

  simgrid_load_platform(argv[1]);

  simgrid_register_function("sender", sender);
  simgrid_register_function("receiver", receiver);
  simgrid_load_deployment(argv[2]);

  simgrid_run();

  XBT_INFO("Simulation time %g", simgrid_get_clock());

  return 0;
}
