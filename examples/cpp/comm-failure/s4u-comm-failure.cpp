/* Copyright (c) 2021-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to react to a failed communication, which occurs when a link is turned off,
 * or when the actor with whom you communicate fails because its host is turned off.
 */

#include <simgrid/s4u.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_comm_failure, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

class Sender {
  std::string mailbox1_name;
  std::string mailbox2_name;

public:
  Sender(const std::string& mailbox1_name, const std::string& mailbox2_name)
      : mailbox1_name(mailbox1_name), mailbox2_name(mailbox2_name)
  {
  }

  void operator()() const
  {
    auto* mailbox1 = sg4::Mailbox::by_name(mailbox1_name);
    auto* mailbox2 = sg4::Mailbox::by_name(mailbox2_name);

    XBT_INFO("Initiating asynchronous send to %s", mailbox1->get_cname());
    auto comm1 = mailbox1->put_async((void*)666, 5);
    XBT_INFO("Initiating asynchronous send to %s", mailbox2->get_cname());
    auto comm2 = mailbox2->put_async((void*)666, 2);

    XBT_INFO("Calling wait_any..");
    sg4::ActivitySet pending_comms;
    pending_comms.push(comm1);
    pending_comms.push(comm2);
    try {
      auto* acti = pending_comms.wait_any().get();
      XBT_INFO("Wait any returned comm to %s", dynamic_cast<sg4::Comm*>(acti)->get_mailbox()->get_cname());
    } catch (const simgrid::NetworkFailureException&) {
      XBT_INFO("Sender has experienced a network failure exception, so it knows that something went wrong");
      XBT_INFO("Now it needs to figure out which of the two comms failed by looking at their state:");
      XBT_INFO("  Comm to %s has state: %s", comm1->get_mailbox()->get_cname(), comm1->get_state_str());
      XBT_INFO("  Comm to %s has state: %s", comm2->get_mailbox()->get_cname(), comm2->get_state_str());
    }

    try {
      comm1->wait();
    } catch (const simgrid::NetworkFailureException& e) {
      XBT_INFO("Waiting on a FAILED comm raises an exception: '%s'", e.what());
    }
    XBT_INFO("Wait for remaining comm, just to be nice");
    pending_comms.wait_all();
  }
};

class Receiver {
  sg4::Mailbox* mailbox;

public:
  explicit Receiver(const std::string& mailbox_name) : mailbox(sg4::Mailbox::by_name(mailbox_name)) {}

  void operator()() const
  {
    XBT_INFO("Receiver posting a receive...");
    try {
      mailbox->get<void*>();
      XBT_INFO("Receiver has received successfully!");
    } catch (const simgrid::NetworkFailureException&) {
      XBT_INFO("Receiver has experience a network failure exception");
    }
  }
};

int main(int argc, char** argv)
{
  sg4::Engine engine(&argc, argv);
  auto* zone  = sg4::create_full_zone("AS0");
  auto* host1 = zone->create_host("Host1", "1f");
  auto* host2 = zone->create_host("Host2", "1f");
  auto* host3 = zone->create_host("Host3", "1f");
  auto* link2 = zone->create_link("linkto2", "1bps")->seal();
  auto* link3 = zone->create_link("linkto3", "1bps")->seal();

  zone->add_route(host1, host2, {link2});
  zone->add_route(host1, host3, {link3});
  zone->seal();

  sg4::Actor::create("Sender", host1, Sender("mailbox2", "mailbox3"));
  sg4::Actor::create("Receiver", host2, Receiver("mailbox2"));
  sg4::Actor::create("Receiver", host3, Receiver("mailbox3"));

  sg4::Actor::create("LinkKiller", host1, [](){
    sg4::this_actor::sleep_for(10.0);
    XBT_INFO("Turning off link 'linkto2'");
    sg4::Link::by_name("linkto2")->turn_off();
  });

  engine.run();

  return 0;
}
