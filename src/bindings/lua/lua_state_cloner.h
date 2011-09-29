/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* SimGrid Lua state management                                             */


#include <lua.h>

void sglua_move_value(lua_State* src, lua_State* dst);
lua_State* sglua_clone_state(lua_State* L);
