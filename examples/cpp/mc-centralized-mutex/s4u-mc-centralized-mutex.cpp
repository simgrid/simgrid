/* Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.            */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package.   */

/***************** Centralized Mutual Exclusion Algorithm *********************/
/* This example implements a centralized mutual exclusion algorithm.          */
/* There is no bug on it, it is just provided to test the state space         */
/* reduction of DPOR.                                                         */
/******************************************************************************/

#include "simgrid/s4u.hpp"

constexpr int AMOUNT_OF_CLIENTS = 4;
constexpr int CS_PER_PROCESS    = 2;

XBT_LOG_NEW_DEFAULT_CATEGORY(centralized, "my log messages");
namespace sg4 = simgrid::s4u;

class Message {
public:
  enum class Kind { GRANT, REQUEST, RELEASE };
  Kind kind                             = Kind::GRANT;
  sg4::Mailbox* return_mailbox          = nullptr;
  explicit Message(Message::Kind kind, sg4::Mailbox* mbox) : kind(kind), return_mailbox(mbox) {}
};

static void coordinator()
{
  std::queue<sg4::Mailbox*> requests;
  sg4::Mailbox* mbox = sg4::Mailbox::by_name("coordinator");

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
        sg4::Mailbox* req = requests.front();
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
  aid_t my_pid = sg4::this_actor::get_pid();

  sg4::Mailbox* my_mailbox = sg4::Mailbox::by_name(std::to_string(my_pid));

  // request the CS 3 times, sleeping a bit in between
  for (int i = 0; i < CS_PER_PROCESS; i++) {
    XBT_INFO("Ask the request");
    sg4::Mailbox::by_name("coordinator")->put(new Message(Message::Kind::REQUEST, my_mailbox), 1000);
    // wait for the answer
    auto grant = my_mailbox->get_unique<Message>();
    XBT_INFO("got the answer. Sleep a bit and release it");
    sg4::this_actor::sleep_for(1);

    sg4::Mailbox::by_name("coordinator")->put(new Message(Message::Kind::RELEASE, my_mailbox), 1000);
    sg4::this_actor::sleep_for(static_cast<double>(my_pid));
  }
  XBT_INFO("Got all the CS I wanted, quit now");
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  e.load_platform(argv[1]);

  sg4::Actor::create("coordinator", e.host_by_name("Tremblay"), coordinator);
  sg4::Actor::create("client", e.host_by_name("Fafard"), client);
  sg4::Actor::create("client", e.host_by_name("Boivin"), client);
  sg4::Actor::create("client", e.host_by_name("Jacquelin"), client);
  sg4::Actor::create("client", e.host_by_name("Ginette"), client);

  e.run();

  return 0;
}
