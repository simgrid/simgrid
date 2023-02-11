/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/xbt_os_time.h>

#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/Link.hpp"
#include "src/kernel/resource/StandardLinkImpl.hpp"

#include <algorithm>
#include <cstring>
#include <sstream>

XBT_LOG_NEW_DEFAULT_CATEGORY(flatifier, "Logging specific to this platform parsing tool");

namespace sg4 = simgrid::s4u;

static bool parse_cmdline(bool* timings, char** platformFile, int argc, char** argv)
{
  bool parse_ok = true;
  for (int i = 1; i < argc; i++) {
    if (std::strlen(argv[i]) > 1 && argv[i][0] == '-' && argv[i][1] == '-') {
      if (not std::strcmp(argv[i], "--timings")) {
        *timings = true;
      } else {
        parse_ok = false;
        break;
      }
    } else {
      *platformFile = argv[i];
    }
  }
  return parse_ok && platformFile != nullptr;
}

static void dump_hosts(sg4::Engine& engine, std::stringstream& ss)
{
  std::vector<sg4::Host*> hosts = engine.get_all_hosts();

  for (auto const* h : hosts) {
    ss << "  <host id=\"" << h->get_name() << "\" speed=\"" << h->get_speed() << "\"";
    const std::unordered_map<std::string, std::string>* props = h->get_properties();
    if (h->get_core_count() > 1)
      ss << " core=\"" << h->get_core_count() << "\"";

    // Sort the properties before displaying them, so that the tests are perfectly reproducible
    std::vector<std::string> keys;
    for (auto const& [key, _] : *props)
      keys.push_back(key);
    if (not keys.empty()) {
      ss << ">\n";
      std::sort(keys.begin(), keys.end());
      for (const std::string& key : keys)
        ss << "    <prop id=\"" << key << "\" value=\"" << props->at(key) << "\"/>\n";
      ss << "  </host>\n";
    } else {
      ss << "/>\n";
    }
  }
}

static void dump_links(sg4::Engine& engine, std::stringstream& ss)
{
  std::vector<sg4::Link*> links = engine.get_all_links();

  std::sort(links.begin(), links.end(),
            [](const sg4::Link* a, const sg4::Link* b) { return a->get_name() < b->get_name(); });

  for (auto const* link : links) {
    ss << "  <link id=\"" << link->get_name() << "\" ";
    ss << "bandwidth=\"" << link->get_bandwidth() << "\" ";
    ss << "latency=\"" << link->get_latency() << "\"";
    if (link->is_shared()) {
      ss << "/>\n";
    } else {
      ss << " sharing_policy=\"FATPIPE\"/>\n";
    }
  }
}

static void dump_routers(sg4::Engine& engine, std::stringstream& ss)
{
  std::vector<simgrid::kernel::routing::NetPoint*> netpoints = engine.get_all_netpoints();
  std::sort(netpoints.begin(), netpoints.end(),
            [](const simgrid::kernel::routing::NetPoint* a, const simgrid::kernel::routing::NetPoint* b) {
              return a->get_name() < b->get_name();
            });

  for (auto const& src : netpoints)
    if (src->is_router())
      ss << "  <router id=\"" << src->get_name() << "\"/>\n";
}

static void dump_routes(sg4::Engine& engine, std::stringstream& ss)
{
  auto hosts     = engine.get_all_hosts();
  auto netpoints = engine.get_all_netpoints();
  std::sort(netpoints.begin(), netpoints.end(),
            [](const simgrid::kernel::routing::NetPoint* a, const simgrid::kernel::routing::NetPoint* b) {
              return a->get_name() < b->get_name();
            });

  for (auto const* src_host : hosts) { // Routes from host
    const simgrid::kernel::routing::NetPoint* src = src_host->get_netpoint();
    for (auto const* dst_host : hosts) { // Routes to host
      std::vector<simgrid::kernel::resource::StandardLinkImpl*> route;
      const simgrid::kernel::routing::NetPoint* dst = dst_host->get_netpoint();
      simgrid::kernel::routing::NetZoneImpl::get_global_route(src, dst, route, nullptr);
      if (route.empty())
        continue;
      ss << "  <route src=\"" << src_host->get_name() << "\" dst=\"" << dst_host->get_name() << "\">\n  ";
      for (auto const& link : route)
        ss << "<link_ctn id=\"" << link->get_name() << "\"/>";
      ss << "\n  </route>\n";
    }

    for (auto const& dst : netpoints) { // to router
      if (not dst->is_router())
        continue;
      ss << "  <route src=\"" << src_host->get_name() << "\" dst=\"" << dst->get_name() << "\">\n  ";
      std::vector<simgrid::kernel::resource::StandardLinkImpl*> route;
      simgrid::kernel::routing::NetZoneImpl::get_global_route(src, dst, route, nullptr);
      for (auto const& link : route)
        ss << "<link_ctn id=\"" << link->get_name() << "\"/>";
      ss << "\n  </route>\n";
    }
  }

  for (auto const& value1 : netpoints) { // Routes from router
    if (not value1->is_router())
      continue;
    for (auto const& value2 : netpoints) { // to router
      if (not value2->is_router())
        continue;
      std::vector<simgrid::kernel::resource::StandardLinkImpl*> route;
      simgrid::kernel::routing::NetZoneImpl::get_global_route(value1, value2, route, nullptr);
      if (route.empty())
        continue;
      ss << "  <route src=\"" << value1->get_name() << "\" dst=\"" << value2->get_name() << "\">\n  ";
      for (auto const& link : route)
        ss << "<link_ctn id=\"" << link->get_name() << "\"/>";
      ss << "\n  </route>\n";
    }
    for (auto const* dst_host : hosts) { // Routes to host
      ss << "  <route src=\"" << value1->get_name() << "\" dst=\"" << dst_host->get_name() << "\">\n  ";
      std::vector<simgrid::kernel::resource::StandardLinkImpl*> route;
      const simgrid::kernel::routing::NetPoint* netcardDst = dst_host->get_netpoint();
      simgrid::kernel::routing::NetZoneImpl::get_global_route(value1, netcardDst, route, nullptr);
      for (auto const& link : route)
        ss << "<link_ctn id=\"" << link->get_name() << "\"/>";
      ss << "\n  </route>\n";
    }
  }
}

static std::string dump_platform(sg4::Engine& engine)
{
  std::string version = "4.1";
  std::stringstream ss;

  ss << "<?xml version='1.0'?>\n";
  ss << "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">\n";
  ss << "<platform version=\"" << version << "\">\n";
  ss << "<AS id=\"" << engine.get_netzone_root()->get_name() << "\" routing=\"Full\">\n";

  dump_hosts(engine, ss);
  dump_routers(engine, ss);
  dump_links(engine, ss);
  dump_routes(engine, ss);

  ss << "</AS>\n";
  ss << "</platform>\n";
  return ss.str();
}

int main(int argc, char** argv)
{
  char* platformFile = nullptr;
  bool timings       = false;

  xbt_os_timer_t parse_time = xbt_os_timer_new();

  sg4::Engine e(&argc, argv);

  xbt_assert(parse_cmdline(&timings, &platformFile, argc, argv),
             "Invalid command line arguments: expected [--timings] platformFile");

  xbt_os_cputimer_start(parse_time);
  e.load_platform(platformFile);
  e.seal_platform();
  xbt_os_cputimer_stop(parse_time);

  if (timings) {
    XBT_INFO("Parsing time: %fs (%zu hosts, %zu links)", xbt_os_timer_elapsed(parse_time), e.get_host_count(),
             e.get_link_count());
  } else {
    std::printf("%s", dump_platform(e).c_str());
  }

  xbt_os_timer_free(parse_time);

  return 0;
}
