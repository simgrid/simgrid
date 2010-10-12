/* SimGrid Lua Console                                                    */

/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid_lua.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(luax, bindings, "Lua Bindings");

static p_AS_attr AS;
//using xbt_dynar_t :
static xbt_dynar_t host_list_d;
static xbt_dynar_t link_list_d;
static xbt_dynar_t route_list_d;

/*
 * Initialize platform model routing
 */

static void create_AS(const char *id, const char *mode)
{
  surf_AS_new(id, mode);
}

/**
 * create host resource via CPU model [for MSG]
 */

static void create_host(const char *id, double power_peak, double power_sc,
                        const char *power_tr, int state_init,
                        const char *state_tr)
{

  double power_scale = 1.0;
  tmgr_trace_t power_trace = NULL;
  e_surf_resource_state_t state_initial;
  tmgr_trace_t state_trace;
  if (power_sc)                 // !=0
    power_scale = power_sc;
  if (state_init == -1)
    state_initial = SURF_RESOURCE_OFF;
  else
    state_initial = SURF_RESOURCE_ON;
  if (power_tr)
    power_trace = tmgr_trace_new(power_tr);
  else
    power_trace = tmgr_trace_new("");
  if (state_tr)
    state_trace = tmgr_trace_new(state_tr);
  else
    state_trace = tmgr_trace_new("");
  current_property_set = xbt_dict_new();
  surf_host_create_resource(xbt_strdup(id), power_peak, power_scale,
                            power_trace, state_initial, state_trace,
                            current_property_set);

}

/**
 * create link resource via network model
 */
static void create_link(const char *name,
                        double bw_initial, const char *trace,
                        double lat_initial, const char *latency_trace,
                        int state_init, const char *state_trace,
                        int policy)
{
  tmgr_trace_t bw_trace;
  tmgr_trace_t lat_trace;
  e_surf_resource_state_t state_initial_link = SURF_RESOURCE_ON;
  e_surf_link_sharing_policy_t policy_initial_link = SURF_LINK_SHARED;
  tmgr_trace_t st_trace;
  if (trace)
    bw_trace = tmgr_trace_new(trace);
  else
    bw_trace = tmgr_trace_new("");

  if (latency_trace)
    lat_trace = tmgr_trace_new(latency_trace);
  else
    lat_trace = tmgr_trace_new("");

  if (state_trace)
    st_trace = tmgr_trace_new(state_trace);
  else
    st_trace = tmgr_trace_new("");

  if (state_init == -1)
    state_initial_link = SURF_RESOURCE_OFF;
  if (policy == -1)
    policy_initial_link = SURF_LINK_FATPIPE;

  surf_link_create_resource(xbt_strdup(name), bw_initial, bw_trace,
                            lat_initial, lat_trace, state_initial_link,
                            st_trace, policy_initial_link, xbt_dict_new());
}


/*
 *create host resource via workstation_ptask_L07 model [for SimDag]
 */
static void create_host_wsL07(const char *id, double power_peak,
                              double power_sc, const char *power_tr,
                              int state_init, const char *state_tr)
{
  double power_scale = 1.0;
  tmgr_trace_t power_trace = NULL;
  e_surf_resource_state_t state_initial;
  tmgr_trace_t state_trace;
  if (power_sc)                 // !=0
    power_scale = power_sc;
  if (state_init == -1)
    state_initial = SURF_RESOURCE_OFF;
  else
    state_initial = SURF_RESOURCE_ON;
  if (power_tr)
    power_trace = tmgr_trace_new(power_tr);
  else
    power_trace = tmgr_trace_new("");
  if (state_tr)
    state_trace = tmgr_trace_new(state_tr);
  else
    state_trace = tmgr_trace_new("");
  current_property_set = xbt_dict_new();
  surf_wsL07_host_create_resource(xbt_strdup(id), power_peak, power_scale,
                                  power_trace, state_initial, state_trace,
                                  current_property_set);

}

/**
 * create link resource via workstation_ptask_L07 model [for SimDag]
 */

static void create_link_wsL07(const char *name,
                              double bw_initial, const char *trace,
                              double lat_initial,
                              const char *latency_trace, int state_init,
                              const char *state_trace, int policy)
{
  tmgr_trace_t bw_trace;
  tmgr_trace_t lat_trace;
  e_surf_resource_state_t state_initial_link = SURF_RESOURCE_ON;
  e_surf_link_sharing_policy_t policy_initial_link = SURF_LINK_SHARED;
  tmgr_trace_t st_trace;
  if (trace)
    bw_trace = tmgr_trace_new(trace);
  else
    bw_trace = tmgr_trace_new("");

  if (latency_trace)
    lat_trace = tmgr_trace_new(latency_trace);
  else
    lat_trace = tmgr_trace_new("");

  if (state_trace)
    st_trace = tmgr_trace_new(state_trace);
  else
    st_trace = tmgr_trace_new("");

  if (state_init == -1)
    state_initial_link = SURF_RESOURCE_OFF;
  if (policy == -1)
    policy_initial_link = SURF_LINK_FATPIPE;

  surf_wsL07_link_create_resource(xbt_strdup(name), bw_initial, bw_trace,
                                  lat_initial, lat_trace,
                                  state_initial_link, st_trace,
                                  policy_initial_link, xbt_dict_new());
}


/*
 * init AS
 */

static int AS_new(lua_State * L)
{
  const char *id;
  const char *mode;
  if (lua_istable(L, 1)) {
    lua_pushstring(L, "id");
    lua_gettable(L, -2);
    id = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_pushstring(L, "mode");
    lua_gettable(L, -2);
    mode = lua_tostring(L, -1);
    lua_pop(L, 1);
  } else {
    ERROR0
        ("Bad Arguments to AS.new, Should be a table with named arguments");
    return -1;
  }
  AS = malloc(sizeof(AS_attr));
  AS->id = id;
  AS->mode = mode;

  return 0;
}

/*
 * add new host to platform hosts list
 */
static int Host_new(lua_State * L)
{

  if (xbt_dynar_is_empty(host_list_d))
    host_list_d = xbt_dynar_new(sizeof(p_host_attr), &xbt_free_ref);

  p_host_attr host;
  const char *id;
  const char *power_trace;
  const char *state_trace;
  double power, power_scale;
  int state_initial;
  //get values from the table passed as argument
  if (lua_istable(L, -1)) {

    // get Id Value
    lua_pushstring(L, "id");
    lua_gettable(L, -2);
    id = lua_tostring(L, -1);
    lua_pop(L, 1);

    // get power value
    lua_pushstring(L, "power");
    lua_gettable(L, -2);
    power = lua_tonumber(L, -1);
    lua_pop(L, 1);

    //get power_scale
    lua_pushstring(L, "power_scale");
    lua_gettable(L, -2);
    power_scale = lua_tonumber(L, -1);
    lua_pop(L, 1);

    //get power_trace
    lua_pushstring(L, "power_trace");
    lua_gettable(L, -2);
    power_trace = lua_tostring(L, -1);
    lua_pop(L, 1);

    //get state initial
    lua_pushstring(L, "state_initial");
    lua_gettable(L, -2);
    state_initial = lua_tonumber(L, -1);
    lua_pop(L, 1);

    //get trace state
    lua_pushstring(L, "state_trace");
    lua_gettable(L, -2);
    state_trace = lua_tostring(L, -1);
    lua_pop(L, 1);

  } else {
    ERROR0
        ("Bad Arguments to create host, Should be a table with named arguments");
    return -1;
  }

  host = malloc(sizeof(host_attr));
  host->id = id;
  host->power_peak = power;
  host->power_scale = power_scale;
  host->power_trace = power_trace;
  host->state_initial = state_initial;
  host->state_trace = state_trace;
  host->function = NULL;
  xbt_dynar_push(host_list_d, &host);

  return 0;
}

/**
 * add link to platform links list
 */
static int Link_new(lua_State * L)      // (id,bandwidth,latency)
{
  if (xbt_dynar_is_empty(link_list_d))
    link_list_d = xbt_dynar_new(sizeof(p_link_attr), &xbt_free_ref);

  const char *id;
  double bandwidth, latency;
  const char *bandwidth_trace;
  const char *latency_trace;
  const char *state_trace;
  int state_initial, policy;

  //get values from the table passed as argument
  if (lua_istable(L, -1)) {
    // get Id Value
    lua_pushstring(L, "id");
    lua_gettable(L, -2);
    id = lua_tostring(L, -1);
    lua_pop(L, 1);

    // get bandwidth value
    lua_pushstring(L, "bandwidth");
    lua_gettable(L, -2);
    bandwidth = lua_tonumber(L, -1);
    lua_pop(L, 1);

    //get latency value
    lua_pushstring(L, "latency");
    lua_gettable(L, -2);
    latency = lua_tonumber(L, -1);
    lua_pop(L, 1);

    /*Optional Arguments  */

    //get bandwidth_trace value
    lua_pushstring(L, "bandwidth_trace");
    lua_gettable(L, -2);
    bandwidth_trace = lua_tostring(L, -1);
    lua_pop(L, 1);

    //get latency_trace value
    lua_pushstring(L, "latency_trace");
    lua_gettable(L, -2);
    latency_trace = lua_tostring(L, -1);
    lua_pop(L, 1);

    //get state_trace value
    lua_pushstring(L, "state_trace");
    lua_gettable(L, -2);
    state_trace = lua_tostring(L, -1);
    lua_pop(L, 1);

    //get state_initial value
    lua_pushstring(L, "state_initial");
    lua_gettable(L, -2);
    state_initial = lua_tonumber(L, -1);
    lua_pop(L, 1);


    //get policy value
    lua_pushstring(L, "policy");
    lua_gettable(L, -2);
    policy = lua_tonumber(L, -1);
    lua_pop(L, 1);

  } else {
    ERROR0
        ("Bad Arguments to create link, Should be a table with named arguments");
    return -1;
  }

  p_link_attr link = malloc(sizeof(link_attr));
  link->id = id;
  link->bandwidth = bandwidth;
  link->latency = latency;
  link->bandwidth_trace = bandwidth_trace;
  link->latency_trace = latency_trace;
  link->state_trace = state_trace;
  link->state_initial = state_initial;
  link->policy = policy;
  xbt_dynar_push(link_list_d, &link);
  return 0;
}

/**
 * add route to platform routes list
 */
static int Route_new(lua_State * L)     // (src_id,dest_id,links_number,link_table)
{
  if (xbt_dynar_is_empty(route_list_d))
    route_list_d = xbt_dynar_new(sizeof(p_route_attr), &xbt_free_ref);
  const char *link_id;
  p_route_attr route = malloc(sizeof(route_attr));
  route->src_id = luaL_checkstring(L, 1);
  route->dest_id = luaL_checkstring(L, 2);
  route->links_id = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
  lua_pushnil(L);
  while (lua_next(L, 3) != 0) {
    link_id = lua_tostring(L, -1);
    xbt_dynar_push(route->links_id, &link_id);
    DEBUG2("index = %f , Link_id = %s \n", lua_tonumber(L, -2),
           lua_tostring(L, -1));
    lua_pop(L, 1);
  }
  lua_pop(L, 1);

  //add route to platform's route list
  xbt_dynar_push(route_list_d, &route);
  return 0;
}

/**
 * set function to process
 */
static int Host_set_function(lua_State * L)     //(host,function,nb_args,list_args)
{
  // look for the index of host in host_list
  const char *host_id = luaL_checkstring(L, 1);
  const char *argument;
  unsigned int i;
  p_host_attr p_host;

  xbt_dynar_foreach(host_list_d, i, p_host) {
    if (p_host->id == host_id) {
      p_host->function = luaL_checkstring(L, 2);
      if (lua_istable(L, 3)) {
        p_host->args_list = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
        // fill the args list
        lua_pushnil(L);
        int j = 0;
        while (lua_next(L, 3) != 0) {
          argument = lua_tostring(L, -1);
          xbt_dynar_push(p_host->args_list, &argument);
          DEBUG2("index = %f , Arg_id = %s \n", lua_tonumber(L, -2),
                 lua_tostring(L, -1));
          j++;
          lua_pop(L, 1);
        }
      }
      lua_pop(L, 1);
      return 0;
    }
  }
  ERROR1("Host : %s Not Found !!", host_id);
  return 1;
}

/*
 * surf parse bypass platform
 * through CPU/network Models
 */

static int surf_parse_bypass_platform()
{
  unsigned int i;
  p_host_attr p_host;
  p_link_attr p_link;
  p_route_attr p_route;

  // Init routing mode
  create_AS(AS->id, AS->mode);


  // Add Hosts
  xbt_dynar_foreach(host_list_d, i, p_host) {
    create_host(p_host->id, p_host->power_peak, p_host->power_scale,
                p_host->power_trace, p_host->state_initial,
                p_host->state_trace);
    //add to routing model host list
    surf_route_add_host((char *) p_host->id);

  }

  //add Links
  xbt_dynar_foreach(link_list_d, i, p_link) {
    create_link(p_link->id, p_link->bandwidth, p_link->bandwidth_trace,
                p_link->latency, p_link->latency_trace,
                p_link->state_initial, p_link->state_trace,
                p_link->policy);
  }
  // add route
  xbt_dynar_foreach(route_list_d, i, p_route) {
    surf_routing_add_route((char *) p_route->src_id,
                           (char *) p_route->dest_id, p_route->links_id);
  }
  /* </platform> */

  // Finalize AS
  surf_AS_finalize(AS->id);

  // add traces
  surf_add_host_traces();
  surf_add_link_traces();

  return 0;                     // must return 0 ?!!

}

/**
 *
 * surf parse bypass platform
 * through workstation_ptask_L07 Model
 */

static int surf_wsL07_parse_bypass_platform()
{
  unsigned int i;
  p_host_attr p_host;
  p_link_attr p_link;
  p_route_attr p_route;

  // Init routing mode
  create_AS(AS->id, AS->mode);

  // Add Hosts
  xbt_dynar_foreach(host_list_d, i, p_host) {
    create_host_wsL07(p_host->id, p_host->power_peak, p_host->power_scale,
                      p_host->power_trace, p_host->state_initial,
                      p_host->state_trace);
    //add to routing model host list
    surf_route_add_host((char *) p_host->id);
  }

  //add Links
  xbt_dynar_foreach(link_list_d, i, p_link) {
    create_link_wsL07(p_link->id, p_link->bandwidth,
                      p_link->bandwidth_trace, p_link->latency,
                      p_link->latency_trace, p_link->state_initial,
                      p_link->state_trace, p_link->policy);
  }
  // add route
  xbt_dynar_foreach(route_list_d, i, p_route) {
    surf_routing_add_route((char *) p_route->src_id,
                           (char *) p_route->dest_id, p_route->links_id);
  }
  /* </platform> */

  // Finalize AS
  surf_AS_finalize(AS->id);
  // add traces
  surf_wsL07_add_traces();

  return 0;
}

/*
 * surf parse bypass application for MSG Module
 */
static int surf_parse_bypass_application()
{
  unsigned int i;
  p_host_attr p_host;
  xbt_dynar_foreach(host_list_d, i, p_host) {
    if (p_host->function)
      MSG_set_function(p_host->id, p_host->function, p_host->args_list);
  }
  return 0;
}


/*
 * Public Methods
 */

int console_add_host(lua_State *L)
{
	return Host_new(L);
}

int  console_add_link(lua_State *L)
{
	return Link_new(L);
}

int console_add_route(lua_State *L)
{
	return Route_new(L);
}

int console_add_AS(lua_State *L)
{
	return AS_new(L);
}

int console_set_function(lua_State *L)
{
	return Host_set_function(L);
}

int console_parse_platform()
{
	return surf_parse_bypass_platform();
}

int  console_parse_application()
{
	return surf_parse_bypass_application();
}

int console_parse_platform_wsL07()
{
	return surf_wsL07_parse_bypass_platform();
}
