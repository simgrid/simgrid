/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* SimGrid Lua bindings                                                     */

#include "lua_private.hpp"
#include "src/kernel/routing/NetPoint.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/xml/platf_private.hpp"
#include <cctype>
#include <cstring>

extern "C" {
#include <lauxlib.h>
}

#include "src/surf/surf_private.hpp"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <simgrid/s4u/Host.hpp>
#include <string>
#include <vector>

XBT_LOG_NEW_DEFAULT_CATEGORY(lua_platf, "Lua bindings (platform module)");

#define PLATF_MODULE_NAME "simgrid.engine"
#define AS_FIELDNAME   "__simgrid_as"

/* ********************************************************************************* */
/*                               simgrid.platf API                                   */
/* ********************************************************************************* */

static const luaL_Reg platf_functions[] = {
    {"open", console_open},
    {"close", console_close},
    {"AS_open", console_AS_open},
    {"AS_seal", console_AS_seal},
    {"backbone_new", console_add_backbone},
    {"host_link_new", console_add_host___link},
    {"host_new", console_add_host},
    {"link_new", console_add_link},
    {"router_new", console_add_router},
    {"route_new", console_add_route},
    {"ASroute_new", console_add_ASroute},
    {nullptr, nullptr}
};

int console_open(lua_State *L) {
  sg_platf_init();
  sg_platf_begin();

  storage_register_callbacks();

  return 0;
}

int console_close(lua_State *L) {
  sg_platf_end();
  sg_platf_exit();
  return 0;
}

int console_add_backbone(lua_State *L) {
  LinkCreationArgs link;

  link.properties = nullptr;

  lua_ensure(lua_istable(L, -1),"Bad Arguments to create backbone in Lua. Should be a table with named arguments.");

  lua_pushstring(L, "id");
  int type = lua_gettable(L, -2);
  lua_ensure(type == LUA_TSTRING, "Attribute 'id' must be specified for backbone and must be a string.");
  link.id = lua_tostring(L, -1);
  lua_pop(L, 1);

  lua_pushstring(L, "bandwidth");
  type = lua_gettable(L, -2);
  lua_ensure(type == LUA_TSTRING || type == LUA_TNUMBER,
      "Attribute 'bandwidth' must be specified for backbone and must either be a string (in the right format; see docs) or a number.");
  link.bandwidth = surf_parse_get_bandwidth(lua_tostring(L, -1), "bandwidth of backbone", link.id.c_str());
  lua_pop(L, 1);

  lua_pushstring(L, "lat");
  type = lua_gettable(L, -2);
  lua_ensure(type == LUA_TSTRING || type == LUA_TNUMBER,
      "Attribute 'lat' must be specified for backbone and must either be a string (in the right format; see docs) or a number.");
  link.latency = surf_parse_get_time(lua_tostring(L, -1), "latency of backbone", link.id.c_str());
  lua_pop(L, 1);

  lua_pushstring(L, "sharing_policy");
  lua_gettable(L, -2);
  const char* policy = lua_tostring(L, -1);
  lua_pop(L, 1);
  if (policy && not strcmp(policy, "FULLDUPLEX")) {
    link.policy = SURF_LINK_FULLDUPLEX;
  } else if (policy && not strcmp(policy, "FATPIPE")) {
    link.policy = SURF_LINK_FATPIPE;
  } else {
    link.policy = SURF_LINK_SHARED;
  }

  sg_platf_new_link(&link);
  routing_cluster_add_backbone(simgrid::surf::LinkImpl::byName(link.id));

  return 0;
}

int console_add_host___link(lua_State *L) {
  HostLinkCreationArgs hostlink;
  int type;

  lua_ensure(lua_istable(L, -1), "Bad Arguments to create host_link in Lua. Should be a table with named arguments.");

  lua_pushstring(L, "id");
  type = lua_gettable(L, -2);
  lua_ensure(type == LUA_TSTRING, "Attribute 'id' must be specified for any host_link and must be a string.");
  hostlink.id = lua_tostring(L, -1);
  lua_pop(L, 1);

  lua_pushstring(L, "up");
  type = lua_gettable(L, -2);
  lua_ensure(type == LUA_TSTRING || type == LUA_TNUMBER,
      "Attribute 'up' must be specified for host_link and must either be a string or a number.");
  hostlink.link_up = lua_tostring(L, -1);
  lua_pop(L, 1);

  lua_pushstring(L, "down");
  type = lua_gettable(L, -2);
  lua_ensure(type == LUA_TSTRING || type == LUA_TNUMBER,
      "Attribute 'down' must be specified for host_link and must either be a string or a number.");
  hostlink.link_down = lua_tostring(L, -1);
  lua_pop(L, 1);

  XBT_DEBUG("Create a host_link for host %s", hostlink.id.c_str());
  sg_platf_new_hostlink(&hostlink);

  return 0;
}

int console_add_host(lua_State *L) {
  s_sg_platf_host_cbarg_t host;
  int type;

  // we get values from the table passed as argument
  lua_ensure(lua_istable(L, -1),
      "Bad Arguments to create host. Should be a table with named arguments");

  // get Id Value
  lua_pushstring(L, "id");
  type = lua_gettable(L, -2);
  lua_ensure(type == LUA_TSTRING,
      "Attribute 'id' must be specified for any host and must be a string.");
  host.id = lua_tostring(L, -1);
  lua_pop(L, 1);

  // get power value
  lua_pushstring(L, "speed");
  type = lua_gettable(L, -2);
  lua_ensure(type == LUA_TSTRING || type == LUA_TNUMBER,
      "Attribute 'speed' must be specified for host and must either be a string (in the correct format; check documentation) or a number.");
  if (type == LUA_TNUMBER)
    host.speed_per_pstate.push_back(lua_tointeger(L, -1));
  else // LUA_TSTRING
    host.speed_per_pstate.push_back(surf_parse_get_speed(lua_tostring(L, -1), "speed of host", host.id));
  lua_pop(L, 1);

  // get core
  lua_pushstring(L, "core");
  lua_gettable(L, -2);
  if (not lua_isnumber(L, -1))
    host.core_amount = 1; // Default value
  else
    host.core_amount = lua_tonumber(L, -1);
  if (host.core_amount == 0)
    host.core_amount = 1;
  lua_pop(L, 1);

  //get power_trace
  lua_pushstring(L, "availability_file");
  lua_gettable(L, -2);
  const char *filename = lua_tostring(L, -1);
  if (filename)
    host.speed_trace = tmgr_trace_new_from_file(filename);
  lua_pop(L, 1);

  //get trace state
  lua_pushstring(L, "state_file");
  lua_gettable(L, -2);
  filename = lua_tostring(L, -1);
    if (filename)
      host.state_trace = tmgr_trace_new_from_file(filename);
  lua_pop(L, 1);

  sg_platf_new_host(&host);

  return 0;
}

int  console_add_link(lua_State *L) {
  LinkCreationArgs link;

  const char* policy;

  lua_ensure(lua_istable(L, -1), "Bad Arguments to create link, Should be a table with named arguments");

  // get Id Value
  lua_pushstring(L, "id");
  int type = lua_gettable(L, -2);
  lua_ensure(type == LUA_TSTRING || type == LUA_TNUMBER,
      "Attribute 'id' must be specified for any link and must be a string.");
  link.id = lua_tostring(L, -1);
  lua_pop(L, 1);

  // get bandwidth value
  lua_pushstring(L, "bandwidth");
  type = lua_gettable(L, -2);
  lua_ensure(type == LUA_TSTRING || type == LUA_TNUMBER,
      "Attribute 'bandwidth' must be specified for any link and must either be either a string (in the right format; see docs) or a number.");
  if (type == LUA_TNUMBER)
    link.bandwidth = lua_tonumber(L, -1);
  else // LUA_TSTRING
    link.bandwidth = surf_parse_get_bandwidth(lua_tostring(L, -1), "bandwidth of link", link.id.c_str());
  lua_pop(L, 1);

  //get latency value
  lua_pushstring(L, "lat");
  type = lua_gettable(L, -2);
  lua_ensure(type == LUA_TSTRING || type == LUA_TNUMBER,
      "Attribute 'lat' must be specified for any link and must either be a string (in the right format; see docs) or a number.");
  if (type == LUA_TNUMBER)
    link.latency = lua_tonumber(L, -1);
  else // LUA_TSTRING
    link.latency = surf_parse_get_time(lua_tostring(L, -1), "latency of link", link.id.c_str());
  lua_pop(L, 1);

  /*Optional Arguments  */

  //get bandwidth_trace value
  lua_pushstring(L, "bandwidth_file");
  lua_gettable(L, -2);
  const char *filename = lua_tostring(L, -1);
  if (filename)
    link.bandwidth_trace = tmgr_trace_new_from_file(filename);
  lua_pop(L, 1);

  //get latency_trace value
  lua_pushstring(L, "latency_file");
  lua_gettable(L, -2);
  filename = lua_tostring(L, -1);
  if (filename)
    link.latency_trace = tmgr_trace_new_from_file(filename);
  lua_pop(L, 1);

  //get state_trace value
  lua_pushstring(L, "state_file");
  lua_gettable(L, -2);
  filename = lua_tostring(L, -1);
  if (filename)
    link.state_trace = tmgr_trace_new_from_file(filename);
  lua_pop(L, 1);

  //get policy value
  lua_pushstring(L, "sharing_policy");
  lua_gettable(L, -2);
  policy = lua_tostring(L, -1);
  lua_pop(L, 1);
  if (policy && not strcmp(policy, "FULLDUPLEX")) {
    link.policy = SURF_LINK_FULLDUPLEX;
  } else if (policy && not strcmp(policy, "FATPIPE")) {
    link.policy = SURF_LINK_FATPIPE;
  } else {
    link.policy = SURF_LINK_SHARED;
  }

  sg_platf_new_link(&link);

  return 0;
}
/**
 * add Router to AS components
 */
int console_add_router(lua_State* L) {
  lua_ensure(lua_istable(L, -1), "Bad Arguments to create router, Should be a table with named arguments");

  lua_pushstring(L, "id");
  int type = lua_gettable(L, -2);
  lua_ensure(type == LUA_TSTRING, "Attribute 'id' must be specified for any link and must be a string.");
  const char* name = lua_tostring(L, -1);
  lua_pop(L,1);

  lua_pushstring(L,"coord");
  lua_gettable(L,-2);
  const char* coords = lua_tostring(L, -1);
  lua_pop(L,1);

  sg_platf_new_router(name, coords);

  return 0;
}

int console_add_route(lua_State *L) {
  XBT_DEBUG("Adding route");
  s_sg_platf_route_cbarg_t route;
  int type;

  lua_ensure(lua_istable(L, -1), "Bad Arguments to add a route. Should be a table with named arguments");

  lua_pushstring(L,"src");
  type = lua_gettable(L,-2);
  lua_ensure(type == LUA_TSTRING, "Attribute 'src' must be specified for any route and must be a string.");
  const char *srcName = lua_tostring(L, -1);
  route.src           = sg_netpoint_by_name_or_null(srcName);
  lua_ensure(route.src != nullptr, "Attribute 'src=%s' of route does not name a node.", srcName);
  lua_pop(L,1);

  lua_pushstring(L,"dest");
  type = lua_gettable(L,-2);
  lua_ensure(type == LUA_TSTRING, "Attribute 'dest' must be specified for any route and must be a string.");
  const char *dstName = lua_tostring(L, -1);
  route.dst           = sg_netpoint_by_name_or_null(dstName);
  lua_ensure(route.dst != nullptr, "Attribute 'dst=%s' of route does not name a node.", dstName);
  lua_pop(L,1);

  lua_pushstring(L,"links");
  type = lua_gettable(L,-2);
  lua_ensure(type == LUA_TSTRING,
      "Attribute 'links' must be specified for any route and must be a string (different links separated by commas or single spaces.");
  std::vector<std::string> names;
  const char* str = lua_tostring(L, -1);
  boost::split(names, str, boost::is_any_of(", \t\r\n"));
  if (names.empty()) {
    /* unique name */
    route.link_list.push_back(simgrid::surf::LinkImpl::byName(lua_tostring(L, -1)));
  } else {
    // Several names separated by , \t\r\n
    for (auto const& name : names) {
      if (name.length() > 0) {
        simgrid::surf::LinkImpl* link = simgrid::surf::LinkImpl::byName(name);
        route.link_list.push_back(link);
      }
    }
  }
  lua_pop(L,1);

  /* We are relying on the XML bypassing mechanism since the corresponding sg_platf does not exist yet.
   * Et ouais mon pote. That's the way it goes. F34R.
   *
   * (Note that above this function, there is a #include statement. Is this
   * comment related to that statement?)
   */
  lua_pushstring(L,"symmetrical");
  lua_gettable(L,-2);
  if (lua_isstring(L, -1)) {
    const char* value = lua_tostring(L, -1);
    if (strcmp("YES", value) == 0)
      route.symmetrical = true;
    else
      route.symmetrical = false;
  }
  else {
    route.symmetrical = true;
  }
  lua_pop(L,1);

  route.gw_src = nullptr;
  route.gw_dst = nullptr;

  sg_platf_new_route(&route);

  return 0;
}

int console_add_ASroute(lua_State *L) {
  s_sg_platf_route_cbarg_t ASroute;

  lua_pushstring(L, "src");
  lua_gettable(L, -2);
  const char *srcName = lua_tostring(L, -1);
  ASroute.src         = sg_netpoint_by_name_or_null(srcName);
  lua_ensure(ASroute.src != nullptr, "Attribute 'src=%s' of AS route does not name a node.", srcName);
  lua_pop(L, 1);

  lua_pushstring(L, "dst");
  lua_gettable(L, -2);
  const char *dstName = lua_tostring(L, -1);
  ASroute.dst         = sg_netpoint_by_name_or_null(dstName);
  lua_ensure(ASroute.dst != nullptr, "Attribute 'dst=%s' of AS route does not name a node.", dstName);
  lua_pop(L, 1);

  lua_pushstring(L, "gw_src");
  lua_gettable(L, -2);
  const char *name = lua_tostring(L, -1);
  ASroute.gw_src   = sg_netpoint_by_name_or_null(name);
  lua_ensure(ASroute.gw_src, "Attribute 'gw_src=%s' of AS route does not name a valid node", name);
  lua_pop(L, 1);

  lua_pushstring(L, "gw_dst");
  lua_gettable(L, -2);
  name = lua_tostring(L, -1);
  ASroute.gw_dst = sg_netpoint_by_name_or_null(name);
  lua_ensure(ASroute.gw_dst, "Attribute 'gw_dst=%s' of AS route does not name a valid node", name);
  lua_pop(L, 1);

  lua_pushstring(L,"links");
  lua_gettable(L,-2);
  std::vector<std::string> names;
  const char* str = lua_tostring(L, -1);
  boost::split(names, str, boost::is_any_of(", \t\r\n"));
  if (names.empty()) {
    /* unique name with no comma */
    ASroute.link_list.push_back(simgrid::surf::LinkImpl::byName(lua_tostring(L, -1)));
  } else {
    // Several names separated by , \t\r\n
    for (auto const& name : names) {
      if (name.length() > 0) {
        simgrid::surf::LinkImpl* link = simgrid::surf::LinkImpl::byName(name);
        ASroute.link_list.push_back(link);
      }
    }
  }
  lua_pop(L,1);

  lua_pushstring(L,"symmetrical");
  lua_gettable(L,-2);
  if (lua_isstring(L, -1)) {
    const char* value = lua_tostring(L, -1);
    if (strcmp("YES", value) == 0)
      ASroute.symmetrical = true;
    else
      ASroute.symmetrical = false;
  }
  else {
    ASroute.symmetrical = true;
  }
  lua_pop(L,1);

  sg_platf_new_route(&ASroute);

  return 0;
}

int console_AS_open(lua_State *L) {
 XBT_DEBUG("Opening AS");

 lua_ensure(lua_istable(L, 1), "Bad Arguments to AS_open, Should be a table with named arguments");

 lua_pushstring(L, "id");
 int type = lua_gettable(L, -2);
 lua_ensure(type == LUA_TSTRING, "Attribute 'id' must be specified for any AS and must be a string.");
 const char* id = lua_tostring(L, -1);
 lua_pop(L, 1);

 lua_pushstring(L, "mode");
 lua_gettable(L, -2);
 const char* mode = lua_tostring(L, -1);
 lua_pop(L, 1);

 int mode_int = A_surfxml_AS_routing_None;
 if (not strcmp(mode, "Full"))
   mode_int = A_surfxml_AS_routing_Full;
 else if (not strcmp(mode, "Floyd"))
   mode_int = A_surfxml_AS_routing_Floyd;
 else if (not strcmp(mode, "Dijkstra"))
   mode_int = A_surfxml_AS_routing_Dijkstra;
 else if (not strcmp(mode, "DijkstraCache"))
   mode_int = A_surfxml_AS_routing_DijkstraCache;
 else if (not strcmp(mode, "Vivaldi"))
   mode_int = A_surfxml_AS_routing_Vivaldi;
 else if (not strcmp(mode, "Cluster"))
   mode_int = A_surfxml_AS_routing_Cluster;
 else if (not strcmp(mode, "none"))
   mode_int = A_surfxml_AS_routing_None;
 else xbt_die("Don't have the model name '%s'",mode);

 ZoneCreationArgs AS;
 AS.id = id;
 AS.routing = mode_int;
 simgrid::s4u::NetZone* new_as = sg_platf_new_Zone_begin(&AS);

 /* Build a Lua representation of the new AS on the stack */
 lua_newtable(L);
 simgrid::s4u::NetZone** lua_as =
     (simgrid::s4u::NetZone**)lua_newuserdata(L, sizeof(simgrid::s4u::NetZone*)); /* table userdatum */
 *lua_as = new_as;
 luaL_getmetatable(L, PLATF_MODULE_NAME); /* table userdatum metatable */
 lua_setmetatable(L, -2);                 /* table userdatum */
 lua_setfield(L, -2, AS_FIELDNAME);       /* table -- put the userdata as field of the table */

 return 1;
}
int console_AS_seal(lua_State *L) {
  XBT_DEBUG("Sealing AS");
  sg_platf_new_Zone_seal();
  return 0;
}

int console_host_set_property(lua_State *L) {
  lua_ensure(lua_istable(L, -1), "Bad Arguments to create link, Should be a table with named arguments");

  // get Host id
  lua_pushstring(L, "host");
  lua_gettable(L, -2);
  const char* name = lua_tostring(L, -1);
  lua_pop(L, 1);

  // get prop Name
  lua_pushstring(L, "prop");
  lua_gettable(L, -2);
  const char* prop_id = lua_tostring(L, -1);
  lua_pop(L, 1);
  //get args
  lua_pushstring(L,"value");
  lua_gettable(L, -2);
  const char* prop_value = lua_tostring(L, -1);
  lua_pop(L, 1);

  sg_host_t host = sg_host_by_name(name);
  lua_ensure(host, "no host '%s' found",name);
  host->setProperty(prop_id, prop_value);

  return 0;
}

/**
 * \brief Registers the platform functions into the table simgrid.platf.
 * \param L a lua state
 */
void sglua_register_platf_functions(lua_State* L)
{
  lua_getglobal(L, "simgrid");     /* simgrid */
  luaL_newlib(L, platf_functions); /* simgrid simgrid.platf */
  lua_setfield(L, -2, "engine");   /* simgrid */

  lua_pop(L, 1);                   /* -- */
}
