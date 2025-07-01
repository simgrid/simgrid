/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include <iostream>
#include <iomanip>

XBT_LOG_NEW_DEFAULT_CATEGORY(ns3_wifi_example, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

class Message
{
public:
  std::string sender;
  int size;

  Message(const std::string& sender_, int size_) : sender(sender_), size(size_) {}
};

static void sender(const std::string& mailbox, int msg_size, unsigned sleep_time)
{
  sg4::this_actor::sleep_for(sleep_time);
  auto* mbox = sg4::Mailbox::by_name(mailbox);
  auto* msg  = new Message(sg4::this_actor::get_host()->get_name(), msg_size);
  mbox->put(msg, msg_size);
}

static void receiver(const std::string& mailbox)
{
  auto* mbox = sg4::Mailbox::by_name(mailbox);
  auto msg   = mbox->get_unique<Message>();
  XBT_INFO("[%s] %s received %d bytes from %s", mailbox.c_str(), sg4::this_actor::get_host()->get_name().c_str(),
           msg->size, msg->sender.c_str());
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  e.load_platform(argv[1]);
  int msg_size = 1e5;

  /* Communication between STA in the same wifi zone */
  e.host_by_name("STA0-0")->add_actor("sender", sender, "1", msg_size, 10);
  e.host_by_name("STA0-1")->add_actor("receiver",receiver, "1");
  e.host_by_name("STA0-1")->add_actor("sender", sender, "2", msg_size, 20);
  e.host_by_name("STA0-0")->add_actor("receiver", receiver, "2");
  e.host_by_name("STA1-1")->add_actor("sender", sender, "3", msg_size, 30);
  e.host_by_name("STA1-2")->add_actor("receiver", receiver, "3");
  e.host_by_name("STA1-2")->add_actor("sender", sender, "4", msg_size, 40);
  e.host_by_name("STA1-1")->add_actor("receiver", receiver, "4");

  /* Communication between STA of different wifi zones */
  e.host_by_name("STA0-0")->add_actor("sender", sender, "5", msg_size, 50);
  e.host_by_name("STA1-0")->add_actor("receiver", receiver, "5");
  e.host_by_name("STA1-0")->add_actor("sender", sender, "6", msg_size, 60);
  e.host_by_name("STA0-0")->add_actor("receiver", receiver, "6");
  e.host_by_name("STA0-1")->add_actor("sender", sender, "7", msg_size, 70);
  e.host_by_name("STA1-2")->add_actor("receiver", receiver, "7");
  e.host_by_name("STA1-2")->add_actor("sender", sender, "8", msg_size, 80);
  e.host_by_name("STA0-1")->add_actor("receiver", receiver, "8");

  e.run();
  return 0;
}
