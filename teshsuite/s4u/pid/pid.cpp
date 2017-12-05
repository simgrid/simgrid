/* Copyright (c) 2009-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this msg example");

static int my_onexit(smx_process_exit_status_t /*status*/, int* pid)
{
  XBT_INFO("Process \"%d\" killed.", *pid);
  return 0;
}

static void sendpid()
{
  simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName("mailbox");
  int pid                          = simgrid::s4u::this_actor::getPid();
  double comm_size                 = 100000;
  simgrid::s4u::this_actor::onExit((int_f_pvoid_pvoid_t)my_onexit, &pid);

  XBT_INFO("Sending pid of \"%d\".", pid);
  mailbox->put(&pid, comm_size);
  XBT_INFO("Send of pid \"%d\" done.", pid);

  simgrid::s4u::this_actor::suspend();
}

static void killall()
{
  simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName("mailbox");
  for (int i = 0; i < 3; i++) {
    int* pid = static_cast<int*>(mailbox->get());
    XBT_INFO("Killing process \"%d\".", *pid);
    simgrid::s4u::Actor::byPid(*pid)->kill();
  }
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.loadPlatform(argv[1]);

  if (argc > 2)
    simgrid::s4u::Actor::killAll(atoi(argv[2]));
  else
    simgrid::s4u::Actor::killAll();

  simgrid::s4u::Actor::createActor("sendpid", simgrid::s4u::Host::by_name("Tremblay"), sendpid);
  simgrid::s4u::Actor::createActor("sendpid", simgrid::s4u::Host::by_name("Tremblay"), sendpid);
  simgrid::s4u::Actor::createActor("sendpid", simgrid::s4u::Host::by_name("Tremblay"), sendpid);
  simgrid::s4u::Actor::createActor("killall", simgrid::s4u::Host::by_name("Tremblay"), killall);

  e.run();

  return 0;
}
