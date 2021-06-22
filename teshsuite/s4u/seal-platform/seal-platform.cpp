/* Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_torus_multicpu, "Messages specific for this s4u example");

class Sender {
  long msg_size = 1e6; /* message size in bytes */
  std::vector<sg4::Host*> hosts_;

public:
  explicit Sender(const std::vector<sg4::Host*>& hosts) : hosts_{hosts} {}
  void operator()() const
  {
    /* Vector in which we store all ongoing communications */
    std::vector<sg4::CommPtr> pending_comms;
    /* Make a vector of the mailboxes to use */
    std::vector<sg4::Mailbox*> mboxes;

    std::string msg_content =
        std::string("Hello, I'm alive and running on ") + std::string(sg4::this_actor::get_host()->get_name());
    for (const auto* host : hosts_) {
      auto* payload = new std::string(msg_content);
      /* Create a communication representing the ongoing communication, and store it in pending_comms */
      auto mbox = sg4::Mailbox::by_name(host->get_name());
      mboxes.push_back(mbox);
      sg4::CommPtr comm = mbox->put_async(payload, msg_size);
      pending_comms.push_back(comm);
    }

    XBT_INFO("Done dispatching all messages");

    /* Now that all message exchanges were initiated, wait for their completion in one single call */
    sg4::Comm::wait_all(pending_comms);

    XBT_INFO("Goodbye now!");
  }
};

/* Receiver actor: wait for 1 message on the mailbox identified by the hostname */
class Receiver {
public:
  void operator()() const
  {
    auto mbox     = sg4::Mailbox::by_name(sg4::this_actor::get_host()->get_name());
    auto comm     = mbox->get_init();
    auto received = mbox->get_unique<std::string>();
    XBT_INFO("I got a '%s'.", received->c_str());

    const sg4::Disk* disk = sg4::Host::current()->get_disks().front();
    sg_size_t write_size  = disk->write(4e6);
    XBT_INFO("Wrote %llu bytes on '%s'", write_size, disk->get_cname());
  }
};

/*************************************************************************************************/
static sg4::NetZone* create_zone(const sg4::NetZone* root, const std::string& id)
{
  auto* zone = sg4::create_floyd_zone(id);
  zone->set_parent(root);
  constexpr int n_host = 2;

  auto* router = zone->create_router("router" + id);
  for (int i = 0; i < n_host; i++) {
    std::string hostname = id + "-cpu-" + std::to_string(i);
    auto* host           = zone->create_host(hostname, 1e9);
    host->create_disk("disk-" + hostname, 1e9, 1e6);
    auto* link = zone->create_link("link-" + hostname, 1e9);
    zone->add_route(host->get_netpoint(), router, nullptr, nullptr, std::vector<sg4::Link*>{link});
  }
  return zone;
}

/*************************************************************************************************/

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  /* create platform: intentionally do not do the seal of objects */
  auto* root  = sg4::create_full_zone("root");
  auto* zoneA = create_zone(root, "A");
  auto* zoneB = create_zone(root, "B");
  auto* link  = root->create_link("root-link", 1e10);
  root->add_route(zoneA->get_netpoint(), zoneB->get_netpoint(), e.netpoint_by_name("routerA"),
                  e.netpoint_by_name("routerB"), std::vector<sg4::Link*>{link});

  std::vector<sg4::Host*> host_list = e.get_all_hosts();
  /* create the sender actor running on first host */
  sg4::Actor::create("sender", host_list[0], Sender(host_list));
  /* create receiver in every host */
  for (auto* host : host_list) {
    sg4::Actor::create(std::string("receiver-") + std::string(host->get_name()), host, Receiver());
  }

  /* runs the simulation */
  e.run();

  return 0;
}
