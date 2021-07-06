/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/plugins/ProducerConsumer.hpp>
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

  for (int i = 0; i < 3; i++) {
    auto* data = new int(10 * id + i);
    pc->put_async(data, 1.2125e6); // last for 0.01s
    XBT_INFO("data sucessfully put: %d", *data);
    sg4::this_actor::sleep_for((i + 3) * simgrid::xbt::random::uniform_real(0, 1));
  }
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
  auto* cluster = sg4::create_star_zone("cluster");
  for (int i = 0; i < 8; i++) {
    std::string hostname = std::string("node-") + std::to_string(i) + ".simgrid.org";

    const auto* host = cluster->create_host(hostname, "1Gf");

    std::string linkname = std::string("cluster") + "_link_" + std::to_string(i);
    const auto* link     = cluster->create_split_duplex_link(linkname, "1Gbps");

    cluster->add_route(host->get_netpoint(), nullptr, nullptr, nullptr, {{link, sg4::LinkInRoute::Direction::UP}},
                       true);
  }

  auto* router = cluster->create_router("cluster_router");
  cluster->add_route(router, nullptr, nullptr, nullptr, {});

  simgrid::plugin::ProducerConsumerPtr<int> pc = simgrid::plugin::ProducerConsumer<int>::create(2);

  XBT_INFO("Maximum number of queued data is %u", pc->get_max_queue_size());
  XBT_INFO("Transfers are done in %s mode", pc->get_transfer_mode().c_str());

  for (int i = 0; i < 3; i++) {
    std::string hostname = std::string("node-") + std::to_string(i) + ".simgrid.org";
    sg4::Actor::create("ingester-" + std::to_string(i), sg4::Host::by_name(hostname), &ingester, i, pc);

    hostname = std::string("node-") + std::to_string(i + 3) + ".simgrid.org";
    sg4::Actor::create("retriever-" + std::to_string(i), sg4::Host::by_name(hostname), &retriever, pc);
  }

  e.run();

  return 0;
}
