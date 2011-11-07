/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef LUA_STATE_CLONER_H
#define LUA_STATE_CLONER_H

/* SimGrid Lua state management                                             */
#include <lua.h>

int sglua_is_maestro(lua_State* L);
lua_State* sglua_get_maestro(void);
lua_State* sglua_clone_maestro(void);
void sglua_move_value(lua_State* src, lua_State* dst);
void sglua_copy_value(lua_State* src, lua_State* dst);

#endif  /* LUA_STATE_CLONER_H */
