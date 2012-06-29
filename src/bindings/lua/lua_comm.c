/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "lua_private.h"
#include <lauxlib.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(lua_comm, bindings, "Lua bindings (comm module)");

#define COMM_MODULE_NAME "simgrid.comm"

/* ********************************************************************************* */
/*                                simgrid.comm API                                   */
/* ********************************************************************************* */

/**
 * \brief Ensures that a value in the stack is a comm and returns it.
 * \param L a Lua state
 * \param index an index in the Lua stack
 * \return the C comm
 */
msg_comm_t sglua_check_comm(lua_State* L, int index)
{
  msg_comm_t comm = *((msg_comm_t*) luaL_checkudata(L, index, COMM_MODULE_NAME));
  return comm;
}

/**
 * \brief Pushes a comm onto the stack.
 * \param L a Lua state
 * \param comm a comm
 */
void sglua_push_comm(lua_State* L, msg_comm_t comm)
{
  msg_comm_t* userdata = (msg_comm_t*) lua_newuserdata(L, sizeof(msg_comm_t));
                                /* comm */
  *userdata = comm;
  luaL_getmetatable(L, COMM_MODULE_NAME);
                                /* comm mt */
  lua_setmetatable(L, -2);
                                /* comm */
}

/**
 * \brief Blocks the current process until a communication is finished.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Argument 1 (comm): a comm (previously created by isend or irecv)
 * - Argument 2 (number, optional): timeout (default is no timeout)
 * - Return values (task or nil + string): in case of success, returns the task
 * received if you are the receiver and nil if you are the sender. In case of
 * failure, returns nil plus an error string.
 */
static int l_comm_wait(lua_State* L) {

  msg_comm_t comm = sglua_check_comm(L, 1);
  double timeout = -1;
  if (lua_gettop(L) >= 2) {
    timeout = luaL_checknumber(L, 2);
  }
                                  /* comm ... */
  msg_error_t res = MSG_comm_wait(comm, timeout);

  if (res == MSG_OK) {
    msg_task_t task = MSG_comm_get_task(comm);
    if (MSG_task_get_sender(task) == MSG_process_self()) {
      /* I'm the sender */
      return 0;
    }
    else {
      /* I'm the receiver: find the Lua task from the C task */
      sglua_task_unregister(L, task);
                                  /* comm ... task */
      return 1;
    }
  }
  else {
    /* the communication has failed */
    lua_pushnil(L);
                                  /* comm ... nil */
    lua_pushstring(L, sglua_get_msg_error(res));
                                  /* comm ... nil error */
    return 2;
  }
}

/**
 * @brief Returns whether a communication is finished.
 *
 * Unlike wait(), This function always returns immediately.
 *
 * - Argument 1 (comm): a comm (previously created by isend or irecv)
 * - Return values (task/boolean or nil + string): if the communication is not
 * finished, return false. If the communication is finished and was successful,
 * returns the task received if you are the receiver or true if you are the
 * sender. If the communication is finished and has failed, returns nil
 * plus an error string.
 */
static int l_comm_test(lua_State* L) {

  msg_comm_t comm = sglua_check_comm(L, 1);
                                  /* comm ... */
  if (!MSG_comm_test(comm)) {
    /* not finished yet */
    lua_pushboolean(L, 0);
                                  /* comm ... false */
    return 1;
  }
  else {
    /* finished but may have failed */
    msg_error_t res = MSG_comm_get_status(comm);

    if (res == MSG_OK) {
      msg_task_t task = MSG_comm_get_task(comm);
      if (MSG_task_get_sender(task) == MSG_process_self()) {
        /* I'm the sender */
        lua_pushboolean(L, 1);
                                  /* comm ... true */
        return 1;
      }
      else {
        /* I'm the receiver: find the Lua task from the C task*/
        sglua_task_unregister(L, task);
                                  /* comm ... task */
        return 1;
      }
    }
    else {
      /* the communication has failed */
      lua_pushnil(L);
                                  /* comm ... nil */
      lua_pushstring(L, sglua_get_msg_error(res));
                                  /* comm ... nil error */
      return 2;
    }
  }
}

static const luaL_reg comm_functions[] = {
  {"wait", l_comm_wait},
  {"test", l_comm_test},
  /* TODO waitany, testany */
  {NULL, NULL}
};

/**
 * \brief Finalizes a comm userdata.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Argument 1 (userdata): a comm
 */
static int l_comm_gc(lua_State* L)
{
                                  /* ctask */
  msg_comm_t comm = *((msg_comm_t*) luaL_checkudata(L, 1, COMM_MODULE_NAME));
  MSG_comm_destroy(comm);
  return 0;
}

/**
 * \brief Metamethods of the comm userdata.
 */
static const luaL_reg comm_meta[] = {
  {"__gc", l_comm_gc},
  {NULL, NULL}
};

/**
 * \brief Registers the comm functions into the table simgrid.comm.
 *
 * Also initialize the metatable of the comm userdata type.
 *
 * \param L a lua state
 */
void sglua_register_comm_functions(lua_State* L)
{
  /* create a table simgrid.com and fill it with com functions */
  luaL_openlib(L, COMM_MODULE_NAME, comm_functions, 0);
                                  /* simgrid.comm */

  /* create the metatable for comms, add it to the Lua registry */
  luaL_newmetatable(L, COMM_MODULE_NAME);
                                  /* simgrid.comm mt */
  /* fill the metatable */
  luaL_openlib(L, NULL, comm_meta, 0);
                                  /* simgrid.comm mt */
  lua_pushvalue(L, -2);
                                  /* simgrid.comm mt simgrid.comm */
  /* metatable.__index = simgrid.comm
   * we put the comm functions inside the comm itself:
   * this allows to write my_comm:method(args) for
   * simgrid.comm.method(my_comm, args) */
  lua_setfield(L, -2, "__index");
                                  /* simgrid.comm mt */
  lua_pop(L, 2);
                                  /* -- */
}

