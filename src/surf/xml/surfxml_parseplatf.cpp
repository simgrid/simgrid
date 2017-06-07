/* Copyright (c) 2006-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/instr/instr_private.h" // TRACE_start(). FIXME: remove by subscribing tracing to the surf signals
#include "src/surf/cpu_interface.hpp"
#include "src/surf/network_interface.hpp"
#include "xbt/dict.h"
#include "xbt/log.h"
#include "xbt/misc.h"
#include "xbt/str.h"
#include <vector>

#include "src/surf/xml/platf_private.hpp"

#if SIMGRID_HAVE_LUA
extern "C" {
#include "src/bindings/lua/simgrid_lua.h"

#include <lua.h>                /* Always include this when calling Lua */
#include <lauxlib.h>            /* Always include this when calling Lua */
#include <lualib.h>             /* Prototype for luaL_openlibs(), */
}
#endif

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_parse);

SG_BEGIN_DECL()

/* Trace related stuff */

xbt_dict_t traces_set_list = nullptr;
XBT_PRIVATE std::unordered_map<std::string, std::string> trace_connect_list_host_avail;
XBT_PRIVATE std::unordered_map<std::string, std::string> trace_connect_list_host_speed;
XBT_PRIVATE std::unordered_map<std::string, std::string> trace_connect_list_link_avail;
XBT_PRIVATE std::unordered_map<std::string, std::string> trace_connect_list_link_bw;
XBT_PRIVATE std::unordered_map<std::string, std::string> trace_connect_list_link_lat;

void sg_platf_trace_connect(sg_platf_trace_connect_cbarg_t trace_connect)
{
  xbt_assert(xbt_dict_get_or_null(traces_set_list, trace_connect->trace),
             "Cannot connect trace %s to %s: trace unknown", trace_connect->trace, trace_connect->element);

  switch (trace_connect->kind) {
  case SURF_TRACE_CONNECT_KIND_HOST_AVAIL:
    trace_connect_list_host_avail.insert({trace_connect->trace, trace_connect->element});
    break;
  case SURF_TRACE_CONNECT_KIND_SPEED:
    trace_connect_list_host_speed.insert({trace_connect->trace, trace_connect->element});
    break;
  case SURF_TRACE_CONNECT_KIND_LINK_AVAIL:
    trace_connect_list_link_avail.insert({trace_connect->trace, trace_connect->element});
    break;
  case SURF_TRACE_CONNECT_KIND_BANDWIDTH:
    trace_connect_list_link_bw.insert({trace_connect->trace, trace_connect->element});
    break;
  case SURF_TRACE_CONNECT_KIND_LATENCY:
    trace_connect_list_link_lat.insert({trace_connect->trace, trace_connect->element});
    break;
  default:
    surf_parse_error("Cannot connect trace %s to %s: kind of trace unknown", trace_connect->trace,
                     trace_connect->element);
    break;
  }
}

static int after_config_done;
void parse_after_config() {
  if (not after_config_done) {
    TRACE_start();

    /* Register classical callbacks */
    storage_register_callbacks();

    after_config_done = 1;
  }
}

/* This function acts as a main in the parsing area. */
void parse_platform_file(const char *file)
{
#if SIMGRID_HAVE_LUA
  int len    = (file == nullptr ? 0 : strlen(file));
  int is_lua = (file != nullptr && len > 3 && file[len - 3] == 'l' && file[len - 2] == 'u' && file[len - 1] == 'a');
#endif

  sg_platf_init();

#if SIMGRID_HAVE_LUA
  /* Check if file extension is "lua". If so, we will use
   * the lua bindings to parse the platform file (since it is
   * written in lua). If not, we will use the (old?) XML parser
   */
  if (is_lua) {
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    luaL_loadfile(L, file); // This loads the file without executing it.

    /* Run the script */
    if (lua_pcall(L, 0, 0, 0)) {
      XBT_ERROR("FATAL ERROR:\n  %s: %s\n\n", "Lua call failed. Error message:", lua_tostring(L, -1));
      xbt_die("Lua call failed. See Log");
    }
    lua_close(L);
  }
  else
#endif
  { // Use XML parser

    int parse_status;

    /* init the flex parser */
    after_config_done = 0;
    surf_parse_open(file);

    traces_set_list = xbt_dict_new_homogeneous(nullptr);

    /* Do the actual parsing */
    parse_status = surf_parse();

    /* connect all traces relative to hosts */
    for (auto elm : trace_connect_list_host_avail) {
      tmgr_trace_t trace = (tmgr_trace_t)xbt_dict_get_or_null(traces_set_list, elm.first.c_str());
      xbt_assert(trace, "Trace %s undefined", elm.first.c_str());

      simgrid::s4u::Host* host = sg_host_by_name(elm.second.c_str());
      xbt_assert(host, "Host %s undefined", elm.second.c_str());
      simgrid::surf::Cpu *cpu = host->pimpl_cpu;

      cpu->setStateTrace(trace);
    }

    for (auto elm : trace_connect_list_host_speed) {
      tmgr_trace_t trace = (tmgr_trace_t)xbt_dict_get_or_null(traces_set_list, elm.first.c_str());
      xbt_assert(trace, "Trace %s undefined", elm.first.c_str());

      simgrid::s4u::Host* host = sg_host_by_name(elm.second.c_str());
      xbt_assert(host, "Host %s undefined", elm.second.c_str());
      simgrid::surf::Cpu *cpu = host->pimpl_cpu;

      cpu->setSpeedTrace(trace);
    }

    for (auto elm : trace_connect_list_link_avail) {
      tmgr_trace_t trace = (tmgr_trace_t)xbt_dict_get_or_null(traces_set_list, elm.first.c_str());
      xbt_assert(trace, "Trace %s undefined", elm.first.c_str());

      sg_link_t link = simgrid::s4u::Link::byName(elm.second.c_str());
      xbt_assert(link, "Link %s undefined", elm.second.c_str());
      link->setStateTrace(trace);
    }

    for (auto elm : trace_connect_list_link_bw) {
      tmgr_trace_t trace = (tmgr_trace_t)xbt_dict_get_or_null(traces_set_list, elm.first.c_str());
      xbt_assert(trace, "Trace %s undefined", elm.first.c_str());
      sg_link_t link = simgrid::s4u::Link::byName(elm.second.c_str());
      xbt_assert(link, "Link %s undefined", elm.second.c_str());
      link->setBandwidthTrace(trace);
    }

    for (auto elm : trace_connect_list_link_lat) {
      tmgr_trace_t trace = (tmgr_trace_t)xbt_dict_get_or_null(traces_set_list, elm.first.c_str());
      xbt_assert(trace, "Trace %s undefined", elm.first.c_str());
      sg_link_t link = simgrid::s4u::Link::byName(elm.second.c_str());
      xbt_assert(link, "Link %s undefined", elm.second.c_str());
      link->setLatencyTrace(trace);
    }

    /* Free my data */
    xbt_dict_free(&traces_set_list);

    surf_parse_close();

    if (parse_status)
      surf_parse_error("Parse error in %s", file);
  }
}

SG_END_DECL()
