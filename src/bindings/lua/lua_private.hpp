/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* SimGrid Lua bindings                                                     */

#ifndef LUA_PRIVATE_HPP
#define LUA_PRIVATE_HPP

#include "simgrid/msg.h"
#include "simgrid_lua.hpp"

extern "C" {

void sglua_register_host_functions(lua_State* L);
msg_host_t sglua_check_host(lua_State* L, int index);

void sglua_register_platf_functions(lua_State* L);

const char* sglua_get_msg_error(msg_error_t err);
}

#define lua_ensure(...) _XBT_IF_ONE_ARG(_lua_ensure_ARG1, _lua_ensure_ARGN, __VA_ARGS__)(__VA_ARGS__)
#define _lua_ensure_ARG1(cond) _lua_ensure_ARGN(cond, "Assertion %s failed", #cond)
#define _lua_ensure_ARGN(cond, ...)                                                                                    \
  do {                                                                                                                 \
    if (!(cond)) {                                                                                                     \
      luaL_error(L, __VA_ARGS__);                                                                                      \
      return -1;                                                                                                       \
    }                                                                                                                  \
  } while (0)

#endif
