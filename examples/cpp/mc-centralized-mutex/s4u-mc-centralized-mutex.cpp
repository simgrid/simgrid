/* Copyright (c) 2010-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/***************** Centralized Mutual Exclusion Algorithm *********************/
/* This example implements a centralized mutual exclusion algorithm.          */
/* There is no bug on it, it is just provided to test the state space         */
/* reduction of DPOR.                                                         */
/******************************************************************************/

#include "simgrid/s4u.hpp"

#define AMOUNT_OF_CLIENTS 4
#define CS_PER_PROCESS 2

XBT_LOG_NEW_DEFAULT_CATEGORY(centralized, "my log messages");

class Message {
public:
  enum class Kind { GRANT, REQUEST, RELEASE };
  Kind kind                             = Kind::GRANT;
  simgrid::s4u::Mailbox* return_mailbox = nullptr;
  explicit Message(Message::Kind kind, simgrid::s4u::Mailbox* mbox) : kind(kind), return_mailbox(mbox) {}
};

static void coordinator()
{
  std::queue<simgrid::s4u::Mailbox*> requests;
  simgrid::s4u::Mailbox* mbox = simgrid::s4u::Mailbox::by_name("coordinator");

  bool CS_used = false;                              // initially the CS is idle
  int todo     = AMOUNT_OF_CLIENTS * CS_PER_PROCESS; // amount of releases we are expecting

  while (todo > 0) {
    auto m = mbox->get_unique<Message>();
    if (m->kind == Message::Kind::REQUEST) {
      if (CS_used) { // need to push the request in the vector
        XBT_INFO("CS already used. Queue the request");
        requests.push(m->return_mailbox);
      } else { // can serve it immediately
        XBT_INFO("CS idle. Grant immediately");
        m->return_mailbox->put(new Message(Message::Kind::GRANT, mbox), 1000);
        CS_used = true;
      }
    } else { // that's a release. Check if someone was waiting for the lock
      if (not requests.empty()) {
        XBT_INFO("CS release. Grant to queued requests (queue size: %zu)", requests.size());
        simgrid::s4u::Mailbox* req = requests.front();
        requests.pop();
        req->put(new Message(Message::Kind::GRANT, mbox), 1000);
        todo--;
      } else { // nobody wants it
        XBT_INFO("CS release. resource now idle");
        CS_used = false;
        todo--;
      }
    }
  }
  XBT_INFO("Received all releases, quit now");
}

static void client()
{
  aid_t my_pid = simgrid::s4u::this_actor::get_pid();

  simgrid::s4u::Mailbox* my_mailbox = simgrid::s4u::Mailbox::by_name(std::to_string(my_pid));

  // request the CS 3 times, sleeping a bit in between
  for (int i = 0; i < CS_PER_PROCESS; i++) {
    XBT_INFO("Ask the request");
    simgrid::s4u::Mailbox::by_name("coordinator")->put(new Message(Message::Kind::REQUEST, my_mailbox), 1000);
    // wait for the answer
    auto grant = my_mailbox->get_unique<Message>();
    XBT_INFO("got the answer. Sleep a bit and release it");
    simgrid::s4u::this_actor::sleep_for(1);

    simgrid::s4u::Mailbox::by_name("coordinator")->put(new Message(Message::Kind::RELEASE, my_mailbox), 1000);
    simgrid::s4u::this_actor::sleep_for(my_pid);
  }
  XBT_INFO("Got all the CS I wanted, quit now");
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);

  e.load_platform(argv[1]);

  simgrid::s4u::Actor::create("coordinator", e.host_by_name("Tremblay"), coordinator);
  simgrid::s4u::Actor::create("client", e.host_by_name("Fafard"), client);
  simgrid::s4u::Actor::create("client", e.host_by_name("Boivin"), client);
  simgrid::s4u::Actor::create("client", e.host_by_name("Jacquelin"), client);
  simgrid::s4u::Actor::create("client", e.host_by_name("Ginette"), client);

  e.run();

  return 0;
}
