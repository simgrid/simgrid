/* Copyright (c) 2010-2016. The SimGrid Team.
 * All rights reserved.                                                     
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


/*
 * This file contains functions that aid users to debug their lua scripts; for instance,
 * tables can be easily output and values are represented in a human-readable way. (For instance,
 * a NULL value becomes the string "nil").
 *
 */
 /* SimGrid Lua debug functions                                             */
extern "C" {
#include <lauxlib.h>
}
#include "lua_utils.h"
#include "xbt.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(lua_debug, "Lua bindings (helper functions)");

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
const char* sglua_tostring(lua_State* L, int index) {

  static char buff[64];

  switch (lua_type(L, index)) {

    case LUA_TNIL:
      sprintf(buff, "nil");
      break;

    case LUA_TNUMBER: 
      sprintf(buff, "%.3f", lua_tonumber(L, index));
      break;

    case LUA_TBOOLEAN:
      sprintf(buff, "%s", lua_toboolean(L, index) ? "true" : "false");
      break;

    case LUA_TSTRING:
      snprintf(buff, 63, "'%s'", lua_tostring(L, index));
      break;

    case LUA_TFUNCTION:
      if (lua_iscfunction(L, index)) {
        sprintf(buff, "C-function");
      }
      else {
        sprintf(buff, "function");
      }
      break;

    case LUA_TTABLE:
      sprintf(buff, "table(%p)", lua_topointer(L, index));
      break;

    case LUA_TLIGHTUSERDATA:
    case LUA_TUSERDATA:
      sprintf(buff, "userdata(%p)", lua_touserdata(L, index));
      break;

    case LUA_TTHREAD:
      sprintf(buff, "thread");
      break;
  }
  return buff;
}

static int sglua_dump_table(lua_State* L) {
  int argc = lua_gettop(L);

  for (int i = 1; i < argc; i++) {
    if (lua_istable(L, i)) {
      lua_pushnil(L); /* table nil */

      //lua_next pops the topmost element from the stack and 
      //gets the next pair from the table
      while (lua_next(L, -1)) { /* table key val  */
        // we need to copy here, as a cast from "Number" to "String"
        // could happen in Lua.
        // see remark in the lua manual, function "lua_tolstring"
        // http://www.lua.org/manual/5.3/manual.html#lua_tolstring

        lua_pushvalue(L, -2); /* table key val key */

        const char *key = lua_tostring(L, -1); /* table key val */
        const char *val = lua_tostring(L, -1); /* table key     */

        XBT_DEBUG("%s => %s", key, val);
      }

      lua_settop(L, argc); // Remove everything except the initial arguments
    }
  }

  return 0;
}

/**
 * @brief Returns a string composed of the specified number of spaces.
 *
 * This function can be used to indent strings for debugging purposes.
 * It always returns the same pointer.
 *
 * @param length length of the string
 * @return a string of this length with only spaces
 */
const char* sglua_get_spaces(int length) {

  static char spaces[128];

  xbt_assert(length >= 0 && length < 128,
      "Invalid indentation length: %d", length);
  if (length != 0) {
    memset(spaces, ' ', length);
  }
  spaces[length] = '\0';
  return spaces;
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
const char* sglua_keyvalue_tostring(lua_State* L, int key_index, int value_index) {

  static char buff[64];
  /* value_tostring also always returns the same pointer */
  int len = snprintf(buff, 63, "[%s] -> ", sglua_tostring(L, key_index));
  snprintf(buff + len, 63 - len, "%s", sglua_tostring(L, value_index));
  return buff;
}

/**
 * @brief Dumps the Lua stack if debug logs are enabled.
 * @param msg a message to print
 * @param L a Lua state
 */
void sglua_stack_dump(lua_State* L, const char* msg)
{
  if (XBT_LOG_ISENABLED(lua_debug, xbt_log_priority_debug)) {
    char buff[2048];
    char* p = buff;
    int top = lua_gettop(L);

    fflush(stdout);

    p[0] = '\0';
    for (int i = 1; i <= top; i++) {  /* repeat for each level */
      p += sprintf(p, "%s ", sglua_tostring(L, i));
    }

    XBT_DEBUG("%s%s", msg, buff);
  }
}

/**
 * \brief Like luaL_checkudata, with additional debug logs.
 *
 * This function is for debugging purposes only.
 *
 * \param L a lua state
 * \param ud index of the userdata to check in the stack
 * \param tname key of the metatable of this userdata in the registry
 */
void* sglua_checkudata_debug(lua_State* L, int ud, const char* tname)
{
  XBT_DEBUG("Checking the userdata: ud = %d", ud);
  sglua_stack_dump(L, "my_checkudata: ");
  void* p = lua_touserdata(L, ud);
  lua_getfield(L, LUA_REGISTRYINDEX, tname);
  const void* correct_mt = lua_topointer(L, -1);

  int has_mt = lua_getmetatable(L, ud);
  XBT_DEBUG("Checking the userdata: has metatable ? %d", has_mt);
  const void* actual_mt = NULL;
  if (has_mt) {
    actual_mt = lua_topointer(L, -1);
    lua_pop(L, 1);
  }
  XBT_DEBUG("Checking the task's metatable: expected %p, found %p", correct_mt, actual_mt);
  sglua_stack_dump(L, "my_checkudata: ");

  if (p == NULL || !lua_getmetatable(L, ud) || !lua_rawequal(L, -1, -2))
    XBT_ERROR("Error: Userdata is NULL, couldn't find metatable or top of stack does not equal element below it.");
  lua_pop(L, 2);
  return p;
}

/**
 * @brief Writes the specified data into a memory buffer.
 *
 * This function is a valid lua_Writer that writes into a memory buffer passed
 * as userdata.
 *
 * @param L a lua state
 * @param source some data
 * @param sz number of bytes of data
 * @param user_data the memory buffer to write
 */
int sglua_memory_writer(lua_State* L, const void* source, size_t size,
    void* userdata) {

  sglua_buffer_t buffer = (sglua_buffer_t) userdata;
  while (buffer->capacity < buffer->size + size) {
    buffer->capacity *= 2;
    buffer->data = (char*)xbt_realloc(buffer->data, buffer->capacity);
  }
  memcpy(buffer->data + buffer->size, source, size);
  buffer->size += size;

  return 0;
}
