/* Copyright (c) 2012-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "s4u-dht-kademlia.hpp"

#include "message.hpp"
#include "node.hpp"
#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(kademlia, "Messages specific for this example");
namespace sg4 = simgrid::s4u;

/** @brief Node function
  * @param my node ID
  * @param the ID of the person I know in the system (or not)
  * @param Time before I leave the system because I'm bored
  */
static void node(std::vector<std::string> args)
{
  bool join_success = true;
  double deadline;
  xbt_assert(args.size() == 3 || args.size() == 4, "Wrong number of arguments");
  /* Node initialization */
  auto node_id = static_cast<unsigned int>(std::stoul(args[1], nullptr, 0));
  kademlia::Node node(node_id);

  if (args.size() == 4) {
    XBT_INFO("Hi, I'm going to join the network with id %u", node.getId());
    auto known_id = static_cast<unsigned int>(std::stoul(args[2], nullptr, 0));
    join_success  = node.join(known_id);
    deadline      = std::stod(args[3]);
  } else {
    deadline = std::stod(args[2]);
    XBT_INFO("Hi, I'm going to create the network with id %u", node.getId());
    node.routingTableUpdate(node.getId());
  }

  if (join_success) {
    XBT_VERB("Ok, I'm joining the network with id %u", node.getId());
    // We start the main loop
    double next_lookup_time = sg4::Engine::get_clock() + RANDOM_LOOKUP_INTERVAL;

    XBT_VERB("Main loop start");

    sg4::Mailbox* mailbox = sg4::Mailbox::by_name(std::to_string(node.getId()));

    while (sg4::Engine::get_clock() < deadline) {
      if (kademlia::Message* msg = node.receive(mailbox)) {
        // There has been a message, we need to handle it !
        node.handleFindNode(msg);
        delete msg;
      } else {
        /* We search for a pseudo random node */
        if (sg4::Engine::get_clock() >= next_lookup_time) {
          node.randomLookup();
          next_lookup_time += RANDOM_LOOKUP_INTERVAL;
        } else {
          // Didn't get a message: sleep for a while...
          sg4::this_actor::sleep_for(1);
        }
      }
    }
  } else {
    XBT_INFO("I couldn't join the network :(");
  }
  XBT_DEBUG("I'm leaving the network");
  node.displaySuccessRate();
}

/** @brief Main function */
int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  /* Check the arguments */
  xbt_assert(argc > 2,
             "Usage: %s platform_file deployment_file\n\tExample: %s cluster_backbone.xml dht-kademlia_d.xml\n",
             argv[0], argv[0]);

  e.load_platform(argv[1]);
  e.register_function("node", node);
  e.load_deployment(argv[2]);

  e.run();

  XBT_INFO("Simulated time: %g", sg4::Engine::get_clock());

  return 0;
}
