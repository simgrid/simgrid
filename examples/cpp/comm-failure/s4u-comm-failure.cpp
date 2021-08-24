/* Copyright (c) 2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to serialize a set of communications going through a link
 *
 * As for the other asynchronous examples, the sender initiates all the messages it wants to send and
 * pack the resulting simgrid::s4u::CommPtr objects in a vector.
 * At the same time, the receiver starts receiving all messages asynchronously. Without serialization,
 * all messages would be received at the same timestamp in the receiver.
 *
 * However, as they will be serialized in a link of the platform, the messages arrive 2 by 2.
 *
 * The sender then blocks until all ongoing communication terminate, using simgrid::s4u::Comm::wait_all()
 */

#include <simgrid/s4u.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_comm_failure, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

class Sender {
  std::string mailbox1_name;
  std::string mailbox2_name;

public:
  Sender(std::string mailbox1_name, std::string mailbox2_name)
      : mailbox1_name(mailbox1_name), mailbox2_name(mailbox2_name)
  {
  }

  void operator()()
  {
    auto mailbox1 = sg4::Mailbox::by_name(mailbox1_name);
    auto mailbox2 = sg4::Mailbox::by_name(mailbox2_name);

    XBT_INFO("Initiating asynchronous send to %s", mailbox1->get_cname());
    auto comm1 = mailbox1->put_async((void*)666, 5);
    XBT_INFO("Initiating asynchronous send to %s", mailbox2->get_cname());
    auto comm2 = mailbox2->put_async((void*)666, 2);

    XBT_INFO("Calling wait_any..");
    std::vector<sg4::CommPtr> pending_comms;
    pending_comms.push_back(comm1);
    pending_comms.push_back(comm2);
    try {
      long index = sg4::Comm::wait_any(pending_comms);
      XBT_INFO("Wait any returned index %ld (comm to %s)", index, pending_comms.at(index)->get_mailbox()->get_cname());
    } catch (simgrid::NetworkFailureException& e) {
      XBT_INFO("Sender has experienced a network failure exception, so it knows that something went wrong");
      XBT_INFO("Now it needs to figure out which of the two comms failed by looking at their state");
    }

    XBT_INFO("Comm to %s has state: %s", comm1->get_mailbox()->get_cname(), comm1->get_state_str());
    XBT_INFO("Comm to %s has state: %s", comm2->get_mailbox()->get_cname(), comm2->get_state_str());

    try {
      comm1->wait();
    } catch (simgrid::NetworkFailureException& e) {
      XBT_INFO("Waiting on a FAILED comm raises an exception: '%s'", e.what());
    }
    XBT_INFO("Wait for remaining comm, just to be nice");
    pending_comms.erase(pending_comms.begin());
    simgrid::s4u::Comm::wait_any(pending_comms);
  }
};

class Receiver {
  std::string mailbox_name;

public:
  explicit Receiver(std::string mailbox_name) : mailbox_name(mailbox_name) {}

  void operator()()
  {
    auto mailbox = sg4::Mailbox::by_name(mailbox_name);
    XBT_INFO("Receiver posting a receive...");
    try {
      mailbox->get<void*>();
      XBT_INFO("Receiver has received successfully!");
    } catch (simgrid::NetworkFailureException& e) {
      XBT_INFO("Receiver has experience a network failure exception");
    }
  }
};

class LinkKiller {
  std::string link_name;

public:
  explicit LinkKiller(std::string link_name) : link_name(link_name) {}

  void operator()()
  {
    auto link_to_kill = sg4::Link::by_name(link_name);
    XBT_INFO("LinkKiller  sleeping 10 seconds...");
    sg4::this_actor::sleep_for(10.0);
    XBT_INFO("LinkKiller turning off link %s", link_to_kill->get_cname());
    link_to_kill->turn_off();
    XBT_INFO("LinkKiller killed. exiting");
  }
};

int main(int argc, char** argv)
{

  sg4::Engine engine(&argc, argv);
  auto* zone  = sg4::create_full_zone("AS0");
  auto* host1 = zone->create_host("Host1", "1f");
  auto* host2 = zone->create_host("Host2", "1f");
  auto* host3 = zone->create_host("Host3", "1f");

  sg4::LinkInRoute linkto2{zone->create_link("linkto2", "1bps")->seal()};
  sg4::LinkInRoute linkto3{zone->create_link("linkto3", "1bps")->seal()};

  zone->add_route(host1->get_netpoint(), host2->get_netpoint(), nullptr, nullptr, {linkto2}, false);
  zone->add_route(host1->get_netpoint(), host3->get_netpoint(), nullptr, nullptr, {linkto3}, false);
  zone->seal();

  sg4::Actor::create("Sender", host1, Sender("mailbox2", "mailbox3"));
  sg4::Actor::create("Receiver", host2, Receiver("mailbox2"))->daemonize();
  sg4::Actor::create("Receiver", host3, Receiver("mailbox3"))->daemonize();
  sg4::Actor::create("LinkKiller", host1, LinkKiller("linkto2"))->daemonize();

  engine.run();

  return 0;
}
