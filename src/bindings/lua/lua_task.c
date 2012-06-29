/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "lua_private.h"
#include "lua_utils.h"
#include "lua_state_cloner.h"
#include <lauxlib.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(lua_task, bindings, "Lua bindings (task module)");

#define TASK_MODULE_NAME "simgrid.task"

/* ********************************************************************************* */
/*                                simgrid.task API                                   */
/* ********************************************************************************* */

/**
 * \brief Ensures that a value in the stack is a valid task and returns it.
 * \param L a Lua state
 * \param index an index in the Lua stack
 * \return the C task corresponding to this Lua task
 */
m_task_t sglua_check_task(lua_State* L, int index)
{
  sglua_stack_dump("check task: ", L);
  luaL_checktype(L, index, LUA_TTABLE);
                                  /* ... task ... */
  lua_getfield(L, index, "__simgrid_task");
                                  /* ... task ... ctask */
  m_task_t task = *((m_task_t*) luaL_checkudata(L, -1, TASK_MODULE_NAME));
  lua_pop(L, 1);
                                  /* ... task ... */

  if (task == NULL) {
    luaL_error(L, "This task was sent to someone else, you cannot access it anymore");
  }

  return task;
}

/**
 * \brief Creates a new task and leaves it onto the stack.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Argument 1 (string): name of the task
 * - Argument 2 (number): computation size
 * - Argument 3 (number): communication size
 * - Return value (task): the task created
 *
 * A Lua task is a regular table with a full userdata inside, and both share
 * the same metatable. For the regular table, the metatable allows OO-style
 * writing such as your_task:send(someone).
 * For the userdata, the metatable is used to check its type.
 * TODO: make the task name an optional last parameter
 */
static int l_task_new(lua_State* L)
{
  XBT_DEBUG("Task new");
  const char* name = luaL_checkstring(L, 1);
  int comp_size = luaL_checkint(L, 2);
  int msg_size = luaL_checkint(L, 3);
                                  /* name comp comm */
  lua_settop(L, 0);
                                  /* -- */
  m_task_t msg_task = MSG_task_create(name, comp_size, msg_size, NULL);

  lua_newtable(L);
                                  /* task */
  luaL_getmetatable(L, TASK_MODULE_NAME);
                                  /* task mt */
  lua_setmetatable(L, -2);
                                  /* task */
  m_task_t* lua_task = (m_task_t*) lua_newuserdata(L, sizeof(m_task_t));
                                  /* task ctask */
  *lua_task = msg_task;
  luaL_getmetatable(L, TASK_MODULE_NAME);
                                  /* task ctask mt */
  lua_setmetatable(L, -2);
                                  /* task ctask */
  lua_setfield(L, -2, "__simgrid_task");
                                  /* task */
  return 1;
}

/**
 * \brief Returns the name of a task.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Argument 1 (task): a task
 * - Return value (string): name of the task
 */
static int l_task_get_name(lua_State* L)
{
  m_task_t task = sglua_check_task(L, 1);
  lua_pushstring(L, MSG_task_get_name(task));
  return 1;
}

/**
 * \brief Returns the computation duration of a task.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Argument 1 (task): a task
 * - Return value (number): computation duration of this task
 */
static int l_task_get_computation_duration(lua_State* L)
{
  m_task_t task = sglua_check_task(L, 1);
  lua_pushnumber(L, MSG_task_get_compute_duration(task));
  return 1;
}

/**
 * \brief Executes a task.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Argument 1 (task): the task to execute
 * - Return value (nil or string): nil if the task was successfully executed,
 * or an error string in case of failure, which may be "task canceled" or
 * "host failure"
 */
static int l_task_execute(lua_State* L)
{
  m_task_t task = sglua_check_task(L, 1);
  MSG_error_t res = MSG_task_execute(task);

  if (res == MSG_OK) {
    return 0;
  }
  else {
    lua_pushstring(L, sglua_get_msg_error(res));
    return 1;
  }
}

/**
 * \brief Pops the Lua task from the stack and registers it so that the
 * process can retrieve it later knowing the C task.
 * \param L a lua state
 */
void sglua_task_register(lua_State* L) {

  m_task_t task = sglua_check_task(L, -1);
                                  /* ... task */
  /* put in the C task a ref to the lua task so that the receiver finds it */
  unsigned long ref = luaL_ref(L, LUA_REGISTRYINDEX);
                                  /* ... */
  MSG_task_set_data(task, (void*) ref);
}

/**
 * \brief Pushes onto the stack the Lua task corresponding to a C task.
 *
 * The Lua task must have been previously registered with task_register so
 * that it can be retrieved knowing the C task.
 *
 * \param L a lua state
 * \param task a C task
 */
void sglua_task_unregister(lua_State* L, m_task_t task) {

                                  /* ... */
  /* the task is in my registry, put it onto my stack */
  unsigned long ref = (unsigned long) MSG_task_get_data(task);
  lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
                                  /* ... task */
  luaL_unref(L, LUA_REGISTRYINDEX, ref);
  MSG_task_set_data(task, NULL);
}

/**
 * \brief This function is called when a C task has just been copied.
 *
 * This callback is used to move the corresponding Lua task from the sender
 * process to the receiver process.
 * It is executed in SIMIX kernel mode when the communication finishes,
 * before both processes are awaken. Thus, this function is thread-safe when
 * user processes are executed in parallel, though it modifies the Lua
 * stack of both processes to move the task.
 * After this function, both Lua stacks are restored in their previous state.
 * The task is moved from the registry of the sender to the registry of the
 * receiver.
 *
 * \param task the task copied
 * \param src_process the sender
 * \param dst_process the receiver
 */
static void task_copy_callback(m_task_t task, msg_process_t src_process,
    msg_process_t dst_process) {

  lua_State* src = MSG_process_get_data(src_process);
  lua_State* dst = MSG_process_get_data(dst_process);

                                  /* src: ...
                                     dst: ... */
  sglua_task_unregister(src, task);
                                  /* src: ... task */
  sglua_copy_value(src, dst);
                                  /* src: ... task
                                     dst: ... task */
  sglua_task_register(dst);
                                  /* dst: ... */

  /* the receiver is now the owner of the task and may destroy it:
   * make the sender forget the C task so that it doesn't garbage */
  lua_getfield(src, -1, "__simgrid_task");
                                  /* src: ... task ctask */
  m_task_t* udata = (m_task_t*) luaL_checkudata(src, -1, TASK_MODULE_NAME);
  *udata = NULL;
  lua_pop(src, 2);
                                  /* src: ... */
}

/**
 * \brief Sends a task to a mailbox and waits for its completion.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Argument 1 (task): the task to send
 * - Argument 2 (string or compatible): mailbox name, as a real string or any
 * type convertible to string (numbers always are)
 * - Argument 3 (number, optional): timeout (default is no timeout)
 * - Return values (boolean + string): true if the communication was successful,
 * or false plus an error string in case of failure, which may be "timeout",
 * "host failure" or "transfer failure"
 */
static int l_task_send(lua_State* L)
{
  m_task_t task = sglua_check_task(L, 1);
  const char* mailbox = luaL_checkstring(L, 2);
  double timeout;
  if (lua_gettop(L) >= 3) {
    timeout = luaL_checknumber(L, 3);
  }
  else {
    timeout = -1;
    /* no timeout by default */
  }
  lua_settop(L, 1);
                                  /* task */
  sglua_task_register(L);
                                  /* -- */
  MSG_error_t res = MSG_task_send_with_timeout(task, mailbox, timeout);

  if (res == MSG_OK) {
    lua_pushboolean(L, 1);
                                  /* true */
    return 1;
  }
  else {
    /* the communication has failed, I'm still the owner of the task */
    sglua_task_unregister(L, task);
                                  /* task */
    lua_pushboolean(L, 0);
                                  /* task false */
    lua_pushstring(L, sglua_get_msg_error(res));
                                  /* task false error */
    return 2;
  }
}

/**
 * \brief Sends a task on a mailbox.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * This is a non-blocking function: use simgrid.comm.wait() or
 * simgrid.comm.test() to end the communication.
 *
 * - Argument 1 (task): the task to send
 * - Argument 2 (string or compatible): mailbox name, as a real string or any
 * type convertible to string (numbers always are)
 * - Return value (comm): a communication object to be used later with wait or test
 */
static int l_task_isend(lua_State* L)
{
  m_task_t task = sglua_check_task(L, 1);
  const char* mailbox = luaL_checkstring(L, 2);
                                  /* task mailbox ... */
  lua_settop(L, 1);
                                  /* task */
  sglua_task_register(L);
                                  /* -- */
  msg_comm_t comm = MSG_task_isend(task, mailbox);

  sglua_push_comm(L, comm);
                                  /* comm */
  return 1;
}

/**
 * \brief Sends a task on a mailbox on a best effort way (detached send).
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * Like simgrid.task.isend, this is a non-blocking function.
 * You can use this function if you don't care about when the communication
 * ends and whether it succeeds.
 * FIXME: isn't this equivalent to calling simgrid.task.isend() and ignoring
 * the result?
 *
 * - Argument 1 (task): the task to send
 * - Argument 2 (string or compatible): mailbox name, as a real string or any
 * type convertible to string (numbers always are)
 */
static int l_task_dsend(lua_State* L)
{
  m_task_t task = sglua_check_task(L, 1);
  const char* mailbox = luaL_checkstring(L, 2);
                                  /* task mailbox ... */
  lua_settop(L, 1);
                                  /* task */
  sglua_task_register(L);
                                  /* -- */
  MSG_task_dsend(task, mailbox, NULL);
  return 0;
}

/**
 * \brief Receives a task.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Argument 1 (string or compatible): mailbox name, as a real string or any
 * type convertible to string (numbers always are)
 * - Argument 2 (number, optional): timeout (default is no timeout)
 * - Return values (task or nil + string): the task received, or nil plus an
 * error message if the communication has failed
 */
static int l_task_recv(lua_State* L)
{
  m_task_t task = NULL;
  const char* mailbox = luaL_checkstring(L, 1);
  double timeout;
  if (lua_gettop(L) >= 2) {
                                  /* mailbox timeout ... */
    timeout = luaL_checknumber(L, 2);
  }
  else {
                                  /* mailbox */
    timeout = -1;
    /* no timeout by default */
  }
                                  /* mailbox ... */
  MSG_error_t res = MSG_task_receive_with_timeout(&task, mailbox, timeout);

  if (res == MSG_OK) {
    sglua_task_unregister(L, task);
                                  /* mailbox ... task */
    return 1;
  }
  else {
    lua_pushnil(L);
                                  /* mailbox ... nil */
    lua_pushstring(L, sglua_get_msg_error(res));
                                  /* mailbox ... nil error */
    return 2;
  }
}

/**
 * \brief Asynchronously receives a task on a mailbox.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * This is a non-blocking function: use simgrid.comm.wait() or
 * simgrid.comm.test() to end the communication and get the task in case of
 * success.
 *
 * - Argument 1 (string or compatible): mailbox name, as a real string or any
 * type convertible to string (numbers always are)
 * - Return value (comm): a communication object to be used later with wait or test
 */

static int l_task_irecv(lua_State* L)
{
  const char* mailbox = luaL_checkstring(L, 1);
                                  /* mailbox ... */
  m_task_t* task = xbt_new0(m_task_t, 1); // FIXME fix this leak
  msg_comm_t comm = MSG_task_irecv(task, mailbox);
  sglua_push_comm(L, comm);
                                  /* mailbox ... comm */
  return 1;
}

static const luaL_reg task_functions[] = {
  {"new", l_task_new},
  {"get_name", l_task_get_name},
  {"get_computation_duration", l_task_get_computation_duration},
  {"execute", l_task_execute},
  {"send", l_task_send},
  {"isend", l_task_isend},
  {"dsend", l_task_dsend},
  {"recv", l_task_recv},
  {"irecv", l_task_irecv},
  {NULL, NULL}
};

/**
 * \brief Finalizes the userdata of a task.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Argument 1 (userdata): a C task, possibly NULL if it was sent to another
 * Lua state
 */
static int l_task_gc(lua_State* L)
{
                                  /* ctask */
  m_task_t task = *((m_task_t*) luaL_checkudata(L, 1, TASK_MODULE_NAME));
  /* the task is NULL if I sent it to someone else */
  if (task != NULL) {
    MSG_task_destroy(task);
  }
  return 0;
}

/**
 * \brief Returns a string representation of a C task.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Argument 1 (userdata): a task
 * - Return value (string): a string describing this task
 */
static int l_task_tostring(lua_State* L)
{
  m_task_t task = *((m_task_t*) luaL_checkudata(L, 1, TASK_MODULE_NAME));
  lua_pushfstring(L, "Task: %p", task);
  return 1;
}

/**
 * \brief Metamethods of both a task table and the userdata inside it.
 */
static const luaL_reg task_meta[] = {
  {"__gc", l_task_gc}, /* will be called only for userdata */
  {"__tostring", l_task_tostring},
  {NULL, NULL}
};

/**
 * \brief Registers the task functions into the table simgrid.task.
 *
 * Also initialize the metatable of the task userdata type.
 *
 * \param L a lua state
 */
void sglua_register_task_functions(lua_State* L)
{
  /* create a table simgrid.task and fill it with task functions */
  luaL_openlib(L, TASK_MODULE_NAME, task_functions, 0);
                                  /* simgrid.task */

  /* create the metatable for tasks, add it to the Lua registry */
  luaL_newmetatable(L, TASK_MODULE_NAME);
                                  /* simgrid.task mt */
  /* fill the metatable */
  luaL_openlib(L, NULL, task_meta, 0);
                                  /* simgrid.task mt */
  lua_pushvalue(L, -2);
                                  /* simgrid.task mt simgrid.task */
  /* metatable.__index = simgrid.task
   * we put the task functions inside the task itself:
   * this allows to write my_task:method(args) for
   * simgrid.task.method(my_task, args) */
  lua_setfield(L, -2, "__index");
                                  /* simgrid.task mt */
  lua_pop(L, 2);
                                  /* -- */

  /* set up MSG to copy Lua tasks between states */
  MSG_task_set_copy_callback(task_copy_callback);
}

