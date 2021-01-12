/* Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf/surf.hpp"
#include "src/surf/cpu_interface.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/surf_interface.hpp"
#include "src/surf/xml/platf_private.hpp"

#include <vector>


#if SIMGRID_HAVE_LUA
#include "src/bindings/lua/simgrid_lua.hpp"

#include <lua.h>                /* Always include this when calling Lua */
#include <lauxlib.h>            /* Always include this when calling Lua */
#include <lualib.h>             /* Prototype for luaL_openlibs(), */
#endif

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
         std::string("Cannot connect trace ")+ trace_connect->trace+ " to "+trace_connect->element+": trace unknown");

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
  const char* cfile = file.c_str();
  size_t len        = strlen(cfile);
  bool is_lua       = len > 3 && file[len - 3] == 'l' && file[len - 2] == 'u' && file[len - 1] == 'a';

  sg_platf_init();

  /* Check if file extension is "lua". If so, we will use
   * the lua bindings to parse the platform file (since it is
   * written in lua). If not, we will use the (old?) XML parser
   */
  if (is_lua) {
#if SIMGRID_HAVE_LUA
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    luaL_loadfile(L, cfile); // This loads the file without executing it.

    /* Run the script */
    if (lua_pcall(L, 0, 0, 0)) {
      XBT_ERROR("FATAL ERROR:\n  %s: %s\n\n", "Lua call failed. Error message:", lua_tostring(L, -1));
      xbt_die("Lua call failed. See Log");
    }
    lua_close(L);
    return;
#else
    XBT_WARN("This looks like a lua platform file, but your SimGrid was not compiled with lua. Loading it as XML.");
#endif
  }

  // Use XML parser

  /* init the flex parser */
  surf_parse_open(file);

  /* Do the actual parsing */
  surf_parse();

  /* connect all profiles relative to hosts */
  for (auto const& elm : trace_connect_list_host_avail) {
    surf_parse_assert(traces_set_list.find(elm.first) != traces_set_list.end(), std::string("<trace_connect kind=\"HOST_AVAIL\">: Trace ")+elm.first+" undefined.");
    auto profile = traces_set_list.at(elm.first);

    auto host = simgrid::s4u::Host::by_name_or_null(elm.second);
    surf_parse_assert(host, std::string("<trace_connect kind=\"HOST_AVAIL\">: Host ") + elm.second + " undefined.");
    host->set_state_profile(profile);
  }

  for (auto const& elm : trace_connect_list_host_speed) {
    surf_parse_assert(traces_set_list.find(elm.first) != traces_set_list.end(), std::string("<trace_connect kind=\"SPEED\">: Trace ")+elm.first+" undefined.");
    auto profile = traces_set_list.at(elm.first);

    auto host = simgrid::s4u::Host::by_name_or_null(elm.second);
    surf_parse_assert(host, std::string("<trace_connect kind=\"SPEED\">: Host ") + elm.second + " undefined.");
    host->set_speed_profile(profile);
  }

  for (auto const& elm : trace_connect_list_link_avail) {
    surf_parse_assert(traces_set_list.find(elm.first) != traces_set_list.end(), std::string("<trace_connect kind=\"LINK_AVAIL\">: Trace ")+elm.first+" undefined.");
    auto profile = traces_set_list.at(elm.first);

    auto link = simgrid::s4u::Link::by_name_or_null(elm.second);
    surf_parse_assert(link, std::string("<trace_connect kind=\"LINK_AVAIL\">: Link ") + elm.second + " undefined.");
    link->set_state_profile(profile);
  }

  for (auto const& elm : trace_connect_list_link_bw) {
    surf_parse_assert(traces_set_list.find(elm.first) != traces_set_list.end(), std::string("<trace_connect kind=\"BANDWIDTH\">: Trace ")+elm.first+" undefined.");
    auto profile = traces_set_list.at(elm.first);

    auto link = simgrid::s4u::Link::by_name_or_null(elm.second);
    surf_parse_assert(link, std::string("<trace_connect kind=\"BANDWIDTH\">: Link ") + elm.second + " undefined.");
    link->set_bandwidth_profile(profile);
  }

  for (auto const& elm : trace_connect_list_link_lat) {
    surf_parse_assert(traces_set_list.find(elm.first) != traces_set_list.end(), std::string("<trace_connect kind=\"LATENCY\">: Trace ")+elm.first+" undefined.");
    auto profile = traces_set_list.at(elm.first);

    auto link = simgrid::s4u::Link::by_name_or_null(elm.second);
    surf_parse_assert(link, std::string("<trace_connect kind=\"LATENCY\">: Link ") + elm.second + " undefined.");
    link->set_latency_profile(profile);
  }

  surf_parse_close();
}
