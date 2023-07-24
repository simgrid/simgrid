# Copyright (c) 2017-2023. The SimGrid Team. All rights reserved.
#
# This program is free software you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
Usage: activityset-waitall.py platform_file [other parameters]
"""

import sys
from simgrid import Actor, ActivitySet, Engine, Comm, Exec, Io, Host, Mailbox, this_actor

def bob():
  mbox = Mailbox.by_name("mbox")
  disk = Host.current().get_disks()[0]

  this_actor.info("Create my asynchronous activities")
  exec = this_actor.exec_async(5e9)
  comm = mbox.get_async()
  io   = disk.read_async(300000000)

  pending_activities = ActivitySet([exec, comm])
  pending_activities.push(io) # Activities can be pushed after creation, too
 
  this_actor.info("Wait for asynchronous activities to complete, all in one shot.")
  pending_activities.wait_all()

  this_actor.info("All activities are completed.")

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
