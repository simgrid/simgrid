/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_LUA_H
#define SIMGRID_LUA_H

#include <lua.h>

/* ********************************************************************************* */
/*                           Plaftorm functions                                      */
/* ********************************************************************************* */

int console_open(lua_State *L);
int console_close(lua_State *L);

int console_add_host(lua_State*);
int console_add_link(lua_State*);
int console_add_router(lua_State* L);
int console_add_route(lua_State*);
int console_AS_open(lua_State*);
int console_AS_close(lua_State *L);
int console_set_function(lua_State*);
int console_host_set_property(lua_State*);

#endif  /* SIMGRID_LUA_H */
