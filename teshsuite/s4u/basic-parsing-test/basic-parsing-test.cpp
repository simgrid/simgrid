/* Copyright (c) 2008-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <simgrid/s4u.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(basic_parsing_test, "[usage] basic-parsing-test <platform-file>");

namespace sg4 = simgrid::s4u;

static void test_one_link(const std::vector<sg4::Host*> hosts)
{
  const sg4::Host* h1 = hosts[0];
  const sg4::Host* h2 = hosts[1];
  std::vector<sg4::Link*> route;
  double latency = 0;
  double min_bandwidth = -1;

  XBT_INFO("Route between %s and %s\n", h1->get_cname(), h2->get_cname());
  h1->route_to(h2, route, &latency);
  XBT_INFO("Route size %zu\n", route.size());

  for (auto link: route) {
    double bandwidth = link->get_bandwidth();
    XBT_INFO("  Link %s: latency = %f, bandwidth = %f\n", link->get_cname(), link->get_latency(), bandwidth);
    if (bandwidth < min_bandwidth || min_bandwidth < 0.0)
      min_bandwidth = bandwidth;

  }
  XBT_INFO("Route latency = %f, route bandwidth = %f\n", latency, min_bandwidth);
}

static void test_full_link(const std::vector<sg4::Host*> hosts)
{
  size_t list_size = hosts.size();
  for (size_t i = 0; i < list_size; i++) {
    const sg4::Host* h1 = hosts[i];
    for (size_t j = 0; j < list_size; j++) {
      const sg4::Host* h2 = hosts[j];
      XBT_INFO("Route between %s and %s\n", h1->get_cname(), h2->get_cname());
      std::vector<sg4::Link*> route;
      double latency = 0;
      double min_bandwidth = -1;
      h1->route_to(h2, route, &latency);
      XBT_INFO("  Route size %zu\n", route.size());

      for (auto link: route) {
        double bandwidth = link->get_bandwidth();
        XBT_INFO("  Link %s: latency = %f, bandwidth = %f\n", link->get_cname(), link->get_latency(), bandwidth);
        if (bandwidth < min_bandwidth || min_bandwidth < 0.0)
          min_bandwidth = bandwidth;

      }
      XBT_INFO("  Route latency = %f, route bandwidth = %f\n", latency, min_bandwidth);
    }
  }
}

int main(int argc, char** argv)
{
  simgrid::s4u::Engine e(&argc, argv);

  /* creation of the environment */
  e.load_platform(argv[1]);
  XBT_INFO("Workstation number: %zu, link number: %zu\n", e.get_host_count(), e.get_link_count());

  std::vector<simgrid::s4u::Host*> hosts = e.get_all_hosts();
  if (argc >= 3) {
    if (!strcmp(argv[2], "ONE_LINK"))
      test_one_link(hosts);
    if (!strcmp(argv[2], "FULL_LINK"))
      test_full_link(hosts);
    if (!strcmp(argv[2], "PROP"))
      XBT_INFO("SG_TEST_mem: %s\n", sg4::Host::by_name("host1")->get_property("SG_TEST_mem"));
  }

  return 0;
}
