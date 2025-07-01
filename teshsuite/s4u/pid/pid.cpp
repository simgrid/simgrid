/* Copyright (c) 2009-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this example");

static void sendpid()
{
  simgrid::s4u::Mailbox* mailbox = simgrid::s4u::Mailbox::by_name("mailbox");
  aid_t pid                      = simgrid::s4u::this_actor::get_pid();
  long comm_size                 = 100000;
  simgrid::s4u::this_actor::on_exit([pid](bool /*failed*/) { XBT_INFO("Process \"%ld\" killed.", pid); });

  XBT_INFO("Sending pid of \"%ld\".", pid);
  mailbox->put(&pid, comm_size);
  XBT_INFO("Send of pid \"%ld\" done.", pid);

  simgrid::s4u::this_actor::suspend();
}

static void killall()
{
  simgrid::s4u::Mailbox* mailbox = simgrid::s4u::Mailbox::by_name("mailbox");
  for (int i = 0; i < 3; i++) {
    const auto* pid = mailbox->get<aid_t>();
    XBT_INFO("Killing process \"%ld\".", *pid);
    simgrid::s4u::Actor::by_pid(*pid)->kill();
  }
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  simgrid::s4u::Actor::kill_all();
  auto tremblay =  e.host_by_name("Tremblay");
  tremblay->add_actor("sendpid", sendpid);
  tremblay->add_actor("sendpid", sendpid);
  tremblay->add_actor("sendpid", sendpid);
  tremblay->add_actor("killall", killall);

  e.run();

  return 0;
}
