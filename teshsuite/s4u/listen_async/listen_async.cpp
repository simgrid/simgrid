/* Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Bug report: https://github.com/simgrid/simgrid/issues/40
 *
 * Task.listen used to be on async mailboxes as it always returned false.
 * This occurred in Java and C, but only C remains.
 */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static void server()
{
  simgrid::s4u::Mailbox* mailbox = simgrid::s4u::Mailbox::by_name("mailbox");

  simgrid::s4u::CommPtr sendComm = mailbox->put_async(new std::string("Some data"), 0);

  xbt_assert(mailbox->listen()); // True (1)
  XBT_INFO("Task listen works on regular mailboxes");
  XBT_INFO("Mailbox::listen_from() returns %ld (should return my pid, which is %ld)", mailbox->listen_from(),
           simgrid::s4u::this_actor::get_pid());

  auto res = mailbox->get_unique<std::string>();

  xbt_assert(*res == "Some data", "Data received: %s", res->c_str());
  XBT_INFO("Data successfully received from regular mailbox");
  sendComm->wait();

  simgrid::s4u::Mailbox* mailbox2 = simgrid::s4u::Mailbox::by_name("mailbox2");
  mailbox2->set_receiver(simgrid::s4u::Actor::self());

  mailbox2->put_init(new std::string("More data"), 0)->detach();

  xbt_assert(mailbox2->listen()); // used to break.
  XBT_INFO("Task listen works on asynchronous mailboxes");

  res = mailbox2->get_unique<std::string>();
  xbt_assert(*res == "More data");

  XBT_INFO("Data successfully received from asynchronous mailbox");
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  e.host_by_name("Tremblay")->add_actor("test", server);

  e.run();

  return 0;
}
