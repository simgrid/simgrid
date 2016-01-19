/* Copyright (c) 2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* SimGrid Lua bindings                                                     */

#include "simgrid_lua.h"
#include "simgrid/msg.h"

void sglua_register_host_functions(lua_State* L);
msg_host_t sglua_check_host(lua_State* L, int index);

void sglua_register_platf_functions(lua_State* L);

const char* sglua_get_msg_error(msg_error_t err);
