/* Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include <cstdlib>
#include <iostream>
#include <string>

namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_activity_testany, "Messages specific for this s4u example");

static void bob()
{
  sg4::Mailbox* mbox        = sg4::Mailbox::by_name("mbox");
  sg4::MessageQueue* mqueue = sg4::MessageQueue::by_name("mqueue");
  const sg4::Disk* disk     = sg4::Host::current()->get_disks().front();
  std::string* payload;
  std::string* message;

  XBT_INFO("Create my asynchronous activities");
  auto exec = sg4::this_actor::exec_async(5e9);
  auto comm = mbox->get_async(&payload);
  auto mess = mqueue->get_async(&message);
  auto io   = disk->read_async(3e8);

  sg4::ActivitySet pending_activities({exec, comm, mess, io});
  XBT_INFO("Sleep_for a while");
  sg4::this_actor::sleep_for(1);

  XBT_INFO("Test for completed activities");
  while (not pending_activities.empty()) {
    auto completed_one = pending_activities.test_any();
    if (completed_one != nullptr) {
      if (boost::dynamic_pointer_cast<sg4::Comm>(completed_one))
        XBT_INFO("Completed a Comm");
      if (boost::dynamic_pointer_cast<sg4::Mess>(completed_one))
        XBT_INFO("Completed a Mess");
      if (boost::dynamic_pointer_cast<sg4::Exec>(completed_one))
        XBT_INFO("Completed an Exec");
      if (boost::dynamic_pointer_cast<sg4::Io>(completed_one))
        XBT_INFO("Completed an I/O");
    } else {
      XBT_INFO("Nothing matches, test again in 0.5s");
      sg4::this_actor::sleep_for(.5);
    }
  }
  XBT_INFO("Last activity is complete");
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
  sg4::this_actor::sleep_for(1.99);
  auto* payload = new std::string("Control Message");
  XBT_INFO("Send '%s'", payload->c_str());
  sg4::MessageQueue::by_name("mqueue")->put(payload);
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  e.load_platform(argv[1]);

  e.host_by_name("bob")->add_actor("bob", bob);
  e.host_by_name("alice")->add_actor("alice", alice);
  e.host_by_name("carl")->add_actor("carl", carl);

  e.run();

  return 0;
}
