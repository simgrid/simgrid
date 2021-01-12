/* Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* SimGrid Lua helper functions                                             */

#ifndef LUA_UTILS_HPP
#define LUA_UTILS_HPP

#include <lua.h>
#include <string>

std::string sglua_tostring(lua_State* L, int index);
std::string sglua_keyvalue_tostring(lua_State* L, int key_index, int value_index);

#endif
