/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* SimGrid Lua bindings                                                     */

#include "simgrid_lua.h"
#include "msg/msg.h"

void sglua_register_task_functions(lua_State* L);
m_task_t sglua_check_task(lua_State* L, int index);
void sglua_task_register(lua_State* L);
void sglua_task_unregister(lua_State* L, m_task_t task);

void sglua_register_comm_functions(lua_State* L);
msg_comm_t sglua_check_comm(lua_State* L, int index);
void sglua_push_comm(lua_State* L, msg_comm_t comm);

void sglua_register_host_functions(lua_State* L);
m_host_t sglua_check_host(lua_State* L, int index);

void sglua_register_process_functions(lua_State* L);

void sglua_register_platf_functions(lua_State* L);

const char* sglua_get_msg_error(MSG_error_t err);
