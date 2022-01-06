/* Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.          */

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

#include "simgrid/s4u.hpp"
#include <string>
namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_async_serialize, "Messages specific for this s4u example");

class Sender {
  int messages_count; /* - number of messages */
  int msg_size;       /* - message size in bytes */

public:
  explicit Sender(int size, int count) : messages_count(count), msg_size(size) {}
  void operator()() const
  {
    // sphinx-doc: init-begin (this line helps the doc to build; ignore it)
    /* Vector in which we store all ongoing communications */
    std::vector<sg4::CommPtr> pending_comms;

    /* Make a vector of the mailboxes to use */
    sg4::Mailbox* mbox = sg4::Mailbox::by_name("receiver");
    // sphinx-doc: init-end

    /* Start dispatching all messages to receiver */
    for (int i = 0; i < messages_count; i++) {
      std::string msg_content = std::string("Message ") + std::to_string(i);
      // Copy the data we send: the 'msg_content' variable is not a stable storage location.
      // It will be destroyed when this actor leaves the loop, ie before the receiver gets it
      auto* payload = new std::string(msg_content);

      XBT_INFO("Send '%s' to '%s'", msg_content.c_str(), mbox->get_cname());

      /* Create a communication representing the ongoing communication, and store it in pending_comms */
      sg4::CommPtr comm = mbox->put_async(payload, msg_size);
      pending_comms.push_back(comm);
    }

    XBT_INFO("Done dispatching all messages");

    /* Now that all message exchanges were initiated, wait for their completion in one single call */
    sg4::Comm::wait_all(pending_comms);
    // sphinx-doc: put-end

    XBT_INFO("Goodbye now!");
  }
};

/* Receiver actor expects 1 argument: number of messages to be received */
class Receiver {
  sg4::Mailbox* mbox;
  int messages_count = 10; /* - number of messages */

public:
  explicit Receiver(int count) : messages_count(count) { mbox = sg4::Mailbox::by_name("receiver"); }
  void operator()()
  {
    /* Vector in which we store all incoming msgs */
    std::vector<std::unique_ptr<std::string*>> pending_msgs;
    std::vector<sg4::CommPtr> pending_comms;

    XBT_INFO("Wait for %d messages asynchronously", messages_count);
    for (int i = 0; i < messages_count; i++) {
      pending_msgs.push_back(std::make_unique<std::string*>());
      pending_comms.emplace_back(mbox->get_async<std::string>(pending_msgs[i].get()));
    }
    while (not pending_comms.empty()) {
      ssize_t index    = sg4::Comm::wait_any(pending_comms);
      std::string* msg = *pending_msgs[index];
      XBT_INFO("I got '%s'.", msg->c_str());
      /* cleanup memory and remove from vectors */
      delete msg;
      pending_comms.erase(pending_comms.begin() + index);
      pending_msgs.erase(pending_msgs.begin() + index);
    }
  }
};

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  /* Creates the platform
   *  ________                 __________
   * | Sender |===============| Receiver |
   * |________|    Link1      |__________|
   */
  auto* zone     = sg4::create_full_zone("Zone1");
  auto* sender   = zone->create_host("sender", 1)->seal();
  auto* receiver = zone->create_host("receiver", 1)->seal();

  /* create split-duplex link1 (UP/DOWN), limiting the number of concurrent flows in it for 2 */
  const auto* link =
      zone->create_split_duplex_link("link1", 10e9)->set_latency(10e-6)->set_concurrency_limit(2)->seal();

  /* create routes between nodes */
  zone->add_route(sender->get_netpoint(), receiver->get_netpoint(), nullptr, nullptr,
                  {{link, sg4::LinkInRoute::Direction::UP}}, true);
  zone->seal();

  /* create actors Sender/Receiver */
  sg4::Actor::create("receiver", receiver, Receiver(10));
  sg4::Actor::create("sender", sender, Sender(1e6, 10));

  e.run();

  return 0;
}
