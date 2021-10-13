/* Copyright (c) 2008-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/xbt_os_time.h>

#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/Link.hpp"
#include "src/surf/network_interface.hpp"

#include <algorithm>
#include <cstring>

XBT_LOG_NEW_DEFAULT_CATEGORY(flatifier, "Logging specific to this platform parsing tool");

namespace sg4 = simgrid::s4u;

static bool parse_cmdline(int* timings, char** platformFile, int argc, char** argv)
{
  bool parse_ok = true;
  for (int i = 1; i < argc; i++) {
    if (std::strlen(argv[i]) > 1 && argv[i][0] == '-' && argv[i][1] == '-') {
      if (not std::strcmp(argv[i], "--timings")) {
        *timings = 1;
      } else {
        parse_ok = false;
        break;
      }
    } else {
      *platformFile = argv[i];
    }
  }
  return parse_ok;
}

static void create_environment(xbt_os_timer_t parse_time, const std::string& platformFile)
{
  xbt_os_cputimer_start(parse_time);
  sg4::Engine::get_instance()->load_platform(platformFile);
  xbt_os_cputimer_stop(parse_time);
}

static void dump_hosts()
{
  std::vector<sg4::Host*> hosts  = sg4::Engine::get_instance()->get_all_hosts();
  std::sort(hosts.begin(), hosts.end(),
            [](const sg4::Host* a, const sg4::Host* b) { return a->get_name() < b->get_name(); });

  for (auto h : hosts) {
    std::printf("  <host id=\"%s\" speed=\"%.0f\"", h->get_cname(), h->get_speed());
    const std::unordered_map<std::string, std::string>* props = h->get_properties();
    if (h->get_core_count() > 1) {
      std::printf(" core=\"%d\"", h->get_core_count());
    }
    // Sort the properties before displaying them, so that the tests are perfectly reproducible
    std::vector<std::string> keys;
    for (auto const& kv : *props)
      keys.push_back(kv.first);
    if (not keys.empty()) {
      std::printf(">\n");
      std::sort(keys.begin(), keys.end());
      for (const std::string& key : keys)
        std::printf("    <prop id=\"%s\" value=\"%s\"/>\n", key.c_str(), props->at(key).c_str());
      std::printf("  </host>\n");
    } else {
      std::printf("/>\n");
    }
  }
}

static void dump_links()
{
  std::vector<sg4::Link*> links = sg4::Engine::get_instance()->get_all_links();

  std::sort(links.begin(), links.end(), [](const sg4::Link* a, const sg4::Link* b) {
    return a->get_name() < b->get_name();
  });

  for (auto link : links) {
    std::printf("  <link id=\"");

    std::printf("%s\" bandwidth=\"%.0f\" latency=\"%.9f\"", link->get_cname(), link->get_bandwidth(), link->get_latency());
    if (link->is_shared()) {
      std::printf("/>\n");
    } else {
      std::printf(" sharing_policy=\"FATPIPE\"/>\n");
    }
  }
}

static void dump_routers()
{
  std::vector<simgrid::kernel::routing::NetPoint*> netpoints =
      sg4::Engine::get_instance()->get_all_netpoints();
  std::sort(netpoints.begin(), netpoints.end(),
            [](const simgrid::kernel::routing::NetPoint* a, const simgrid::kernel::routing::NetPoint* b) {
              return a->get_name() < b->get_name();
            });

  for (auto const& src : netpoints)
    if (src->is_router())
      std::printf("  <router id=\"%s\"/>\n", src->get_cname());
}

static void dump_routes()
{
  std::vector<sg4::Host*> hosts  = sg4::Engine::get_instance()->get_all_hosts();
  std::sort(hosts.begin(), hosts.end(),
            [](const sg4::Host* a, const sg4::Host* b) { return a->get_name() < b->get_name(); });
  std::vector<simgrid::kernel::routing::NetPoint*> netpoints =
      sg4::Engine::get_instance()->get_all_netpoints();
  std::sort(netpoints.begin(), netpoints.end(),
            [](const simgrid::kernel::routing::NetPoint* a, const simgrid::kernel::routing::NetPoint* b) {
              return a->get_name() < b->get_name();
            });

  for (auto src_host : hosts) { // Routes from host
    const simgrid::kernel::routing::NetPoint* src = src_host->get_netpoint();
    for (auto dst_host : hosts) { // Routes to host
      std::vector<simgrid::kernel::resource::LinkImpl*> route;
      const simgrid::kernel::routing::NetPoint* dst = dst_host->get_netpoint();
      simgrid::kernel::routing::NetZoneImpl::get_global_route(src, dst, route, nullptr);
      if (route.empty())
        continue;
      std::printf("  <route src=\"%s\" dst=\"%s\">\n  ", src_host->get_cname(), dst_host->get_cname());
      for (auto const& link : route)
        std::printf("<link_ctn id=\"%s\"/>", link->get_cname());
      std::printf("\n  </route>\n");
    }

    for (auto const& dst : netpoints) { // to router
      if (not dst->is_router())
        continue;
      std::printf("  <route src=\"%s\" dst=\"%s\">\n  ", src_host->get_cname(), dst->get_cname());
      std::vector<simgrid::kernel::resource::LinkImpl*> route;
      simgrid::kernel::routing::NetZoneImpl::get_global_route(src, dst, route, nullptr);
      for (auto const& link : route)
        std::printf("<link_ctn id=\"%s\"/>", link->get_cname());
      std::printf("\n  </route>\n");
    }
  }

  for (auto const& value1 : netpoints) { // Routes from router
    if (not value1->is_router())
      continue;
    for (auto const& value2 : netpoints) { // to router
      if (not value2->is_router())
        continue;
      std::vector<simgrid::kernel::resource::LinkImpl*> route;
      simgrid::kernel::routing::NetZoneImpl::get_global_route(value1, value2, route, nullptr);
      if (route.empty())
        continue;
      std::printf("  <route src=\"%s\" dst=\"%s\">\n  ", value1->get_cname(), value2->get_cname());
      for (auto const& link : route)
        std::printf("<link_ctn id=\"%s\"/>", link->get_cname());
      std::printf("\n  </route>\n");
    }
    for (auto dst_host : hosts) { // Routes to host
      std::printf("  <route src=\"%s\" dst=\"%s\">\n  ", value1->get_cname(), dst_host->get_cname());
      std::vector<simgrid::kernel::resource::LinkImpl*> route;
      const simgrid::kernel::routing::NetPoint* netcardDst = dst_host->get_netpoint();
      simgrid::kernel::routing::NetZoneImpl::get_global_route(value1, netcardDst, route, nullptr);
      for (auto const& link : route)
        std::printf("<link_ctn id=\"%s\"/>", link->get_cname());
      std::printf("\n  </route>\n");
    }
  }
}

static void dump_platform()
{
  int version = 4;

  std::printf("<?xml version='1.0'?>\n");
  std::printf("<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">\n");
  std::printf("<platform version=\"%d\">\n", version);
  std::printf("<AS id=\"AS0\" routing=\"Full\">\n");

  // Hosts
  dump_hosts();

  // Routers
  dump_routers();

  // Links
  dump_links();

  // Routes
  dump_routes();

  std::printf("</AS>\n");
  std::printf("</platform>\n");
}

int main(int argc, char** argv)
{
  char* platformFile = nullptr;
  int timings        = 0;

  xbt_os_timer_t parse_time = xbt_os_timer_new();

  sg4::Engine e(&argc, argv);

  xbt_assert(parse_cmdline(&timings, &platformFile, argc, argv) && platformFile,
             "Invalid command line arguments: expected [--timings] platformFile");

  XBT_DEBUG("%d,%s", timings, platformFile);

  create_environment(parse_time, platformFile);

  if (timings) {
    XBT_INFO("Parsing time: %fs (%zu hosts, %zu links)", xbt_os_timer_elapsed(parse_time), e.get_host_count(),
             e.get_link_count());
  } else {
    dump_platform();
  }

  xbt_os_timer_free(parse_time);

  return 0;
}
