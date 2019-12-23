/* Copyright (c) 2008-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/xbt_os_time.h>

#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/simdag.h"
#include "src/surf/network_interface.hpp"

#include <algorithm>

XBT_LOG_NEW_DEFAULT_CATEGORY(flatifier, "Logging specific to this platform parsing tool");

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

static void create_environment(xbt_os_timer_t parse_time, const char *platformFile)
{
  xbt_os_cputimer_start(parse_time);
  SD_create_environment(platformFile);
  xbt_os_cputimer_stop(parse_time);
}

static void dump_hosts()
{
  unsigned int totalHosts = sg_host_count();
  sg_host_t* hosts        = sg_host_list();
  std::sort(hosts, hosts + totalHosts,
            [](sg_host_t a, sg_host_t b) { return strcmp(sg_host_get_name(a), sg_host_get_name(b)) < 0; });

  for (unsigned int i = 0; i < totalHosts; i++) {
    std::printf("  <host id=\"%s\" speed=\"%.0f\"", hosts[i]->get_cname(), sg_host_speed(hosts[i]));
    const std::unordered_map<std::string, std::string>* props = hosts[i]->get_properties();
    if (hosts[i]->get_core_count() > 1) {
      std::printf(" core=\"%d\"", hosts[i]->get_core_count());
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
  xbt_free(hosts);
}

static void dump_links()
{
  unsigned int totalLinks    = sg_link_count();
  simgrid::s4u::Link** links = sg_link_list();

  std::sort(links, links + totalLinks,
            [](simgrid::s4u::Link* a, simgrid::s4u::Link* b) { return strcmp(sg_link_name(a), sg_link_name(b)) < 0; });

  for (unsigned int i = 0; i < totalLinks; i++) {
    simgrid::s4u::Link* link = links[i];
    std::printf("  <link id=\"");

    std::printf("%s\" bandwidth=\"%.0f\" latency=\"%.9f\"", link->get_cname(), link->get_bandwidth(), link->get_latency());
    if (sg_link_is_shared(link)) {
      std::printf("/>\n");
    } else {
      std::printf(" sharing_policy=\"FATPIPE\"/>\n");
    }
  }

  xbt_free(links);
}

static void dump_routers()
{
  std::vector<simgrid::kernel::routing::NetPoint*> netpoints =
      simgrid::s4u::Engine::get_instance()->get_all_netpoints();
  std::sort(netpoints.begin(), netpoints.end(),
            [](simgrid::kernel::routing::NetPoint* a, simgrid::kernel::routing::NetPoint* b) {
              return a->get_name() < b->get_name();
            });

  for (auto const& src : netpoints)
    if (src->is_router())
      std::printf("  <router id=\"%s\"/>\n", src->get_cname());
}

static void dump_routes()
{
  unsigned int totalHosts = sg_host_count();
  sg_host_t* hosts        = sg_host_list();
  std::sort(hosts, hosts + totalHosts,
            [](sg_host_t a, sg_host_t b) { return strcmp(sg_host_get_name(a), sg_host_get_name(b)) < 0; });
  std::vector<simgrid::kernel::routing::NetPoint*> netpoints =
      simgrid::s4u::Engine::get_instance()->get_all_netpoints();
  std::sort(netpoints.begin(), netpoints.end(),
            [](simgrid::kernel::routing::NetPoint* a, simgrid::kernel::routing::NetPoint* b) {
              return a->get_name() < b->get_name();
            });

  for (unsigned int it_src = 0; it_src < totalHosts; it_src++) { // Routes from host
    const simgrid::s4u::Host* host1         = hosts[it_src];
    simgrid::kernel::routing::NetPoint* src = host1->get_netpoint();
    for (unsigned int it_dst = 0; it_dst < totalHosts; it_dst++) { // Routes to host
      const simgrid::s4u::Host* host2 = hosts[it_dst];
      std::vector<simgrid::kernel::resource::LinkImpl*> route;
      simgrid::kernel::routing::NetPoint* dst = host2->get_netpoint();
      simgrid::kernel::routing::NetZoneImpl::get_global_route(src, dst, route, nullptr);
      if (not route.empty()) {
        std::printf("  <route src=\"%s\" dst=\"%s\">\n  ", host1->get_cname(), host2->get_cname());
        for (auto const& link : route)
          std::printf("<link_ctn id=\"%s\"/>", link->get_cname());
        std::printf("\n  </route>\n");
      }
    }

    for (auto const& dst : netpoints) { // to router
      if (dst->is_router()) {
        std::printf("  <route src=\"%s\" dst=\"%s\">\n  ", host1->get_cname(), dst->get_cname());
        std::vector<simgrid::kernel::resource::LinkImpl*> route;
        simgrid::kernel::routing::NetZoneImpl::get_global_route(src, dst, route, nullptr);
        for (auto const& link : route)
          std::printf("<link_ctn id=\"%s\"/>", link->get_cname());
        std::printf("\n  </route>\n");
      }
    }
  }

  for (auto const& value1 : netpoints) { // Routes from router
    if (value1->is_router()) {
      for (auto const& value2 : netpoints) { // to router
        if (value2->is_router()) {
          std::printf("  <route src=\"%s\" dst=\"%s\">\n  ", value1->get_cname(), value2->get_cname());
          std::vector<simgrid::kernel::resource::LinkImpl*> route;
          simgrid::kernel::routing::NetZoneImpl::get_global_route(value1, value2, route, nullptr);
          for (auto const& link : route)
            std::printf("<link_ctn id=\"%s\"/>", link->get_cname());
          std::printf("\n  </route>\n");
        }
      }
      for (unsigned int it_dst = 0; it_dst < totalHosts; it_dst++) { // Routes to host
        const simgrid::s4u::Host* host2 = hosts[it_dst];
        std::printf("  <route src=\"%s\" dst=\"%s\">\n  ", value1->get_cname(), host2->get_cname());
        std::vector<simgrid::kernel::resource::LinkImpl*> route;
        simgrid::kernel::routing::NetPoint* netcardDst = host2->get_netpoint();
        simgrid::kernel::routing::NetZoneImpl::get_global_route(value1, netcardDst, route, nullptr);
        for (auto const& link : route)
          std::printf("<link_ctn id=\"%s\"/>", link->get_cname());
        std::printf("\n  </route>\n");
      }
    }
  }
  xbt_free(hosts);
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

  SD_init(&argc, argv);

  if (not parse_cmdline(&timings, &platformFile, argc, argv) || not platformFile)
    xbt_die("Invalid command line arguments: expected [--timings] platformFile");

  XBT_DEBUG("%d,%s", timings, platformFile);

  create_environment(parse_time, platformFile);

  if (timings) {
    XBT_INFO("Parsing time: %fs (%zu hosts, %d links)", xbt_os_timer_elapsed(parse_time), sg_host_count(),
             sg_link_count());
  } else {
    dump_platform();
  }

  xbt_os_timer_free(parse_time);

  return 0;
}
