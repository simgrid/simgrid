/* Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/energy.h"
#include "xbt/log.h"
#include "xbt/random.hpp"
#include <simgrid/s4u.hpp>

/* Parameters of the random generation of the flow size */
static const int min_size = 1e6;
static const int max_size = 1e9;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_app_energyconsumption, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

static void sender(std::vector<std::string> args)
{
  xbt_assert(args.size() == 2, "The master function expects 2 arguments.");
  int flow_amount = std::stoi(args.at(0));
  long comm_size  = std::stol(args.at(1));
  XBT_INFO("Send %ld bytes, in %d flows", comm_size, flow_amount);

  sg4::Mailbox* mailbox = sg4::Mailbox::by_name("message");

  /* Sleep a while before starting the example */
  sg4::this_actor::sleep_for(10);

  if (flow_amount == 1) {
    /* - Send the task to the @ref worker */
    char* payload = bprintf("%ld", comm_size);
    mailbox->put(payload, comm_size);
  } else {
    // Start all comms in parallel, and wait for all completions in one shot
    sg4::ActivitySet comms;
    for (int i = 0; i < flow_amount; i++)
      comms.push(mailbox->put_async(bprintf("%d", i), comm_size));
    comms.wait_all();
  }
  XBT_INFO("sender done.");
}

static void receiver(std::vector<std::string> args)
{
  int flow_amount = std::stoi(args.at(0));

  XBT_INFO("Receiving %d flows ...", flow_amount);

  sg4::Mailbox* mailbox = sg4::Mailbox::by_name("message");

  if (flow_amount == 1) {
    char* res = mailbox->get<char>();
    xbt_free(res);
  } else {
    std::vector<char*> data(flow_amount);

    // Start all comms in parallel, and wait for their completion in one shot
    sg4::ActivitySet comms;
    for (int i = 0; i < flow_amount; i++)
      comms.push(mailbox->get_async<char>(&data[i]));

    comms.wait_all();
    for (int i = 0; i < flow_amount; i++)
      xbt_free(data[i]);
  }
  XBT_INFO("receiver done.");
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  XBT_INFO("Activating the SimGrid link energy plugin");
  sg_link_energy_plugin_init();

  xbt_assert(argc > 1, "\nUsage: %s platform_file [flowCount [datasize]]\n"
                       "\tExample: %s s4uplatform.xml \n",
             argv[0], argv[0]);
  e.load_platform(argv[1]);

  /* prepare to launch the actors */
  std::vector<std::string> argSender;
  std::vector<std::string> argReceiver;
  if (argc > 2) {
    argSender.emplace_back(argv[2]); // Take the amount of flows from the command line
    argReceiver.emplace_back(argv[2]);
  } else {
    argSender.emplace_back("1"); // Default value
    argReceiver.emplace_back("1");
  }

  if (argc > 3) {
    if (strcmp(argv[3], "random") == 0) { // We're asked to get a random size
      std::string size = std::to_string(simgrid::xbt::random::uniform_int(min_size, max_size));
      argSender.push_back(size);
    } else {                        // Not "random" ? Then it should be the size to use
      argSender.emplace_back(argv[3]); // Take the datasize from the command line
    }
  } else { // No parameter at all? Then use the default value
    argSender.emplace_back("25000");
  }
  e.host_by_name("MyHost1")->add_actor("sender", sender, argSender);
  e.host_by_name("MyHost2")->add_actor("receiver", receiver, argReceiver);

  /* And now, launch the simulation */
  e.run();

  return 0;
}
