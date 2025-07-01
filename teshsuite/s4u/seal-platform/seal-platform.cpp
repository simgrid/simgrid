/* Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(seal_platform, "Messages specific for this s4u example");

class Sender {
  long msg_size = 1e6; /* message size in bytes */
  std::vector<sg4::Host*> hosts_;

public:
  explicit Sender(const std::vector<sg4::Host*>& hosts) : hosts_{hosts} {}
  void operator()() const
  {
    /* Vector in which we store all ongoing communications */
    sg4::ActivitySet pending_comms;
    /* Make a vector of the mailboxes to use */
    std::vector<sg4::Mailbox*> mboxes;

    std::string msg_content = "Hello, I'm alive and running on " + sg4::this_actor::get_host()->get_name();
    for (const auto* host : hosts_) {
      auto* payload = new std::string(msg_content);
      /* Create a communication representing the ongoing communication, and store it in pending_comms */
      auto* mbox = sg4::Mailbox::by_name(host->get_name());
      mboxes.push_back(mbox);
      sg4::CommPtr comm = mbox->put_async(payload, msg_size);
      pending_comms.push(comm);
    }

    XBT_INFO("Done dispatching all messages");

    /* Now that all message exchanges were initiated, wait for their completion in one single call */
    pending_comms.wait_all();

    XBT_INFO("Goodbye now!");
  }
};

/* Receiver actor: wait for 1 message on the mailbox identified by the hostname */
class Receiver {
public:
  void operator()() const
  {
    auto* mbox    = sg4::Mailbox::by_name(sg4::this_actor::get_host()->get_name());
    auto received = mbox->get_unique<std::string>();
    XBT_INFO("I got a '%s'.", received->c_str());

    const sg4::Disk* disk = sg4::Host::current()->get_disks().front();
    sg_size_t write_size  = disk->write(4e6);
    XBT_INFO("Wrote %llu bytes on '%s'", write_size, disk->get_cname());
  }
};

/*************************************************************************************************/
static sg4::NetZone* create_zone(sg4::NetZone* root, const std::string& id)
{
  auto* zone           = root->add_netzone_star(id);
  constexpr int n_host = 2;

  zone->set_gateway(zone->add_router("router" + id));
  for (int i = 0; i < n_host; i++) {
    std::string hostname = id + "-cpu-" + std::to_string(i);
    auto* host           = zone->add_host(hostname, 1e9);
    host->add_disk("disk-" + hostname, 1e9, 1e6);
    const auto* link = zone->add_link("link-" + hostname, 1e9);
    zone->add_route(host, nullptr, {link});
  }
  return zone;
}

/*************************************************************************************************/

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  /* create platform: intentionally do not do the seal of objects */
  auto* root       = e.get_netzone_root();
  auto* zoneA = create_zone(root, "A");
  auto* zoneB = create_zone(root, "B");
  const auto* link = root->add_link("root-link", 1e10);
  root->add_route(zoneA, zoneB, {sg4::LinkInRoute(link)}, true);

  std::vector<sg4::Host*> host_list = e.get_all_hosts();
  /* create the sender actor running on first host */
  host_list[0]->add_actor("sender", Sender(host_list));
  /* create receiver in every host */
  for (auto* host : host_list) {
    host->add_actor("receiver-" + host->get_name(),Receiver());
  }

  /* runs the simulation */
  e.run();

  return 0;
}
