/* Copyright (c) 2010-2021. The SimGrid Team.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/*
 * This file contains functions that aid users to debug their lua scripts; for instance,
 * tables can be easily output and values are represented in a human-readable way. (For instance,
 * a nullptr value becomes the string "nil").
 *
 */
/* SimGrid Lua helper functions                                             */
#include "lua_utils.hpp"
#include <lauxlib.h>
#include <string>
#include <xbt/string.hpp>

/**
 * @brief Returns a string representation of a value in the Lua stack.
 *
 * This function is for debugging purposes.
 *
 * @param L the Lua state
 * @param index index in the stack
 * @return a string representation of the value at this index
 */
std::string sglua_tostring(lua_State* L, int index)
{
  std::string buff;

  switch (lua_type(L, index)) {
    case LUA_TNIL:
      buff = "nil";
      break;

    case LUA_TNUMBER:
      buff = simgrid::xbt::string_printf("%.3f", lua_tonumber(L, index));
      break;

    case LUA_TBOOLEAN:
      buff = lua_toboolean(L, index) ? "true" : "false";
      break;

    case LUA_TSTRING:
      buff = simgrid::xbt::string_printf("'%s'", lua_tostring(L, index));
      break;

    case LUA_TFUNCTION:
      buff = lua_iscfunction(L, index) ? "C-function" : "function";
      break;

    case LUA_TTABLE:
      buff = simgrid::xbt::string_printf("table(%p)", lua_topointer(L, index));
      break;

    case LUA_TLIGHTUSERDATA:
    case LUA_TUSERDATA:
      buff = simgrid::xbt::string_printf("userdata(%p)", lua_touserdata(L, index));
      break;

    case LUA_TTHREAD:
      buff = "thread";
      break;

    default:
      buff = simgrid::xbt::string_printf("unknown(%d)", lua_type(L, index));
      break;
  }
  return buff;
}

/**
 * @brief Returns a string representation of a key-value pair.
 *
 * @param L the Lua state
 * @param key_index index of the key (in the lua stack)
 * @param value_index index of the value (in the lua stack)
 * @return a string representation of the key-value pair
 */
std::string sglua_keyvalue_tostring(lua_State* L, int key_index, int value_index)
{
  return std::string("[") + sglua_tostring(L, key_index) + "] -> " + sglua_tostring(L, value_index);
}
