/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_LUA_HPP
#define SIMGRID_LUA_HPP

extern "C" {
#include <lua.h>

/* ********************************************************************************* */
/*                           Plaftorm functions                                      */
/* ********************************************************************************* */

int console_open(lua_State* L);
int console_close(lua_State* L);

int console_add_backbone(lua_State*);
int console_add_host___link(lua_State*);
int console_add_host(lua_State*);
int console_add_link(lua_State*);
int console_add_router(lua_State* L);
int console_add_route(lua_State*);
int console_add_ASroute(lua_State*);
int console_AS_open(lua_State*);
int console_AS_seal(lua_State* L);
int console_set_function(lua_State*);
int console_host_set_property(lua_State*);
}
#endif
