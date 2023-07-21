# Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.
#
# This program is free software you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
Usage: activityset-waitany.py platform_file [other parameters]
"""

import sys
from simgrid import Actor, ActivitySet, Engine, Comm, Exec, Io, Host, Mailbox, this_actor

def bob():
  mbox = Mailbox.by_name("mbox")
  disk = Host.current().get_disks()[0]

  this_actor.info("Create my asynchronous activities")
  exec = this_actor.exec_async(5e9)
  comm, payload = mbox.get_async()
  io   = disk.read_async(300000000)

  pending_activities = ActivitySet([exec, comm])
  pending_activities.push(io) # Activities can be pushed after creation, too
 
  this_actor.info("Wait for asynchronous activities to complete")
  while not pending_activities.empty():
    completed_one = pending_activities.wait_any()

    if isinstance(completed_one, Comm):
      this_actor.info("Completed a Comm")
    elif isinstance(completed_one, Exec):
      this_actor.info("Completed an Exec")
    elif isinstance(completed_one, Io):
      this_actor.info("Completed an I/O")

  this_actor.info("Last activity is complete")

def alice():
  this_actor.info("Send 'Message'")
  Mailbox.by_name("mbox").put("Message", 600000000)

if __name__ == '__main__':
  e = Engine(sys.argv)
  e.set_log_control("root.fmt:[%4.6r]%e[%5a]%e%m%n")

  # Load the platform description
  e.load_platform(sys.argv[1])

  Actor.create("bob",   Host.by_name("bob"), bob)
  Actor.create("alice", Host.by_name("alice"), alice)

  e.run()

"""

/* Copyright (c) 2010-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_activity_waittany, "Messages specific for this s4u example");

static void bob()
{
  sg4::Mailbox* mbox    = sg4::Mailbox::by_name("mbox");
  const sg4::Disk* disk = sg4::Host::current()->get_disks().front();
  std::string* payload;

  XBT_INFO("Create my asynchronous activities");
  auto exec = sg4::this_actor::exec_async(5e9);
  auto comm = mbox->get_async(&payload);
  auto io   = disk->read_async(3e8);

  sg4::ActivitySet pending_activities({boost::dynamic_pointer_cast<sg4::Activity>(exec),
                                       boost::dynamic_pointer_cast<sg4::Activity>(comm),
                                       boost::dynamic_pointer_cast<sg4::Activity>(io)});

  XBT_INFO("Wait for asynchronous activities to complete");
  while (not pending_activities.empty()) {
    auto completed_one = pending_activities.wait_any();
    if (completed_one != nullptr) {
      if (boost::dynamic_pointer_cast<sg4::Comm>(completed_one))
        XBT_INFO("Completed a Comm");
      if (boost::dynamic_pointer_cast<sg4::Exec>(completed_one))
        XBT_INFO("Completed an Exec");
      if (boost::dynamic_pointer_cast<sg4::Io>(completed_one))
        XBT_INFO("Completed an I/O");
    }
  }
  XBT_INFO("Last activity is complete");
  delete payload;
}

static void alice()
{
  auto* payload = new std::string("Message");
  XBT_INFO("Send '%s'", payload->c_str());
  sg4::Mailbox::by_name("mbox")->put(payload, 6e8);
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  e.load_platform(argv[1]);

  sg4::Actor::create("bob", e.host_by_name("bob"), bob);
  sg4::Actor::create("alice", e.host_by_name("alice"), alice);

  e.run();

  return 0;
}
"""