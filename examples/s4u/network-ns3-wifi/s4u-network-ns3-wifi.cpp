/* Copyright (c) 2007-2020. The SimGrid Team. LEVEL_ALL rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include <iostream>
#include <iomanip>

XBT_LOG_NEW_DEFAULT_CATEGORY(ns3_wifi_example, "Messages specific for this s4u example");

double start_time;

class Message
{
    public:
    std::string sender;
    int size;

    Message(std::string sender_, int size_) : sender(sender_), size(size_){}
};

static void sender(std::string mailbox, double msg_size, unsigned sleep_time)
{
  simgrid::s4u::this_actor::sleep_for(sleep_time);
  simgrid::s4u::Mailbox* mbox = simgrid::s4u::Mailbox::by_name(mailbox);
  Message* msg = new Message(simgrid::s4u::this_actor::get_host()->get_name(), msg_size);
  start_time = simgrid::s4u::Engine::get_clock();
  mbox->put(msg, msg_size);
}

static void receiver(std::string mailbox)
{
  simgrid::s4u::Mailbox* mbox = simgrid::s4u::Mailbox::by_name(mailbox);
  Message* msg = (Message*) mbox->get();
  double elapsed_time = simgrid::s4u::Engine::get_clock() - start_time;
  XBT_INFO("[%s] %s received %d bytes from %s      Communication time: %f seconds      Throughput: %f Mbps",
          mailbox.c_str(), simgrid::s4u::this_actor::get_host()->get_name().c_str(), msg->size,
          msg->sender.c_str(), elapsed_time, msg->size * 8 / elapsed_time / 1E6);
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
  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name("STA1-0"), sender, "3", msg_size, 30);
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name("STA1-1"), receiver, "3");
  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name("STA1-1"), sender, "4", msg_size, 40);
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name("STA1-0"), receiver, "4");
  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name("STA1-0"), sender, "5", msg_size, 50);
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name("STA1-2"), receiver, "5");
  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name("STA1-0"), sender, "6", msg_size, 60);
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name("STA1-2"), receiver, "6");
  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name("STA1-1"), sender, "7", msg_size, 70);
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name("STA1-2"), receiver, "7");
  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name("STA1-2"), sender, "8", msg_size, 80);
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name("STA1-1"), receiver, "8");

  /* Communication between STA of different wifi zones */
  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name("STA0-0"), sender, "9", msg_size, 90);
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name("STA1-0"), receiver, "9");
  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name("STA1-0"), sender, "10", msg_size, 100);
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name("STA0-0"), receiver, "10");
  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name("STA0-0"), sender, "11", msg_size, 110);
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name("STA1-1"), receiver, "11");
  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name("STA1-1"), sender, "12", msg_size, 120);
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name("STA0-0"), receiver, "12");
  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name("STA0-0"), sender, "13", msg_size, 130);
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name("STA1-2"), receiver, "13");
  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name("STA1-2"), sender, "14", msg_size, 140);
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name("STA0-0"), receiver, "14");
  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name("STA0-1"), sender, "15", msg_size, 150);
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name("STA1-0"), receiver, "15");
  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name("STA1-0"), sender, "16", msg_size, 160);
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name("STA0-1"), receiver, "16");
  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name("STA0-1"), sender, "17", msg_size, 170);
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name("STA1-1"), receiver, "17");
  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name("STA1-1"), sender, "18", msg_size, 180);
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name("STA0-1"), receiver, "18");
  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name("STA0-1"), sender, "19", msg_size, 190);
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name("STA1-2"), receiver, "19");
  simgrid::s4u::Actor::create("sender", simgrid::s4u::Host::by_name("STA1-2"), sender, "20", msg_size, 200);
  simgrid::s4u::Actor::create("receiver", simgrid::s4u::Host::by_name("STA0-1"), receiver, "20");

  e.run();
  return 0;
}
