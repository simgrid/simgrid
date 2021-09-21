/* Copyright (c) 2012-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "message.h"
#include "node.h"

#include "simgrid/actor.h"
#include "simgrid/comm.h"
#include "simgrid/engine.h"
#include "simgrid/mailbox.h"

#include "xbt/asserts.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(dht_kademlia, "Messages specific for this example");

/** @brief Node function
 * @param my node ID
 * @param the ID of the person I know in the system (or not)
 * @param Time before I leave the system because I'm bored
 */
static void node(int argc, char* argv[])
{
  unsigned int join_success = 1;
  double deadline;
  xbt_assert(argc == 3 || argc == 4, "Wrong number of arguments");
  /* Node initialization */
  unsigned int id = (unsigned int)strtoul(argv[1], NULL, 0);
  node_t node     = node_init(id);

  if (argc == 4) {
    XBT_INFO("Hi, I'm going to join the network with id %s", sg_mailbox_get_name(node->mailbox));
    unsigned int id_known = (unsigned int)strtoul(argv[2], NULL, 0);
    join_success          = join(node, id_known);
    deadline              = strtod(argv[3], NULL);
  } else {
    deadline = strtod(argv[2], NULL);
    XBT_INFO("Hi, I'm going to create the network with id %s", sg_mailbox_get_name(node->mailbox));
    routing_table_update(node, node->id);
  }
  if (join_success) {
    XBT_VERB("Ok, I'm joining the network with id %s", sg_mailbox_get_name(node->mailbox));
    // We start the main loop
    double next_lookup_time = simgrid_get_clock() + RANDOM_LOOKUP_INTERVAL;

    XBT_VERB("Main loop start");

    sg_mailbox_t mailbox = get_node_mailbox(node->id);

    while (simgrid_get_clock() < deadline) {
      const kademlia_message_t msg = receive(node, mailbox);
      if (msg) {
        // There has been a transfer, we need to handle it !
        handle_find_node(node, msg);
        answer_free(msg->answer);
        free(msg);
      } else {
        /* We search for a pseudo random node */
        if (simgrid_get_clock() >= next_lookup_time) {
          random_lookup(node);
          next_lookup_time += RANDOM_LOOKUP_INTERVAL;
        } else {
          // Didn't get a task: sleep for a while...
          sg_actor_sleep_for(1);
        }
      }
    }
  } else {
    XBT_INFO("I couldn't join the network :(");
  }
  if (node->receive_comm)
    sg_comm_unref(node->receive_comm);

  XBT_DEBUG("I'm leaving the network");
  XBT_INFO("%u/%u FIND_NODE have succeeded", node->find_node_success, node->find_node_success + node->find_node_failed);
  node_free(node);
}

/** @brief Main function */
int main(int argc, char* argv[])
{
  simgrid_init(&argc, argv);

  /* Check the arguments */
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n\tExample: %s platform.xml deployment.xml\n", argv[0],
             argv[0]);

  simgrid_load_platform(argv[1]);
  simgrid_register_function("node", node);
  simgrid_load_deployment(argv[2]);

  simgrid_run();

  XBT_INFO("Simulated time: %g", simgrid_get_clock());

  return 0;
}
