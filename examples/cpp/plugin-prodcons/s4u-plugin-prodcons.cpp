/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/plugins/ProducerConsumer.hpp>
#include <simgrid/s4u/ActivitySet.hpp>
#include <simgrid/s4u/Actor.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Host.hpp>
#include <xbt/random.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

namespace sg4 = simgrid::s4u;

static void ingester(int id, simgrid::plugin::ProducerConsumerPtr<int> pc)
{
  sg4::this_actor::sleep_for(simgrid::xbt::random::uniform_real(0, 1));
  for (int i = 0; i < 3; i++) {
    auto* data = new int(10 * id + i);
    pc->put(data, 1.2125e6); // last for 0.01s
    XBT_INFO("data sucessfully put: %d", *data);
    sg4::this_actor::sleep_for((3 - i) * simgrid::xbt::random::uniform_real(0, 1));
  }

  sg4::ActivitySet pending;
  for (int i = 0; i < 3; i++) {
    auto* data = new int(10 * id + i);
    pending.push(pc->put_async(data, 1.2125e6)); // last for 0.01s
    XBT_INFO("data sucessfully put: %d", *data);
    sg4::this_actor::sleep_for((i + 3) * simgrid::xbt::random::uniform_real(0, 1));
  }
  pending.wait_all();
}

static void retriever(simgrid::plugin::ProducerConsumerPtr<int> pc)
{
  sg4::this_actor::sleep_for(simgrid::xbt::random::uniform_real(0, 1));
  for (int i = 0; i < 3; i++) {
    int* data;
    sg4::CommPtr comm = pc->get_async(&data);
    comm->wait();
    XBT_INFO("data sucessfully get: %d", *data);
    delete data;
    sg4::this_actor::sleep_for((i + 3) * simgrid::xbt::random::uniform_real(0, 1));
  }

  for (int i = 0; i < 3; i++) {
    int* data = pc->get();
    XBT_INFO("data sucessfully get: %d", *data);
    delete data;
    sg4::this_actor::sleep_for((3 - i) * simgrid::xbt::random::uniform_real(0, 1));
  }
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  // Platform creation
  auto* cluster = e.get_netzone_root()->add_netzone_star("cluster");
  for (int i = 0; i < 8; i++) {
    std::string hostname = "node-" + std::to_string(i) + ".simgrid.org";

    const auto* host = cluster->add_host(hostname, "1Gf");

    std::string linkname = "cluster_link_" + std::to_string(i);
    const auto* link     = cluster->add_split_duplex_link(linkname, "1Gbps");

    cluster->add_route(host, nullptr,  {{link, sg4::LinkInRoute::Direction::UP}}, true);
  }
  cluster->seal();

  simgrid::plugin::ProducerConsumerPtr<int> pc = simgrid::plugin::ProducerConsumer<int>::create(2);

  XBT_INFO("Maximum number of queued data is %u", pc->get_max_queue_size());
  XBT_INFO("Transfers are done in %s mode", pc->get_transfer_mode().c_str());

  for (int i = 0; i < 3; i++) {
    std::string hostname = "node-" + std::to_string(i) + ".simgrid.org";
    e.host_by_name(hostname)->add_actor("ingester-" + std::to_string(i), &ingester, i, pc);

    hostname = "node-" + std::to_string(i + 3) + ".simgrid.org";
    e.host_by_name(hostname)->add_actor("retriever-" + std::to_string(i), &retriever, pc);
  }

  e.run();

  return 0;
}
