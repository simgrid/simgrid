/* Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to build a torus cluster with multi-core hosts.
 *
 * However, each leaf in the torus is a StarZone, composed of several CPUs
 *
 * Each actor runs in a specific CPU. One sender broadcasts a message to all receivers.
 */

#include "simgrid/s4u.hpp"
namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_torus_multicpu, "Messages specific for this s4u example");

class Sender {
  long msg_size = 1e6; /* message size in bytes */
  std::vector<sg4::Host*> hosts_;

public:
  explicit Sender(const std::vector<sg4::Host*> hosts) : hosts_{hosts} {}
  void operator()() const
  {
    /* Vector in which we store all ongoing communications */
    std::vector<sg4::CommPtr> pending_comms;

    /* Make a vector of the mailboxes to use */
    std::vector<sg4::Mailbox*> mboxes;

    /* Start dispatching 1 message to all receivers */
    std::string msg_content =
        std::string("Hello, I'm alive and running on ") + std::string(sg4::this_actor::get_host()->get_name());
    for (const auto* host : hosts_) {
      /* Copy the data we send: the 'msg_content' variable is not a stable storage location.
       * It will be destroyed when this actor leaves the loop, ie before the receiver gets it */
      auto* payload = new std::string(msg_content);

      /* Create a communication representing the ongoing communication, and store it in pending_comms */
      auto mbox = sg4::Mailbox::by_name(host->get_name());
      mboxes.push_back(mbox);
      sg4::CommPtr comm = mbox->put_async(payload, msg_size);
      pending_comms.push_back(comm);
    }

    XBT_INFO("Done dispatching all messages");

    /* Now that all message exchanges were initiated, wait for their completion in one single call */
    sg4::Comm::wait_all(&pending_comms);

    XBT_INFO("Goodbye now!");
  }
};

/* Receiver actor: wait for 1 message on the mailbox identified by the hostname */
class Receiver {
public:
  void operator()() const
  {
    auto mbox     = sg4::Mailbox::by_name(sg4::this_actor::get_host()->get_name());
    auto received = mbox->get_unique<std::string>();
    XBT_INFO("I got a '%s'.", received->c_str());
  }
};

/**
 * @brief Callback to set a Torus leaf/element
 *
 * In our example, each leaf if a StarZone, composed of 8 CPUs.
 * Each CPU is modeled as a host, connected to the outer world through a high-speed PCI link.
 * Obs.: CPU0 is the gateway for this zone
 *
 *    (outer world)
 *         CPU0 (gateway)
 *    up ->|   |
 *         |   |<-down
 *         +star+
 *      /   / \   \
 *     /   /   \   \<-- 100Gbs, 10us link (1 link UP and 1 link DOWN for full-duplex)
 *    /   /     \   \
 *   /   /       \   \
 *   CPU1   ...   CPU8
 *
 * @param zone Torus netzone being created (usefull to create the hosts/links inside it)
 * @param coord Coordinates in the torus (e.g. "0,0,0", "0,1,0")
 * @param id Internal identifier in the torus (for information)
 * @return netpoint, gateway: the netpoint to the StarZone and CPU0 as gateway
 */
static std::pair<simgrid::kernel::routing::NetPoint*, simgrid::kernel::routing::NetPoint*>
create_hostzone(const sg4::NetZone* zone, const std::vector<unsigned int>& /*coord*/, int id)
{
  constexpr int num_cpus    = 8;     //!< Number of CPUs in the zone
  constexpr double speed    = 1e9;   //!< Speed of each CPU
  constexpr double link_bw  = 100e9; //!< Link bw connecting the CPU
  constexpr double link_lat = 1e-9;  //!< Link latency

  std::string hostname = "host" + std::to_string(id);
  /* create the StarZone */
  auto* host_zone = sg4::create_star_zone(hostname);
  /* setting my Torus parent zone */
  host_zone->set_parent(zone);

  const sg4::Host* gateway = nullptr;
  /* create CPUs */
  for (int i = 0; i < num_cpus; i++) {
    std::string cpu_name = hostname + "-cpu" + std::to_string(i);
    sg4::Host* host      = host_zone->create_host(cpu_name, speed)->seal();
    /* the first CPU is the gateway */
    if (i == 0)
      gateway = host;
    /* create 2 links for a full-duplex communication */
    sg4::Link* link_up   = host_zone->create_link("link-up-" + cpu_name, link_bw)->set_latency(link_lat)->seal();
    sg4::Link* link_down = host_zone->create_link("link-down-" + cpu_name, link_bw)->set_latency(link_lat)->seal();
    /* link UP, connection from CPU to outer world */
    host_zone->add_route(host->get_netpoint(), nullptr, nullptr, nullptr, {link_up}, false);
    /* link DOWN, connection from outer to CPU */
    host_zone->add_route(nullptr, host->get_netpoint(), nullptr, nullptr, {link_down}, false);
  }
  return std::make_pair(host_zone->get_netpoint(), gateway->get_netpoint());
}

/**
 * @brief Creates a TORUS cluster
 *
 * Creates a TORUS clustes with dimensions 2x2x2
 *
 * The cluster has 8 elements/leaves in total. Each element is a StarZone containing 8 Hosts.
 * Each pair in the torus is connected through 2 links:
 * 1) limiter: a 1Gbs limiter link (set by user through the set_limiter callback)
 * 2) link: 10Gbs link connecting the components (created automatically)
 *
 * (Y-axis=2)
 * A
 * |
 * |   X (Z-axis=2)
 * |  / 10 Gbs
 * | +
 * |/ limiter=1Gps
 * B----------C (X-axis=2)
 *
 * For example, a communication from A to C goes through:
 * <tt> A->limiter(A)->link(A-B)->limiter(B)->link(B-C)->C </tt>
 *
 * More precisely, considering that A and C are StarZones, a
 * communication from A-CPU-3 to C-CPU-7 goes through:
 * 1) StarZone A: A-CPU-3 -> link-up-A-CPU-3 -> A-CPU-0
 * 2) A-CPU-0->limiter(A)->link(A-B)->limiter(B)->link(B-C)->C-CPU-0
 * 3) C-CPU-0-> link-down-C-CPU-7 -> C-CPU-7
 *
 * Note that we don't have limiter links inside the StarZones(A, B, C),
 * but we have limiters in the Torus that are added to the links in the path (as we can see in "2)"")
 *
 * More details in: <a href="https://simgrid.org/doc/latest/Platform_examples.html?highlight=torus#torus-cluster">Torus
 * Cluster</a>
 */
static void create_torus_cluster()
{
  // Callback to create limiter link (1Gbs) for each host
  auto create_limiter = [](sg4::NetZone* zone, const std::vector<unsigned int>& /*coord*/, int id) -> sg4::Link* {
    return zone->create_link("limiter-" + std::to_string(id), 1e9)->seal();
  };

  /* create the torus cluster, 10Gbs link between elements in the cluster */
  sg4::create_torus_zone("cluster", nullptr, {2, 2, 2}, 10e9, 10e-6, sg4::Link::SharingPolicy::SPLITDUPLEX,
                         create_hostzone, {}, create_limiter)
      ->seal();
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  /* create platform */
  create_torus_cluster();

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
