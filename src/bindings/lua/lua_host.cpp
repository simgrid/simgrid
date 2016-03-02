/* Copyright (c) 2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* SimGrid Lua bindings                                                     */

#include "lua_private.h"
#include <simgrid/host.h>
extern "C" {
#include <lauxlib.h>
}

XBT_LOG_NEW_DEFAULT_CATEGORY(lua_host, "Lua bindings (host module)");

#define HOST_MODULE_NAME "simgrid.host"
#define HOST_FIELDNAME   "__simgrid_host"
/* ********************************************************************************* */
/*                                simgrid.host API                                   */
/* ********************************************************************************* */

/**
 * \brief Ensures that a value in the stack is a host and returns it.
 *
 * This function is called from C and helps to verify that a specific
 * userdatum on the stack is in fact a sg_host_t.
 *
 * \param L a Lua state
 * \param index an index in the Lua stack
 * \return the C host corresponding to this Lua host
 */
sg_host_t sglua_check_host(lua_State * L, int index)
{
  sg_host_t *pi, ht;
  luaL_checktype(L, index, LUA_TTABLE);
  lua_getfield(L, index, HOST_FIELDNAME);
  pi = (sg_host_t *) luaL_checkudata(L, lua_gettop(L), HOST_MODULE_NAME);
  if (pi == NULL)
    XBT_ERROR("luaL_checkudata() returned NULL");
  ht = *pi;
  if (!ht)
    luaL_error(L, "null Host");
  lua_pop(L, 1);
  return ht;
}

/**
 * \brief Returns a host given its name. This is a lua function.
 *
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Argument 1 (string): name of a host
 * - Return value (host): the corresponding host will be pushed onto the stack
 */
static int l_host_get_by_name(lua_State * L)
{
  const char *name = luaL_checkstring(L, 1);
  XBT_DEBUG("Getting host by name...");
  sg_host_t host = sg_host_by_name(name);
  if (!host) {
    XBT_ERROR("sg_get_host_by_name failed, requested hostname: %s", name);
  }
  lua_newtable(L);                        /* table */
  sg_host_t *lua_host = (sg_host_t *) lua_newuserdata(L, sizeof(sg_host_t));
                                          /* table userdatum */
  *lua_host = host;
  luaL_getmetatable(L, HOST_MODULE_NAME); /* table userdatum metatable */
  lua_setmetatable(L, -2);                /* table userdatum */
  lua_setfield(L, -2, HOST_FIELDNAME);  /* table -- put the userdata as field of the table */

  /* remove the args from the stack */
  lua_remove(L, 1);
  return 1;
}

/**
 * \brief Returns the name of a host.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Argument 1 (host): a host
 * - Return value (string): name of this host
 */
static int l_host_get_name(lua_State * L)
{
  sg_host_t ht = sglua_check_host(L, 1);
  lua_pushstring(L, sg_host_get_name(ht));
  return 1;
}

/**
 * \brief Returns the number of existing hosts.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Return value (number): number of hosts
 */
static int l_host_number(lua_State * L)
{
  xbt_dynar_t hosts = sg_hosts_as_dynar();
  lua_pushinteger(L, xbt_dynar_length(hosts));
  xbt_dynar_free(&hosts);
  return 1;
}

/**
 * \brief Returns the host given its index.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Argument 1 (number): an index (1 is the first)
 * - Return value (host): the host at this index
 */
static int l_host_at(lua_State * L)
{
  int index = luaL_checkinteger(L, 1);
  xbt_dynar_t hosts = sg_hosts_as_dynar();
  sg_host_t host = xbt_dynar_get_as(hosts,index - 1,sg_host_t);// lua indexing start by 1 (lua[1] <=> C[0])
  lua_newtable(L);              /* create a table, put the userdata on top of it */
  sg_host_t *lua_host = (sg_host_t *) lua_newuserdata(L, sizeof(sg_host_t));
  *lua_host = host;
  luaL_getmetatable(L, HOST_MODULE_NAME);
  lua_setmetatable(L, -2);
  lua_setfield(L, -2, HOST_FIELDNAME);        /* put the userdata as field of the table */
  xbt_dynar_free(&hosts);
  return 1;
}

/**
 * \brief Returns the value of a host property.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Argument 1 (host): a host
 * - Argument 2 (string): name of the property to get
 * - Return value (string): the value of this property
 */
static int l_host_get_property_value(lua_State * L)
{
  sg_host_t ht = sglua_check_host(L, 1);
  const char *prop = luaL_checkstring(L, 2);
  lua_pushstring(L, sg_host_get_property_value(ht,prop));
  return 1;
}

/**
 * \brief Destroys a host.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Argument 1 (host): the host to destroy
 */
static int l_host_destroy(lua_State *L)
{
  //sg_host_t ht = sglua_check_host(L, 1);
  //FIXME: not working..__MSG_host_priv_free(MSG_host_priv(ht));
  return 0;
}

static const luaL_Reg host_functions[] = {
  {"get_by_name", l_host_get_by_name},
  {"name", l_host_get_name},
  {"number", l_host_number},
  {"at", l_host_at},
  {"get_prop_value", l_host_get_property_value},
  {"destroy", l_host_destroy},
  // Bypass XML Methods
  {"set_property", console_host_set_property},
  {NULL, NULL}
};

/**
 * \brief Returns a string representation of a host.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Argument 1 (userdata): a host
 * - Return value (string): a string describing this host
 */
static int l_host_tostring(lua_State * L)
{
  lua_pushfstring(L, "Host :%p", lua_touserdata(L, 1));
  return 1;
}

static const luaL_Reg host_meta[] = {
  {"__tostring", l_host_tostring},
  {0, 0}
};

/**
 * \brief Registers the host functions into the table simgrid.host.
 *
 * Also initialize the metatable of the host userdata type.
 *
 * \param L a lua state
 */
void sglua_register_host_functions(lua_State* L)
{
  /* create a table simgrid.host and fill it with host functions */
  lua_getglobal(L, "simgrid"); /* simgrid */
  luaL_newlib(L, host_functions); /* simgrid simgrid.host */
  lua_setfield(L, -2, "host"); /* simgrid */
  lua_getfield(L, -1, "host");    /* simgrid simgrid.host */

  /* create the metatable for host, add it to the Lua registry */
  luaL_newmetatable(L, HOST_MODULE_NAME); /* simgrid simgrid.host mt */

  /* fill the metatable */
  luaL_setfuncs(L, host_meta, 0);         /* simgrid simgrid.host mt */

  /**
   * Copy the table and push it onto the stack.
   * Required for the lua_setfield call below.
   */
  lua_getfield(L, -3, "host");                   /* simgrid simgrid.host mt simgrid.host */

  /* metatable.__index = simgrid.host
   * we put the host functions inside the host userdata itself:
   * this allows to write my_host:method(args) for
   * simgrid.host.method(my_host, args) */
  lua_setfield(L, -2, "__index");         /* simgrid simgrid.host mt */

  lua_setmetatable(L, -2);                /* simgrid simgrid.host */
  lua_pop(L, 2);                          /* -- */
}

