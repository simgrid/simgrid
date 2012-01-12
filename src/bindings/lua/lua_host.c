/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* SimGrid Lua bindings                                                     */

#include "lua_private.h"
#include <lauxlib.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(lua_host, bindings, "Lua bindings (host module)");

#define HOST_MODULE_NAME "simgrid.host"

/* ********************************************************************************* */
/*                                simgrid.host API                                   */
/* ********************************************************************************* */

/**
 * \brief Ensures that a value in the stack is a host and returns it.
 * \param L a Lua state
 * \param index an index in the Lua stack
 * \return the C host corresponding to this Lua host
 */
m_host_t sglua_check_host(lua_State * L, int index)
{
  m_host_t *pi, ht;
  luaL_checktype(L, index, LUA_TTABLE);
  lua_getfield(L, index, "__simgrid_host");
  pi = (m_host_t *) luaL_checkudata(L, lua_gettop(L), HOST_MODULE_NAME);
  if (pi == NULL)
    luaL_typerror(L, index, HOST_MODULE_NAME);
  ht = *pi;
  if (!ht)
    luaL_error(L, "null Host");
  lua_pop(L, 1);
  return ht;
}

/**
 * \brief Returns a host given its name.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Argument 1 (string): name of a host
 * - Return value (host): the corresponding host
 */
static int l_host_get_by_name(lua_State * L)
{
  const char *name = luaL_checkstring(L, 1);
  XBT_DEBUG("Getting Host from name...");
  m_host_t msg_host = MSG_get_host_by_name(name);
  if (!msg_host) {
    luaL_error(L, "null Host : MSG_get_host_by_name failed");
  }
  lua_newtable(L);              /* create a table, put the userdata on top of it */
  m_host_t *lua_host = (m_host_t *) lua_newuserdata(L, sizeof(m_host_t));
  *lua_host = msg_host;
  luaL_getmetatable(L, HOST_MODULE_NAME);
  lua_setmetatable(L, -2);
  lua_setfield(L, -2, "__simgrid_host");        /* put the userdata as field of the table */
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
  m_host_t ht = sglua_check_host(L, 1);
  lua_pushstring(L, MSG_host_get_name(ht));
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
  lua_pushnumber(L, MSG_get_host_number());
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
  m_host_t host = MSG_get_host_table()[index - 1];      // lua indexing start by 1 (lua[1] <=> C[0])
  lua_newtable(L);              /* create a table, put the userdata on top of it */
  m_host_t *lua_host = (m_host_t *) lua_newuserdata(L, sizeof(m_host_t));
  *lua_host = host;
  luaL_getmetatable(L, HOST_MODULE_NAME);
  lua_setmetatable(L, -2);
  lua_setfield(L, -2, "__simgrid_host");        /* put the userdata as field of the table */
  return 1;
}

/**
 * \brief Returns the host where the current process is located.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Return value (host): the current host
 */
static int l_host_self(lua_State * L)
{
                                  /* -- */
  m_host_t host = MSG_host_self();
  lua_newtable(L);
                                  /* table */
  m_host_t* lua_host = (m_host_t*) lua_newuserdata(L, sizeof(m_host_t));
                                  /* table ud */
  *lua_host = host;
  luaL_getmetatable(L, HOST_MODULE_NAME);
                                  /* table ud mt */
  lua_setmetatable(L, -2);
                                  /* table ud */
  lua_setfield(L, -2, "__simgrid_host");
                                  /* table */
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
  m_host_t ht = sglua_check_host(L, 1);
  const char *prop = luaL_checkstring(L, 2);
  lua_pushstring(L,MSG_host_get_property_value(ht,prop));
  return 1;
}

/**
 * \brief Makes the current process sleep for a while.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Argument 1 (number): duration of the sleep
 */
static int l_host_sleep(lua_State *L)
{
  int time = luaL_checknumber(L, 1);
  MSG_process_sleep(time);
  return 0;
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
  m_host_t ht = sglua_check_host(L, 1);
  __MSG_host_destroy(ht);
  return 0;
}

static const luaL_reg host_functions[] = {
  {"get_by_name", l_host_get_by_name},
  {"name", l_host_get_name},
  {"number", l_host_number},
  {"at", l_host_at},
  {"self", l_host_self},
  {"get_prop_value", l_host_get_property_value},
  {"sleep", l_host_sleep},
  {"destroy", l_host_destroy},
  // Bypass XML Methods
  {"set_function", console_set_function},
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

static const luaL_reg host_meta[] = {
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
  luaL_openlib(L, HOST_MODULE_NAME, host_functions, 0);
                                  /* simgrid.host */

  /* create the metatable for host, add it to the Lua registry */
  luaL_newmetatable(L, HOST_MODULE_NAME);
                                  /* simgrid.host mt */
  /* fill the metatable */
  luaL_openlib(L, NULL, host_meta, 0);
                                  /* simgrid.host mt */
  lua_pushvalue(L, -2);
                                  /* simgrid.host mt simgrid.host */
  /* metatable.__index = simgrid.host
   * we put the host functions inside the host userdata itself:
   * this allows to write my_host:method(args) for
   * simgrid.host.method(my_host, args) */
  lua_setfield(L, -2, "__index");
                                  /* simgrid.host mt */
  lua_pop(L, 2);
                                  /* -- */
}

