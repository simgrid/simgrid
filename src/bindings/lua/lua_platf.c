/* Copyright (c) 2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* SimGrid Lua bindings                                                     */

#include "lua_private.h"
#include "simgrid/platf_interface.h"
#include "surf/surfxml_parse.h"
#include "surf/surf_routing.h"
#include <string.h>
#include <ctype.h>
#include <lauxlib.h>

#include "src/msg/msg_private.h"
#include "src/simix/smx_host_private.h"
#include "src/surf/surf_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(lua_platf, bindings, "Lua bindings (platform module)");

#define PLATF_MODULE_NAME "simgrid.platf"

/* ********************************************************************************* */
/*                               simgrid.platf API                                   */
/* ********************************************************************************* */

static const luaL_Reg platf_functions[] = {
    {"open", console_open},
    {"close", console_close},
    {"AS_open", console_AS_open},
    {"AS_close", console_AS_close},
    {"backbone_new", console_add_backbone},
    {"host_link_new", console_add_host___link},
    {"host_new", console_add_host},
    {"link_new", console_add_link},
    {"router_new", console_add_router},
    {"route_new", console_add_route},
    {"ASroute_new", console_add_ASroute},
    {NULL, NULL}
};

int console_open(lua_State *L) {
  sg_platf_init();
  sg_platf_begin();

  storage_register_callbacks();
  routing_register_callbacks();

  return 0;
}

int console_close(lua_State *L) {
  sg_platf_end();
  sg_platf_exit();
  return 0;
}

int console_add_backbone(lua_State *L) {
  s_sg_platf_link_cbarg_t link;
  memset(&link,0,sizeof(link));
  int type;

  link.properties = NULL;

  if (!lua_istable(L, -1)) {
    XBT_ERROR
        ("Bad Arguments to create backbone in Lua. Should be a table with named arguments.");
    return -1;
  }

  lua_pushstring(L, "id");
  type = lua_gettable(L, -2);
  if (type != LUA_TSTRING) {
    XBT_ERROR("Attribute 'id' must be specified for backbone and must be a string.");
  }
  link.id = lua_tostring(L, -1);
  lua_pop(L, 1);

  lua_pushstring(L, "bandwidth");
  type = lua_gettable(L, -2);
  if (type != LUA_TSTRING && type != LUA_TNUMBER) {
    XBT_ERROR("Attribute 'bandwidth' must be specified for backbone and must either be a string (in the right format; see docs) or a number.");
  }
  link.bandwidth = surf_parse_get_bandwidth(lua_tostring(L, -1));
  lua_pop(L, 1);

  lua_pushstring(L, "lat");
  type = lua_gettable(L, -2);
  if (type != LUA_TSTRING && type != LUA_TNUMBER) {
    XBT_ERROR("Attribute 'lat' must be specified for backbone and must either be a string (in the right format; see docs) or a number.");
  }
  link.latency = surf_parse_get_time(lua_tostring(L, -1));
  lua_pop(L, 1);

  link.initiallyOn = 1;

  lua_pushstring(L, "sharing_policy");
  type = lua_gettable(L, -2);
  const char* policy = lua_tostring(L, -1);
  if (policy && !strcmp(policy,"FULLDUPLEX")) {
    link.policy = SURF_LINK_FULLDUPLEX;
  } else if (policy && !strcmp(policy,"FATPIPE")) {
    link.policy = SURF_LINK_FATPIPE;
  } else {
    link.policy = SURF_LINK_SHARED;
  }

  sg_platf_new_link(&link);
  routing_cluster_add_backbone(sg_link_by_name(link.id));

  return 0;
}

int console_add_host___link(lua_State *L) {
  s_sg_platf_host_link_cbarg_t netcard;
  memset(&netcard,0,sizeof(netcard));
  int type;

  // we get values from the table passed as argument
  if (!lua_istable(L, -1)) {
    XBT_ERROR
        ("Bad Arguments to create host_link in Lua. Should be a table with named arguments.");
    return -1;
  }

  lua_pushstring(L, "id");
  type = lua_gettable(L, -2);
  if (type != LUA_TSTRING) {
    XBT_ERROR("Attribute 'id' must be specified for any host_link and must be a string.");
  }
  netcard.id = lua_tostring(L, -1);
  lua_pop(L, 1);

  lua_pushstring(L, "up");
  type = lua_gettable(L, -2);
  if (type != LUA_TSTRING && type != LUA_TNUMBER) {
    XBT_ERROR("Attribute 'up' must be specified for host_link and must either be a string or a number.");
  }
  netcard.link_up = lua_tostring(L, -1);
  lua_pop(L, 1);

  lua_pushstring(L, "down");
  type = lua_gettable(L, -2);
  if (type != LUA_TSTRING && type != LUA_TNUMBER) {
    XBT_ERROR("Attribute 'down' must be specified for host_link and must either be a string or a number.");
  }
  netcard.link_down = lua_tostring(L, -1);
  lua_pop(L, 1);

  XBT_DEBUG("Create a host_link for host %s", netcard.id);
  sg_platf_new_netcard(&netcard);

  return 0;
}

int console_add_host(lua_State *L) {
  s_sg_platf_host_cbarg_t host;
  memset(&host,0,sizeof(host));
  int state, type;

  // we get values from the table passed as argument
  if (!lua_istable(L, -1)) {
    XBT_ERROR
        ("Bad Arguments to create host. Should be a table with named arguments");
    return -1;
  }

  // get Id Value
  lua_pushstring(L, "id");
  type = lua_gettable(L, -2);
  if (type != LUA_TSTRING && type != LUA_TNUMBER) {
    XBT_ERROR("Attribute 'id' must be specified for any host and must be a string.");
  }
  host.id = lua_tostring(L, -1);
  lua_pop(L, 1);

  // get power value
  lua_pushstring(L, "power");
  type = lua_gettable(L, -2);
  if (type != LUA_TSTRING && type != LUA_TNUMBER) {
    XBT_ERROR("Attribute 'power' must be specified for host and must either be a string (in the correct format; check documentation) or a number.");
  }
  host.speed_peak = xbt_dynar_new(sizeof(double), NULL);
  xbt_dynar_push_as(host.speed_peak, double, get_cpu_speed(lua_tostring(L, -1)));
  lua_pop(L, 1);

  // get core
  lua_pushstring(L, "core");
  lua_gettable(L, -2);
  if(!lua_isnumber(L,-1)) {
      host.core_amount = 1;// Default value
  }
  else host.core_amount = lua_tonumber(L, -1);
  if (host.core_amount == 0)
    host.core_amount = 1;
  lua_pop(L, 1);

  //get power_scale
  lua_pushstring(L, "availability");
  lua_gettable(L, -2);
  if(!lua_isnumber(L,-1)) host.speed_scale = 1;// Default value
  else host.speed_scale = lua_tonumber(L, -1);
  lua_pop(L, 1);

  //get power_trace
  lua_pushstring(L, "availability_file");
  lua_gettable(L, -2);
  host.speed_trace = tmgr_trace_new_from_file(lua_tostring(L, -1));
  lua_pop(L, 1);

  //get state initial
  lua_pushstring(L, "state");
  lua_gettable(L, -2);
  if(!lua_isnumber(L,-1)) state = 1;// Default value
  else state = lua_tonumber(L, -1);
  lua_pop(L, 1);

  if (state)
    host.initiallyOn = 1;
  else
    host.initiallyOn = 0;

  //get trace state
  lua_pushstring(L, "state_file");
  lua_gettable(L, -2);
  host.state_trace = tmgr_trace_new_from_file(lua_tostring(L, -1));
  lua_pop(L, 1);

  sg_platf_new_host(&host);
  xbt_dynar_free(&host.speed_peak);

  return 0;
}

int  console_add_link(lua_State *L) {
  s_sg_platf_link_cbarg_t link;
  memset(&link,0,sizeof(link));

  int type;
  const char* policy;

  if (! lua_istable(L, -1)) {
    XBT_ERROR("Bad Arguments to create link, Should be a table with named arguments");
    return -1;
  }

  // get Id Value
  lua_pushstring(L, "id");
  type = lua_gettable(L, -2);
  if (type != LUA_TSTRING && type != LUA_TNUMBER) {
    XBT_ERROR("Attribute 'id' must be specified for any link and must be a string.");
  }
  link.id = lua_tostring(L, -1);
  lua_pop(L, 1);

  // get bandwidth value
  lua_pushstring(L, "bandwidth");
  type = lua_gettable(L, -2);
  if (type != LUA_TSTRING && type != LUA_TNUMBER) {
    XBT_ERROR("Attribute 'bandwidth' must be specified for any link and must either be either a string (in the right format; see docs) or a number.");
  }
  link.bandwidth = surf_parse_get_bandwidth(lua_tostring(L, -1));
  lua_pop(L, 1);

  //get latency value
  lua_pushstring(L, "lat");
  type = lua_gettable(L, -2);
  if (type != LUA_TSTRING && type != LUA_TNUMBER) {
    XBT_ERROR("Attribute 'lat' must be specified for any link and must either be a string (in the right format; see docs) or a number.");
  }
  link.latency = surf_parse_get_time(lua_tostring(L, -1));
  lua_pop(L, 1);

  /*Optional Arguments  */

  //get bandwidth_trace value
  lua_pushstring(L, "bandwidth_file");
  lua_gettable(L, -2);
  link.bandwidth_trace = tmgr_trace_new_from_file(lua_tostring(L, -1));
  lua_pop(L, 1);

  //get latency_trace value
  lua_pushstring(L, "latency_file");
  lua_gettable(L, -2);
  link.latency_trace = tmgr_trace_new_from_file(lua_tostring(L, -1));
  lua_pop(L, 1);

  //get state_trace value
  lua_pushstring(L, "state_file");
  lua_gettable(L, -2);
  link.state_trace = tmgr_trace_new_from_file(lua_tostring(L, -1));
  lua_pop(L, 1);

  //get state_initial value
  lua_pushstring(L, "state");
  lua_gettable(L, -2);
  if (!lua_isnumber(L,-1) || lua_tonumber(L, -1))
    link.initiallyOn = 1;
  else
    link.initiallyOn = 0;
  lua_pop(L, 1);

  //get policy value
  lua_pushstring(L, "sharing_policy");
  lua_gettable(L, -2);
  policy = lua_tostring(L, -1);
  lua_pop(L, 1);
  if (policy && !strcmp(policy,"FULLDUPLEX")) {
    link.policy = SURF_LINK_FULLDUPLEX;
  } else if (policy && !strcmp(policy,"FATPIPE")) {
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
  s_sg_platf_router_cbarg_t router;
  memset(&router,0,sizeof(router));
  int type;

  if (! lua_istable(L, -1)) {
    XBT_ERROR("Bad Arguments to create router, Should be a table with named arguments");
    return -1;
  }

  lua_pushstring(L, "id");
  type = lua_gettable(L, -2);
  if (type != LUA_TSTRING) {
    XBT_ERROR("Attribute 'id' must be specified for any link and must be a string.");
  }
  router.id = lua_tostring(L, -1);
  lua_pop(L,1);

  lua_pushstring(L,"coord");
  lua_gettable(L,-2);
  router.coord = lua_tostring(L, -1);
  lua_pop(L,1);

  sg_platf_new_router(&router);

  return 0;
}

#include "surf/surfxml_parse.h" /* to override surf_parse and bypass the parser */

int console_add_route(lua_State *L) {
  XBT_DEBUG("Adding route");
  s_sg_platf_route_cbarg_t route;
  memset(&route,0,sizeof(route));
  int type;

  /* allocating memory for the buffer, I think 2kB should be enough */
  surfxml_bufferstack = xbt_new0(char, surfxml_bufferstack_size);

  if (! lua_istable(L, -1)) {
    XBT_ERROR("Bad Arguments to create a route. Should be a table with named arguments");
    return -1;
  }

  lua_pushstring(L,"src");
  type = lua_gettable(L,-2);
  if (type != LUA_TSTRING) {
    XBT_ERROR("Attribute 'src' must be specified for any route and must be a string.");
  }
  route.src = lua_tostring(L, -1);
  lua_pop(L,1);

  lua_pushstring(L,"dest");
  type = lua_gettable(L,-2);
  if (type != LUA_TSTRING) {
    XBT_ERROR("Attribute 'dest' must be specified for any route and must be a string.");
  }
  route.dst = lua_tostring(L, -1);
  lua_pop(L,1);

  lua_pushstring(L,"links");
  type = lua_gettable(L,-2);
  if (type != LUA_TSTRING) {
    XBT_ERROR("Attribute 'links' must be specified for any route and must be a string (different links separated by commas or single spaces.");
  }
  route.link_list = xbt_str_split(lua_tostring(L, -1), ", \t\r\n");
  if (xbt_dynar_is_empty(route.link_list))
    xbt_dynar_push_as(route.link_list,char*,xbt_strdup(lua_tostring(L, -1)));
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
    if (strcmp("YES", value) == 0) {
      route.symmetrical = TRUE;
    }
    else
      route.symmetrical = FALSE;
  }
  else {
    route.symmetrical = TRUE;
  }
  lua_pop(L,1);

  route.gw_src = NULL;
  route.gw_dst = NULL;

  sg_platf_new_route(&route);

  return 0;
}

int console_add_ASroute(lua_State *L) {
  s_sg_platf_route_cbarg_t ASroute;
  memset(&ASroute,0,sizeof(ASroute));

  lua_pushstring(L, "src");
  lua_gettable(L, -2);
  ASroute.src = lua_tostring(L, -1);
  lua_pop(L, 1);

  lua_pushstring(L, "dst");
  lua_gettable(L, -2);
  ASroute.dst = lua_tostring(L, -1);
  lua_pop(L, 1);

  lua_pushstring(L, "gw_src");
  lua_gettable(L, -2);
  ASroute.gw_src = sg_netcard_by_name_or_null(lua_tostring(L, -1));
  lua_pop(L, 1);

  lua_pushstring(L, "gw_dst");
  lua_gettable(L, -2);
  ASroute.gw_dst = sg_netcard_by_name_or_null(lua_tostring(L, -1));
  lua_pop(L, 1);

  /*if (A_surfxml_ASroute_gw___src && !ASroute.gw_src)*/
    /*surf_parse_error("gw_src=\"%s\" not found for ASroute from \"%s\" to \"%s\"",*/
                     /*A_surfxml_ASroute_gw___src, ASroute.src, ASroute.dst);*/
  /*if (A_surfxml_ASroute_gw___dst && !ASroute.gw_dst)*/
    /*surf_parse_error("gw_dst=\"%s\" not found for ASroute from \"%s\" to \"%s\"",*/
                     /*A_surfxml_ASroute_gw___dst, ASroute.src, ASroute.dst);*/

  lua_pushstring(L,"links");
  lua_gettable(L,-2);
  ASroute.link_list = xbt_str_split(lua_tostring(L, -1), ", \t\r\n");
  if (xbt_dynar_is_empty(ASroute.link_list))
    xbt_dynar_push_as(ASroute.link_list,char*,xbt_strdup(lua_tostring(L, -1)));
  lua_pop(L,1);

  lua_pushstring(L,"symmetrical");
  lua_gettable(L,-2);
  if (lua_isstring(L, -1)) {
    const char* value = lua_tostring(L, -1);
    if (strcmp("YES", value) == 0) {
      ASroute.symmetrical = TRUE;
    }
    else
      ASroute.symmetrical = FALSE;
  }
  else {
    ASroute.symmetrical = TRUE;
  }
  lua_pop(L,1);

  sg_platf_new_ASroute(&ASroute);

  return 0;
}

int console_AS_open(lua_State *L) {
 const char *id;
 const char *mode;
 int type;

 XBT_DEBUG("Opening AS");

 if (! lua_istable(L, 1)) {
   XBT_ERROR("Bad Arguments to AS_open, Should be a table with named arguments");
   return -1;
 }

 lua_pushstring(L, "id");
 type = lua_gettable(L, -2);
  if (type != LUA_TSTRING) {
    XBT_ERROR("Attribute 'id' must be specified for any AS and must be a string.");
  }
 id = lua_tostring(L, -1);
 lua_pop(L, 1);

 lua_pushstring(L, "mode");
 lua_gettable(L, -2);
 mode = lua_tostring(L, -1);
 lua_pop(L, 1);

 int mode_int = A_surfxml_AS_routing_None;
 if(!strcmp(mode,"Full")) mode_int = A_surfxml_AS_routing_Full;
 else if(!strcmp(mode,"Floyd")) mode_int = A_surfxml_AS_routing_Floyd;
 else if(!strcmp(mode,"Dijkstra")) mode_int = A_surfxml_AS_routing_Dijkstra;
 else if(!strcmp(mode,"DijkstraCache")) mode_int = A_surfxml_AS_routing_DijkstraCache;
 else if(!strcmp(mode,"Vivaldi")) mode_int = A_surfxml_AS_routing_Vivaldi;
 else if(!strcmp(mode,"Cluster")) mode_int = A_surfxml_AS_routing_Cluster;
 else if(!strcmp(mode,"none")) mode_int = A_surfxml_AS_routing_None;
 else xbt_die("Don't have the model name '%s'",mode);

 s_sg_platf_AS_cbarg_t AS = SG_PLATF_AS_INITIALIZER;
 AS.id = id;
 AS.routing = mode_int;

 sg_platf_new_AS_begin(&AS);

 return 0;
}
int console_AS_close(lua_State *L) {
  XBT_DEBUG("Closing AS");
  sg_platf_new_AS_end();
  return 0;
}

int console_set_function(lua_State *L) {

  const char *host_id ;
  const char *function_id;
  xbt_dynar_t args;

  if (! lua_istable(L, 1)) {
    XBT_ERROR("Bad Arguments to AS.new, Should be a table with named arguments");
    return -1;
  }

  // get Host id
  lua_pushstring(L, "host");
  lua_gettable(L, -2);
  host_id = lua_tostring(L, -1);
  lua_pop(L, 1);

  // get Function Name
  lua_pushstring(L, "fct");
  lua_gettable(L, -2);
  function_id = lua_tostring(L, -1);
  lua_pop(L, 1);

  //get args
  lua_pushstring(L,"args");
  lua_gettable(L, -2);
  args = xbt_str_split_str( lua_tostring(L,-1) , ",");
  lua_pop(L, 1);

  msg_host_t host = MSG_host_by_name(host_id);
  if (!host) {
    XBT_ERROR("no host '%s' found",host_id);
    return -1;
  }

  // FIXME: use sg_platf_new_process directly (warning: find a way to check hostname)
  MSG_set_function(host_id, function_id, args);

  return 0;
}

int console_host_set_property(lua_State *L) {
  const char* name ="";
  const char* prop_id = "";
  const char* prop_value = "";
  if (!lua_istable(L, -1)) {
    XBT_ERROR("Bad Arguments to create link, Should be a table with named arguments");
    return -1;
  }


  // get Host id
  lua_pushstring(L, "host");
  lua_gettable(L, -2);
  name = lua_tostring(L, -1);
  lua_pop(L, 1);

  // get prop Name
  lua_pushstring(L, "prop");
  lua_gettable(L, -2);
  prop_id = lua_tostring(L, -1);
  lua_pop(L, 1);
  //get args
  lua_pushstring(L,"value");
  lua_gettable(L, -2);
  prop_value = lua_tostring(L,-1);
  lua_pop(L, 1);

  msg_host_t host = MSG_host_by_name(name);
  if (!host) {
    XBT_ERROR("no host '%s' found",name);
    return -1;
  }
  xbt_dict_t props = MSG_host_get_properties(host);
  xbt_dict_set(props,prop_id,xbt_strdup(prop_value),NULL);

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
  lua_setfield(L, -2, "platf");    /* simgrid */

  lua_pop(L, 1);                   /* -- */
}

