/* Copyright (c) 2007-2020. The SimGrid Team. LEVEL_ALL rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include <iostream>
#include <iomanip>

XBT_LOG_NEW_DEFAULT_CATEGORY(ns3_wifi_example, "Messages specific for this s4u example");

class Message
{
public:
  std::string sender;
  int size;

  Message(std::string sender_, int size_) : sender(sender_), size(size_) {}
};

static void sender(std::string mailbox, double msg_size, unsigned sleep_time)
{
  simgrid::s4u::this_actor::sleep_for(sleep_time);
  auto* mbox = simgrid::s4u::Mailbox::by_name(mailbox);
  auto* msg  = new Message(simgrid::s4u::this_actor::get_host()->get_name(), msg_size);
  mbox->put(msg, msg_size);
}

static void receiver(std::string mailbox)
{
  auto* mbox = simgrid::s4u::Mailbox::by_name(mailbox);
  auto msg   = std::unique_ptr<Message>(mbox->get<Message>());
  XBT_INFO("[%s] %s received %d bytes from %s",
           mailbox.c_str(),
           simgrid::s4u::this_actor::get_host()->get_name().c_str(),
           msg->size,
           msg->sender.c_str());
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);

  e.load_platform(argv[1]);
  double msg_size = 1E5;

  /* Communication between STA in the same wifi zone */    
  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name("STA0-0"), sender, "1", msg_size, 10);
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name("STA0-1"), receiver, "1");
  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name("STA0-1"), sender, "2", msg_size, 20);
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name("STA0-0"), receiver, "2");
  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name("STA1-1"), sender, "3", msg_size, 30);
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name("STA1-2"), receiver, "3");
  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name("STA1-2"), sender, "4", msg_size, 40);
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name("STA1-1"), receiver, "4");

  /* Communication between STA of different wifi zones */
  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name("STA0-0"), sender, "5", msg_size, 50);
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name("STA1-0"), receiver, "5");
  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name("STA1-0"), sender, "6", msg_size, 60);
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name("STA0-0"), receiver, "6");
  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name("STA0-1"), sender, "7", msg_size, 70);
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name("STA1-2"), receiver, "7");
  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name("STA1-2"), sender, "8", msg_size, 80);
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name("STA0-1"), receiver, "8");

  e.run();
  return 0;
}
