/* Copyright (c) 2010-2020. The SimGrid Team.
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

/**
 * @brief Returns a string representation of a value in the Lua stack.
 *
 * This function is for debugging purposes.
 * It always returns the same pointer.
 *
 * @param L the Lua state
 * @param index index in the stack
 * @return a string representation of the value at this index
 */
const char* sglua_tostring(lua_State* L, int index)
{

  static char buff[64];

  switch (lua_type(L, index)) {

    case LUA_TNIL:
      snprintf(buff, 4, "nil");
      break;

    case LUA_TNUMBER:
      snprintf(buff, 64, "%.3f", lua_tonumber(L, index));
      break;

    case LUA_TBOOLEAN:
      snprintf(buff, 64, "%s", lua_toboolean(L, index) ? "true" : "false");
      break;

    case LUA_TSTRING:
      snprintf(buff, 63, "'%s'", lua_tostring(L, index));
      break;

    case LUA_TFUNCTION:
      if (lua_iscfunction(L, index)) {
        snprintf(buff, 11, "C-function");
      } else {
        snprintf(buff, 9, "function");
      }
      break;

    case LUA_TTABLE:
      snprintf(buff, 64, "table(%p)", lua_topointer(L, index));
      break;

    case LUA_TLIGHTUSERDATA:
    case LUA_TUSERDATA:
      snprintf(buff, 64, "userdata(%p)", lua_touserdata(L, index));
      break;

    case LUA_TTHREAD:
      snprintf(buff, 7, "thread");
      break;

    default:
      snprintf(buff, 64, "unknown(%d)", lua_type(L, index));
      break;
  }
  return buff;
}

/**
 * @brief Returns a string representation of a key-value pair.
 *
 * It always returns the same pointer.
 *
 * @param L the Lua state
 * @param key_index index of the key (in the lua stack)
 * @param value_index index of the value (in the lua stack)
 * @return a string representation of the key-value pair
 */
const char* sglua_keyvalue_tostring(lua_State* L, int key_index, int value_index)
{

  static char buff[64];
  /* value_tostring also always returns the same pointer */
  int len = snprintf(buff, 63, "[%s] -> ", sglua_tostring(L, key_index));
  snprintf(buff + len, 63 - len, "%s", sglua_tostring(L, value_index));
  return buff;
}
