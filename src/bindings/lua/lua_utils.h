/* Copyright (c) 2010-2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* SimGrid Lua helper functions                                             */

#ifndef LUA_UTILS_H
#define LUA_UTILS_H

#include <lua.h>

/**
 * @brief A chunk of memory.
 *
 * This structure is used as the userdata parameter of lua_Writer.
 */
typedef struct s_sglua_buffer {
  char* data;
  size_t size;
  size_t capacity;
} s_sglua_buffer_t, *sglua_buffer_t;

const char* sglua_tostring(lua_State* L, int index);
const char* sglua_keyvalue_tostring(lua_State* L, int key_index, int value_index);
void sglua_stack_dump(const char *msg, lua_State* L);
void* sglua_checkudata_debug(lua_State* L, int ud, const char* tname);
const char* sglua_get_spaces(int length);
int sglua_memory_writer(lua_State* L, const void* source, size_t size, void* userdata);

#endif  /* LUA_UTILS_H */
