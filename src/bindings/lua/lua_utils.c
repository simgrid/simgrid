/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* SimGrid Lua helper functions                                             */

#include "lua_utils.h"
#include "xbt.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(lua_utils, lua, "Lua helper functions");

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

/**
 * @brief Returns a string representation of a key-value pair.
 *
 * This function is for debugging purposes.
 * It always returns the same pointer.
 *
 * @param L the Lua state
 * @param key_index index of the key
 * @param value_index index of the value
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

  xbt_assert(length < 128);
  memset(spaces, ' ', length);
  spaces[length] = '\0';
  return spaces;
}

/**
 * @brief Dumps the Lua stack if debug logs are enabled.
 * @param msg a message to print
 * @param L a Lua state
 */
void sglua_stack_dump(const char* msg, lua_State* L)
{
  if (XBT_LOG_ISENABLED(lua_utils, xbt_log_priority_debug)) {
    char buff[2048];
    char* p = buff;
    int i;
    int top = lua_gettop(L);

    //if (1) return;

    fflush(stdout);

    p[0] = '\0';
    for (i = 1; i <= top; i++) {  /* repeat for each level */

      p += sprintf(p, "%s", sglua_tostring(L, i));
      p += sprintf(p, " ");       /* put a separator */
    }
    XBT_DEBUG("%s%s", msg, buff);
  }
}

/**
 * @brief Writes the specified data into a memory buffer.
 *
 * This function is a valid lua_Writer that writes into a memory buffer passed
 * as userdata.
 * TODO: use a dynar as userdata
 *
 * @param L a lua state
 * @param source some data
 * @param sz number of bytes of data
 * @param user_data the memory buffer to write
 */
int sglua_memory_writer(lua_State* L, const void* source, size_t size,
    void* userdata) {

  buffer_t buffer = (buffer_t) userdata;
  while (buffer->capacity < buffer->size + size) {
    buffer->capacity *= 2;
    buffer->data = xbt_realloc(buffer->data, buffer->capacity);
  }
  memcpy(buffer->data + buffer->size, source, size);
  buffer->size += size;

  return 0;
}
