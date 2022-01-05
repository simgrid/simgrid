/* Copyright (c) 2012-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/***************************** Bugged2 ****************************************/
/* This example implements a centralized mutual exclusion algorithm.          */
/* One client stay always in critical section                                 */
/* LTL property checked : !(GFcs)                                             */
/******************************************************************************/

#include <simgrid/modelchecker.h>
#include <simgrid/s4u.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(bugged2_liveness, "my log messages");

class Message {
public:
  enum class Kind { GRANT, NOT_GRANT, REQUEST };
  Kind kind                             = Kind::GRANT;
  simgrid::s4u::Mailbox* return_mailbox = nullptr;
  explicit Message(Message::Kind kind, simgrid::s4u::Mailbox* mbox) : kind(kind), return_mailbox(mbox) {}
};

int cs = 0;

static void coordinator()
{
  bool CS_used = false; // initially the CS is idle
  std::queue<simgrid::s4u::Mailbox*> requests;

  simgrid::s4u::Mailbox* mbox = simgrid::s4u::Mailbox::by_name("coordinator");

  while (true) {
    auto m = mbox->get_unique<Message>();
    if (m->kind == Message::Kind::REQUEST) {
      if (CS_used) {
        XBT_INFO("CS already used.");
        m->return_mailbox->put(new Message(Message::Kind::NOT_GRANT, mbox), 1000);
      } else { // can serve it immediately
        XBT_INFO("CS idle. Grant immediately");
        m->return_mailbox->put(new Message(Message::Kind::GRANT, mbox), 1000);
        CS_used = true;
      }
    } else { // that's a release. Check if someone was waiting for the lock
      XBT_INFO("CS release. resource now idle");
      CS_used = false;
    }
  }
}

static void client(int id)
{
  aid_t my_pid = simgrid::s4u::this_actor::get_pid();

  simgrid::s4u::Mailbox* my_mailbox = simgrid::s4u::Mailbox::by_name(std::to_string(id));

  while (true) {
    XBT_INFO("Client (%d) asks the request", id);
    simgrid::s4u::Mailbox::by_name("coordinator")->put(new Message(Message::Kind::REQUEST, my_mailbox), 1000);

    auto grant = my_mailbox->get_unique<Message>();

    if (grant->kind == Message::Kind::GRANT) {
      XBT_INFO("Client (%d) got the answer (grant). Sleep a bit and release it", id);
      if (id == 1)
        cs = 1;
    } else {
      XBT_INFO("Client (%d) got the answer (not grant). Try again", id);
    }

    simgrid::s4u::this_actor::sleep_for(my_pid);
  }
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);

  MC_automaton_new_propositional_symbol_pointer("cs", &cs);

  e.load_platform(argv[1]);

  simgrid::s4u::Actor::create("coordinator", e.host_by_name("Tremblay"), coordinator);
  simgrid::s4u::Actor::create("client", e.host_by_name("Fafard"), client, 1);
  simgrid::s4u::Actor::create("client", e.host_by_name("Boivin"), client, 2);

  e.run();

  return 0;
}
