/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* SimGrid Lua bindings                                                     */

#include "lua_private.h"
#include "lua_utils.h"
#include "xbt.h"
#include "simgrid/msg.h"
#include "simgrid/simdag.h"
#include "surf/surfxml_parse.h"
#include <lauxlib.h>


XBT_LOG_NEW_DEFAULT_SUBCATEGORY(lua, bindings, "Lua Bindings");

int luaopen_simgrid(lua_State *L);
static void sglua_register_c_functions(lua_State *L);

/* ********************************************************************************* */
/*                                  simgrid API                                      */
/* ********************************************************************************* */

/**
 * \brief Prints a log string with debug level.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Argument 1 (string): the text to print
 */
static int debug(lua_State* L) {

  const char* str = luaL_checkstring(L, 1);
  XBT_DEBUG("%s", str);
  return 0;
}

/**
 * \brief Prints a log string with info level.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Argument 1 (string): the text to print
 */
static int info(lua_State* L) {

  const char* str = luaL_checkstring(L, 1);
  XBT_INFO("%s", str);
  return 0;
}

static int error(lua_State* L) {

  const char* str = luaL_checkstring(L, 1);
  XBT_ERROR("%s", str);
  return 0;
}

static int critical(lua_State* L) {

  const char* str = luaL_checkstring(L, 1);
  XBT_CRITICAL("%s", str);
  return 0;
}

static const luaL_Reg simgrid_functions[] = {
  {"debug", debug},
  {"info", info},
  {"critical", critical},
  {"error", error},
  /* short names */
  /* methods to bypass XML parser */
  {NULL, NULL}
};

/* ********************************************************************************* */
/*                           module management functions                             */
/* ********************************************************************************* */

#define LUA_MAX_ARGS_COUNT 10   /* maximum amount of arguments we can get from lua on command line */

/**
 * \brief Opens the simgrid Lua module.
 *
 * This function is called automatically by the Lua interpreter when some
 * Lua code requires the "simgrid" module.
 *
 * \param L the Lua state
 */

int luaopen_simgrid(lua_State *L)
{
  XBT_DEBUG("luaopen_simgrid *****");

  sglua_register_c_functions(L);

  return 1;
}


/**
 * \brief Makes the core functions available to the Lua world.
 * \param L a Lua world
 */
static void sglua_register_core_functions(lua_State *L)
{
  /* register the core C functions to lua */
  luaL_newlib(L, simgrid_functions); /* simgrid */
  lua_pushvalue(L, -1);              /* simgrid simgrid */
  lua_setglobal(L, "simgrid");       /* simgrid */
}

/**
 * \brief Creates the simgrid module and make it available to Lua.
 * \param L a Lua world
 */
static void sglua_register_c_functions(lua_State *L)
{
  sglua_register_core_functions(L);
  sglua_register_host_functions(L);
  sglua_register_platf_functions(L);
}
