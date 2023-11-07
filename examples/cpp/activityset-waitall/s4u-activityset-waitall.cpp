/* Copyright (c) 2010-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_activity_waitall, "Messages specific for this s4u example");

static void bob()
{
  sg4::Mailbox* mbox    = sg4::Mailbox::by_name("mbox");
  sg4::MessageQueue* mqueue = sg4::MessageQueue::by_name("mqueue");
  const sg4::Disk* disk = sg4::Host::current()->get_disks().front();
  std::string* payload;
  std::string* message;

  XBT_INFO("Create my asynchronous activities");
  auto exec = sg4::this_actor::exec_async(5e9);
  auto comm = mbox->get_async(&payload);
  auto io   = disk->read_async(3e8);
  auto mess = mqueue->get_async(&message);

  sg4::ActivitySet pending_activities({exec, comm, io, mess});

  XBT_INFO("Wait for asynchronous activities to complete, all in one shot.");
  pending_activities.wait_all();

  XBT_INFO("All activities are completed.");
  delete payload;
  delete message;
}

static void alice()
{
  auto* payload = new std::string("Message");
  XBT_INFO("Send '%s'", payload->c_str());
  sg4::Mailbox::by_name("mbox")->put(payload, 6e8);
}

static void carl()
{
  auto* payload = new std::string("Control Message");
  sg4::MessageQueue::by_name("mqueue")->put(payload);
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  e.load_platform(argv[1]);

  sg4::Actor::create("bob", e.host_by_name("bob"), bob);
  sg4::Actor::create("alice", e.host_by_name("alice"), alice);
  sg4::Actor::create("carl", e.host_by_name("carl"), carl);

  e.run();

  return 0;
}
