/* Copyright (c) 2006-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u/Engine.hpp>

#include "src/kernel/resource/CpuImpl.hpp"
#include "src/kernel/resource/LinkImpl.hpp"
#include "src/surf/surf_interface.hpp"
#include "src/surf/xml/platf.hpp"
#include "src/surf/xml/platf_private.hpp"

#include <vector>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_parse);

/* Trace related stuff */
XBT_PRIVATE std::unordered_map<std::string, simgrid::kernel::profile::Profile*> traces_set_list;
XBT_PRIVATE std::unordered_map<std::string, std::string> trace_connect_list_host_avail;
XBT_PRIVATE std::unordered_map<std::string, std::string> trace_connect_list_host_speed;
XBT_PRIVATE std::unordered_map<std::string, std::string> trace_connect_list_link_avail;
XBT_PRIVATE std::unordered_map<std::string, std::string> trace_connect_list_link_bw;
XBT_PRIVATE std::unordered_map<std::string, std::string> trace_connect_list_link_lat;

void sg_platf_trace_connect(simgrid::kernel::routing::TraceConnectCreationArgs* trace_connect)
{
  surf_parse_assert(traces_set_list.find(trace_connect->trace) != traces_set_list.end(),
                    std::string("Cannot connect trace ") + trace_connect->trace + " to " + trace_connect->element +
                        ": trace unknown");

  switch (trace_connect->kind) {
    case simgrid::kernel::routing::TraceConnectKind::HOST_AVAIL:
      trace_connect_list_host_avail.insert({trace_connect->trace, trace_connect->element});
      break;
    case simgrid::kernel::routing::TraceConnectKind::SPEED:
      trace_connect_list_host_speed.insert({trace_connect->trace, trace_connect->element});
      break;
    case simgrid::kernel::routing::TraceConnectKind::LINK_AVAIL:
      trace_connect_list_link_avail.insert({trace_connect->trace, trace_connect->element});
      break;
    case simgrid::kernel::routing::TraceConnectKind::BANDWIDTH:
      trace_connect_list_link_bw.insert({trace_connect->trace, trace_connect->element});
      break;
    case simgrid::kernel::routing::TraceConnectKind::LATENCY:
      trace_connect_list_link_lat.insert({trace_connect->trace, trace_connect->element});
      break;
    default:
      surf_parse_error(std::string("Cannot connect trace ") + trace_connect->trace + " to " + trace_connect->element +
                       ": unknown kind of trace");
  }
}

/* This function acts as a main in the parsing area. */
void parse_platform_file(const std::string& file)
{
  sg_platf_init();

  /* init the flex parser */
  surf_parse_open(file);

  /* Do the actual parsing */
  surf_parse();

  /* Get the Engine singleton once and for all*/
  const auto engine = simgrid::s4u::Engine::get_instance();

  /* connect all profiles relative to hosts */
  for (auto const& elm : trace_connect_list_host_avail) {
    surf_parse_assert(traces_set_list.find(elm.first) != traces_set_list.end(),
                      std::string("<trace_connect kind=\"HOST_AVAIL\">: Trace ") + elm.first + " undefined.");
    auto profile = traces_set_list.at(elm.first);

    auto host = engine->host_by_name_or_null(elm.second);
    surf_parse_assert(host, std::string("<trace_connect kind=\"HOST_AVAIL\">: Host ") + elm.second + " undefined.");
    host->set_state_profile(profile);
  }

  for (auto const& elm : trace_connect_list_host_speed) {
    surf_parse_assert(traces_set_list.find(elm.first) != traces_set_list.end(),
                      std::string("<trace_connect kind=\"SPEED\">: Trace ") + elm.first + " undefined.");
    auto profile = traces_set_list.at(elm.first);

    auto host = engine->host_by_name_or_null(elm.second);
    surf_parse_assert(host, std::string("<trace_connect kind=\"SPEED\">: Host ") + elm.second + " undefined.");
    host->set_speed_profile(profile);
  }

  for (auto const& elm : trace_connect_list_link_avail) {
    surf_parse_assert(traces_set_list.find(elm.first) != traces_set_list.end(),
                      std::string("<trace_connect kind=\"LINK_AVAIL\">: Trace ") + elm.first + " undefined.");
    auto profile = traces_set_list.at(elm.first);

    auto link = engine->link_by_name_or_null(elm.second);
    surf_parse_assert(link, std::string("<trace_connect kind=\"LINK_AVAIL\">: Link ") + elm.second + " undefined.");
    link->set_state_profile(profile);
  }

  for (auto const& elm : trace_connect_list_link_bw) {
    surf_parse_assert(traces_set_list.find(elm.first) != traces_set_list.end(),
                      std::string("<trace_connect kind=\"BANDWIDTH\">: Trace ") + elm.first + " undefined.");
    auto profile = traces_set_list.at(elm.first);

    auto link = engine->link_by_name_or_null(elm.second);
    surf_parse_assert(link, std::string("<trace_connect kind=\"BANDWIDTH\">: Link ") + elm.second + " undefined.");
    link->set_bandwidth_profile(profile);
  }

  for (auto const& elm : trace_connect_list_link_lat) {
    surf_parse_assert(traces_set_list.find(elm.first) != traces_set_list.end(),
                      std::string("<trace_connect kind=\"LATENCY\">: Trace ") + elm.first + " undefined.");
    auto profile = traces_set_list.at(elm.first);

    auto link = engine->link_by_name_or_null(elm.second);
    surf_parse_assert(link, std::string("<trace_connect kind=\"LATENCY\">: Link ") + elm.second + " undefined.");
    link->set_latency_profile(profile);
  }

  surf_parse_close();
}
