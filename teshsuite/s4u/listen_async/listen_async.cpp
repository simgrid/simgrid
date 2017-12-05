/* Copyright (c) 2017. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Bug report: https://github.com/simgrid/simgrid/issues/40
 *
 * Task.listen used to be on async mailboxes as it always returned false.
 * This occures in Java and C, but is only tested here in C.
 */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static void server()
{
  simgrid::s4u::MailboxPtr mailbox = simgrid::s4u::Mailbox::byName("mailbox");

  simgrid::s4u::CommPtr sendComm = mailbox->put_async(xbt_strdup("Some data"), 0);

  xbt_assert(mailbox->listen()); // True (1)
  XBT_INFO("Task listen works on regular mailboxes");
  char* res = static_cast<char*>(mailbox->get());

  xbt_assert(not strcmp("Some data", res), "Data received: %s", res);
  XBT_INFO("Data successfully received from regular mailbox");
  xbt_free(res);
  sendComm->wait();

  simgrid::s4u::MailboxPtr mailbox2 = simgrid::s4u::Mailbox::byName("mailbox2");
  mailbox2->setReceiver(simgrid::s4u::Actor::self());

  mailbox2->put_init(xbt_strdup("More data"), 0)->detach();

  xbt_assert(mailbox2->listen()); // used to break.
  XBT_INFO("Task listen works on asynchronous mailboxes");

  res = static_cast<char*>(mailbox2->get());
  xbt_assert(not strcmp("More data", res));
  xbt_free(res);

  XBT_INFO("Data successfully received from asynchronous mailbox");
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.loadPlatform(argv[1]);

  simgrid::s4u::Actor::createActor("test", simgrid::s4u::Host::by_name("Tremblay"), server);

  e.run();

  return 0;
}
