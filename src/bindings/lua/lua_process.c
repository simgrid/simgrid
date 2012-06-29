/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* SimGrid Lua bindings                                                     */

#include "lua_private.h"
#include <lauxlib.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(lua_process, bindings, "Lua Bindings (process module)");

#define PROCESS_MODULE_NAME "simgrid.process"

/* ********************************************************************************* */
/*                              simgrid.process API                                  */
/* ********************************************************************************* */

/**
 * \brief Makes the current process sleep for a while.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Argument 1 (number): duration of the sleep
 * - Return value (nil or string): nil in everything went ok, or a string error
 * if case of failure ("host failure")
 */
static int l_process_sleep(lua_State* L)
{
  double duration = luaL_checknumber(L, 1);
  msg_error_t res = MSG_process_sleep(duration);

  switch (res) {

  case MSG_OK:
    return 0;

  case MSG_HOST_FAILURE:
    lua_pushliteral(L, "host failure");
    return 1;

  default:
    xbt_die("Unexpected result of MSG_process_sleep: %d, please report this bug", res);
  }
}

static const luaL_reg process_functions[] = {
    {"sleep", l_process_sleep},
    /* TODO: self, create, kill, suspend, is_suspended, resume, get_name,
     * get_pid, get_ppid, migrate
     */
    {NULL, NULL}
};

/**
 * \brief Registers the process functions into the table simgrid.process.
 * \param L a lua state
 */
void sglua_register_process_functions(lua_State* L)
{
  luaL_openlib(L, PROCESS_MODULE_NAME, process_functions, 0);
                                  /* simgrid.process */
  lua_pop(L, 1);
}

