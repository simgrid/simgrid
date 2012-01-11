/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* SimGrid Lua bindings                                                     */

#include "simgrid_lua.h"
#include "lua_state_cloner.h"
#include "lua_utils.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(lua, bindings, "Lua Bindings");

#define TASK_MODULE_NAME "simgrid.task"
#define COMM_MODULE_NAME "simgrid.comm"
#define HOST_MODULE_NAME "simgrid.host"
#define PROCESS_MODULE_NAME "simgrid.process"
// Surf (bypass XML)
#define LINK_MODULE_NAME "simgrid.link"
#define ROUTE_MODULE_NAME "simgrid.route"
#define PLATF_MODULE_NAME "simgrid.platf"

static lua_State* sglua_maestro_state;

static const char* msg_errors[] = {
    NULL,
    "timeout",
    "transfer failure",
    "host failure",
    "task canceled"
};

int luaopen_simgrid(lua_State *L);
static void register_c_functions(lua_State *L);
static int run_lua_code(int argc, char **argv);

/* ********************************************************************************* */
/*                                simgrid.task API                                   */
/* ********************************************************************************* */

/**
 * \brief Ensures that a value in the stack is a valid task and returns it.
 * \param L a Lua state
 * \param index an index in the Lua stack
 * \return the C task corresponding to this Lua task
 */
static m_task_t sglua_checktask(lua_State* L, int index)
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
  m_task_t task = sglua_checktask(L, 1);
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
  m_task_t task = sglua_checktask(L, 1);
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
  m_task_t task = sglua_checktask(L, 1);
  MSG_error_t res = MSG_task_execute(task);

  if (res == MSG_OK) {
    return 0;
  }
  else {
    lua_pushstring(L, msg_errors[res]);
    return 1;
  }
}

/**
 * \brief Pops the Lua task from the stack and registers it so that the
 * process can retrieve it later knowing the C task.
 * \param L a lua state
 */
static void task_register(lua_State* L) {

  m_task_t task = sglua_checktask(L, -1);
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
static void task_unregister(lua_State* L, m_task_t task) {

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
 * This callback is used to copy the corresponding Lua task.
 *
 * \param task the task copied
 * \param src_process the sender
 * \param dst_process the receiver
 */
static void task_copy_callback(m_task_t task, m_process_t src_process,
    m_process_t dst_process) {

  lua_State* src = MSG_process_get_data(src_process);
  lua_State* dst = MSG_process_get_data(dst_process);

                                  /* src: ...
                                     dst: ... */
  task_unregister(src, task);
                                  /* src: ... task */
  sglua_copy_value(src, dst);
                                  /* src: ... task
                                     dst: ... task */
  task_register(dst);             /* dst: ... */

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
 * - Return values (boolean + string): true if the communication was successful,
 * or false plus an error string in case of failure, which may be "timeout",
 * "host failure" or "transfer failure"
 */
static int l_task_send(lua_State* L)
{
  m_task_t task = sglua_checktask(L, 1);
  const char* mailbox = luaL_checkstring(L, 2);
                                  /* task mailbox ... */
  lua_settop(L, 1);
                                  /* task */
  task_register(L);
                                  /* -- */
  MSG_error_t res = MSG_task_send(task, mailbox);

  if (res == MSG_OK) {
    lua_pushboolean(L, 1);
                                  /* true */
    return 1;
  }
  else {
    /* the communication has failed, I'm still the owner of the task */
    task_unregister(L, task);
                                  /* task */
    lua_pushboolean(L, 0);
                                  /* task false */
    lua_pushstring(L, msg_errors[res]);
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
  m_task_t task = sglua_checktask(L, 1);
  const char* mailbox = luaL_checkstring(L, 2);
                                  /* task mailbox ... */
  lua_settop(L, 1);
                                  /* task */
  task_register(L);
                                  /* -- */
  msg_comm_t comm = MSG_task_isend(task, mailbox);

  msg_comm_t* userdata = (msg_comm_t*) lua_newuserdata(L, sizeof(msg_comm_t));
                                  /* comm */
  *userdata = comm;
  luaL_getmetatable(L, COMM_MODULE_NAME);
                                  /* comm mt */
  lua_setmetatable(L, -2);
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
  m_task_t task = sglua_checktask(L, 1);
  const char* mailbox = luaL_checkstring(L, 2);
                                  /* task mailbox ... */
  lua_settop(L, 1);
                                  /* task */
  task_register(L);
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
  int timeout;
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
    task_unregister(L, task);
                                  /* mailbox ... task */
    return 1;
  }
  else {
    lua_pushnil(L);
                                  /* mailbox ... nil */
    lua_pushstring(L, msg_errors[res]);
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

  msg_comm_t* userdata = (msg_comm_t*) lua_newuserdata(L, sizeof(msg_comm_t));
                                  /* mailbox ... comm */
  *userdata = comm;
  luaL_getmetatable(L, COMM_MODULE_NAME);
                                  /* mailbox ... comm mt */
  lua_setmetatable(L, -2);
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

/* ********************************************************************************* */
/*                                simgrid.comm API                                   */
/* ********************************************************************************* */

/**
 * \brief Ensures that a value in the stack is a comm and returns it.
 * \param L a Lua state
 * \param index an index in the Lua stack
 * \return the C comm
 */
static msg_comm_t sglua_checkcomm(lua_State* L, int index)
{
  msg_comm_t comm = *((msg_comm_t*) luaL_checkudata(L, index, COMM_MODULE_NAME));
  return comm;
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

  msg_comm_t comm = sglua_checkcomm(L, 1);
  double timeout = -1;
  if (lua_gettop(L) >= 2) {
    timeout = luaL_checknumber(L, 2);
  }
                                  /* comm ... */
  MSG_error_t res = MSG_comm_wait(comm, timeout);

  if (res == MSG_OK) {
    m_task_t task = MSG_comm_get_task(comm);
    if (MSG_task_get_sender(task) == MSG_process_self()) {
      /* I'm the sender */
      return 0;
    }
    else {
      /* I'm the receiver: find the Lua task from the C task */
      task_unregister(L, task);
                                  /* comm ... task */
      return 1;
    }
  }
  else {
    /* the communication has failed */
    lua_pushnil(L);
                                  /* comm ... nil */
    lua_pushstring(L, msg_errors[res]);
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

  msg_comm_t comm = sglua_checkcomm(L, 1);
                                  /* comm ... */
  if (!MSG_comm_test(comm)) {
    /* not finished yet */
    lua_pushboolean(L, 0);
                                  /* comm ... false */
    return 1;
  }
  else {
    /* finished but may have failed */
    MSG_error_t res = MSG_comm_get_status(comm);

    if (res == MSG_OK) {
      m_task_t task = MSG_comm_get_task(comm);
      if (MSG_task_get_sender(task) == MSG_process_self()) {
        /* I'm the sender */
        lua_pushboolean(L, 1);
                                  /* comm ... true */
        return 1;
      }
      else {
        /* I'm the receiver: find the Lua task from the C task*/
        task_unregister(L, task);
                                  /* comm ... task */
        return 1;
      }
    }
    else {
      /* the communication has failed */
      lua_pushnil(L);
                                  /* comm ... nil */
      lua_pushstring(L, msg_errors[res]);
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

/* ********************************************************************************* */
/*                                simgrid.host API                                   */
/* ********************************************************************************* */

/**
 * \brief Ensures that a value in the stack is a host and returns it.
 * \param L a Lua state
 * \param index an index in the Lua stack
 * \return the C host corresponding to this Lua host
 */
static m_host_t sglua_checkhost(lua_State * L, int index)
{
  m_host_t *pi, ht;
  luaL_checktype(L, index, LUA_TTABLE);
  lua_getfield(L, index, "__simgrid_host");
  pi = (m_host_t *) luaL_checkudata(L, lua_gettop(L), HOST_MODULE_NAME);
  if (pi == NULL)
    luaL_typerror(L, index, HOST_MODULE_NAME);
  ht = *pi;
  if (!ht)
    luaL_error(L, "null Host");
  lua_pop(L, 1);
  return ht;
}

/**
 * \brief Returns a host given its name.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Argument 1 (string): name of a host
 * - Return value (host): the corresponding host
 */
static int l_host_get_by_name(lua_State * L)
{
  const char *name = luaL_checkstring(L, 1);
  XBT_DEBUG("Getting Host from name...");
  m_host_t msg_host = MSG_get_host_by_name(name);
  if (!msg_host) {
    luaL_error(L, "null Host : MSG_get_host_by_name failed");
  }
  lua_newtable(L);              /* create a table, put the userdata on top of it */
  m_host_t *lua_host = (m_host_t *) lua_newuserdata(L, sizeof(m_host_t));
  *lua_host = msg_host;
  luaL_getmetatable(L, HOST_MODULE_NAME);
  lua_setmetatable(L, -2);
  lua_setfield(L, -2, "__simgrid_host");        /* put the userdata as field of the table */
  /* remove the args from the stack */
  lua_remove(L, 1);
  return 1;
}

/**
 * \brief Returns the name of a host.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Argument 1 (host): a host
 * - Return value (string): name of this host
 */
static int l_host_get_name(lua_State * L)
{
  m_host_t ht = sglua_checkhost(L, 1);
  lua_pushstring(L, MSG_host_get_name(ht));
  return 1;
}

/**
 * \brief Returns the number of existing hosts.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Return value (number): number of hosts
 */
static int l_host_number(lua_State * L)
{
  lua_pushnumber(L, MSG_get_host_number());
  return 1;
}

/**
 * \brief Returns the host given its index.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Argument 1 (number): an index (1 is the first)
 * - Return value (host): the host at this index
 */
static int l_host_at(lua_State * L)
{
  int index = luaL_checkinteger(L, 1);
  m_host_t host = MSG_get_host_table()[index - 1];      // lua indexing start by 1 (lua[1] <=> C[0])
  lua_newtable(L);              /* create a table, put the userdata on top of it */
  m_host_t *lua_host = (m_host_t *) lua_newuserdata(L, sizeof(m_host_t));
  *lua_host = host;
  luaL_getmetatable(L, HOST_MODULE_NAME);
  lua_setmetatable(L, -2);
  lua_setfield(L, -2, "__simgrid_host");        /* put the userdata as field of the table */
  return 1;
}

/**
 * \brief Returns the host where the current process is located.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Return value (host): the current host
 */
static int l_host_self(lua_State * L)
{
                                  /* -- */
  m_host_t host = MSG_host_self();
  lua_newtable(L);
                                  /* table */
  m_host_t* lua_host = (m_host_t*) lua_newuserdata(L, sizeof(m_host_t));
                                  /* table ud */
  *lua_host = host;
  luaL_getmetatable(L, HOST_MODULE_NAME);
                                  /* table ud mt */
  lua_setmetatable(L, -2);
                                  /* table ud */
  lua_setfield(L, -2, "__simgrid_host");
                                  /* table */
  return 1;
}

/**
 * \brief Returns the value of a host property.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Argument 1 (host): a host
 * - Argument 2 (string): name of the property to get
 * - Return value (string): the value of this property
 */
static int l_host_get_property_value(lua_State * L)
{
  m_host_t ht = sglua_checkhost(L, 1);
  const char *prop = luaL_checkstring(L, 2);
  lua_pushstring(L,MSG_host_get_property_value(ht,prop));
  return 1;
}

/**
 * \brief Makes the current process sleep for a while.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Argument 1 (number): duration of the sleep
 */
static int l_host_sleep(lua_State *L)
{
  int time = luaL_checknumber(L, 1);
  MSG_process_sleep(time);
  return 0;
}

/**
 * \brief Destroys a host.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Argument 1 (host): the host to destroy
 */
static int l_host_destroy(lua_State *L)
{
  m_host_t ht = sglua_checkhost(L, 1);
  __MSG_host_destroy(ht);
  return 0;
}

static const luaL_reg host_functions[] = {
  {"get_by_name", l_host_get_by_name},
  {"name", l_host_get_name},
  {"number", l_host_number},
  {"at", l_host_at},
  {"self", l_host_self},
  {"get_prop_value", l_host_get_property_value},
  {"sleep", l_host_sleep},
  {"destroy", l_host_destroy},
  // Bypass XML Methods
  {"set_function", console_set_function},
  {"set_property", console_host_set_property},
  {NULL, NULL}
};

/**
 * \brief Returns a string representation of a host.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Argument 1 (userdata): a host
 * - Return value (string): a string describing this host
 */
static int l_host_tostring(lua_State * L)
{
  lua_pushfstring(L, "Host :%p", lua_touserdata(L, 1));
  return 1;
}

static const luaL_reg host_meta[] = {
  {"__tostring", l_host_tostring},
  {0, 0}
};

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
  MSG_error_t res = MSG_process_sleep(duration);

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

/* ********************************************************************************* */
/*                           lua_stub_generator functions                            */
/* ********************************************************************************* */

xbt_dict_t process_function_set;
xbt_dynar_t process_list;
xbt_dict_t machine_set;
static s_process_t process;

void s_process_free(void *process)
{
  s_process_t *p = (s_process_t *) process;
  int i;
  for (i = 0; i < p->argc; i++)
    free(p->argv[i]);
  free(p->argv);
  free(p->host);
}

static int gras_add_process_function(lua_State * L)
{
  const char *arg;
  const char *process_host = luaL_checkstring(L, 1);
  const char *process_function = luaL_checkstring(L, 2);

  if (xbt_dict_is_empty(machine_set)
      || xbt_dict_is_empty(process_function_set)
      || xbt_dynar_is_empty(process_list)) {
    process_function_set = xbt_dict_new_homogeneous(NULL);
    process_list = xbt_dynar_new(sizeof(s_process_t), s_process_free);
    machine_set = xbt_dict_new_homogeneous(NULL);
  }

  xbt_dict_set(machine_set, process_host, NULL, NULL);
  xbt_dict_set(process_function_set, process_function, NULL, NULL);

  process.argc = 1;
  process.argv = xbt_new(char *, 1);
  process.argv[0] = xbt_strdup(process_function);
  process.host = strdup(process_host);

  lua_pushnil(L);
  while (lua_next(L, 3) != 0) {
    arg = lua_tostring(L, -1);
    process.argc++;
    process.argv =
        xbt_realloc(process.argv, (process.argc) * sizeof(char *));
    process.argv[(process.argc) - 1] = xbt_strdup(arg);

    XBT_DEBUG("index = %f , arg = %s \n", lua_tonumber(L, -2),
           lua_tostring(L, -1));
    lua_pop(L, 1);
  }
  lua_pop(L, 1);
  //add to the process list
  xbt_dynar_push(process_list, &process);
  return 0;
}

static int gras_generate(lua_State * L)
{
  const char *project_name = luaL_checkstring(L, 1);
  generate_sim(project_name);
  generate_rl(project_name);
  generate_makefile_local(project_name);
  return 0;
}

/* ********************************************************************************* */
/*                               simgrid.platf API                                   */
/* ********************************************************************************* */

static const luaL_reg platf_functions[] = {
    {"open", console_open},
    {"close", console_close},
    {"AS_open", console_AS_open},
    {"AS_close", console_AS_close},
    {"host_new", console_add_host},
    {"link_new", console_add_link},
    {"router_new", console_add_router},
    {"route_new", console_add_route},
    {NULL, NULL}
};

/* ********************************************************************************* */
/*                                  simgrid API                                      */
/* ********************************************************************************* */

/**
 * \brief Deploys your application.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Argument 1 (string): name of the deployment file to load
 */
static int launch_application(lua_State* L) {

  const char* file = luaL_checkstring(L, 1);
  MSG_function_register_default(run_lua_code);
  MSG_launch_application(file);
  return 0;
}

/**
 * \brief Creates the platform.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Argument 1 (string): name of the platform file to load
 */
static int create_environment(lua_State* L) {

  const char* file = luaL_checkstring(L, 1);
  XBT_DEBUG("Loading environment file %s", file);
  MSG_create_environment(file);
  return 0;
}

/**
 * \brief Prints a log string with debug level.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Argument 1 (string): the text to print
 */
static int debug(lua_State* L) {

  const char* str = luaL_checkstring(L, 1);
  XBT_DEBUG("%s", str);
  return 0;
}

/**
 * \brief Prints a log string with info level.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Argument 1 (string): the text to print
 */
static int info(lua_State* L) {

  const char* str = luaL_checkstring(L, 1);
  XBT_INFO("%s", str);
  return 0;
}

/**
 * \brief Runs your application.
 * \param L a Lua state
 * \return number of values returned to Lua
 */
static int run(lua_State*  L) {

  MSG_main();
  return 0;
}

/**
 * \brief Returns the current simulated time.
 * \param L a Lua state
 * \return number of values returned to Lua
 *
 * - Return value (number): the simulated time
 */
static int get_clock(lua_State* L) {

  lua_pushnumber(L, MSG_get_clock());
  return 1;
}

/**
 * \brief Cleans the simulation.
 * \param L a Lua state
 * \return number of values returned to Lua
 */
static int simgrid_gc(lua_State * L)
{
  MSG_clean();
  return 0;
}

/*
 * Register platform for MSG
 */
static int msg_register_platform(lua_State * L)
{
  /* Tell Simgrid we dont wanna use its parser */
  //surf_parse = console_parse_platform;
  surf_parse_reset_callbacks();
  MSG_create_environment(NULL);
  return 0;
}

/*
 * Register platform for Simdag
 */

static int sd_register_platform(lua_State * L)
{
  //surf_parse = console_parse_platform_wsL07;
  surf_parse_reset_callbacks();
  SD_create_environment(NULL);
  return 0;
}

/*
 * Register platform for gras
 */
static int gras_register_platform(lua_State * L)
{
  //surf_parse = console_parse_platform;
  surf_parse_reset_callbacks();
  gras_create_environment(NULL);
  return 0;
}

/**
 * Register applicaiton for MSG
 */
static int msg_register_application(lua_State * L)
{
  MSG_function_register_default(run_lua_code);
  //surf_parse = console_parse_application;
  MSG_launch_application(NULL);
  return 0;
}

/*
 * Register application for gras
 */
static int gras_register_application(lua_State * L)
{
  gras_function_register_default(run_lua_code);
  //surf_parse = console_parse_application;
  gras_launch_application(NULL);
  return 0;
}

static const luaL_Reg simgrid_functions[] = {
  {"create_environment", create_environment},
  {"launch_application", launch_application},
  {"debug", debug},
  {"info", info},
  {"run", run},
  {"get_clock", get_clock},
  /* short names */
  {"platform", create_environment},
  {"application", launch_application},
  /* methods to bypass XML parser */
  {"msg_register_platform", msg_register_platform},
  {"sd_register_platform", sd_register_platform},
  {"msg_register_application", msg_register_application},
  {"gras_register_platform", gras_register_platform},
  {"gras_register_application", gras_register_application},
  /* gras sub generator method */
  {"gras_set_process_function", gras_add_process_function},
  {"gras_generate", gras_generate},
  {NULL, NULL}
};

/* ********************************************************************************* */
/*                           module management functions                             */
/* ********************************************************************************* */

#define LUA_MAX_ARGS_COUNT 10   /* maximum amount of arguments we can get from lua on command line */

/**
 * \brief Opens the simgrid Lua module.
 *
 * This function is called automatically by the Lua interpreter when some
 * Lua code requires the "simgrid" module.
 *
 * \param L the Lua state
 */
int luaopen_simgrid(lua_State *L)
{
  XBT_DEBUG("luaopen_simgrid *****");

  /* Get the command line arguments from the lua interpreter */
  char **argv = malloc(sizeof(char *) * LUA_MAX_ARGS_COUNT);
  int argc = 1;
  argv[0] = (char *) "/usr/bin/lua";    /* Lie on the argv[0] so that the stack dumping facilities find the right binary. FIXME: what if lua is not in that location? */

  lua_getglobal(L, "arg");
  /* if arg is a null value, it means we use lua only as a script to init platform
   * else it should be a table and then take arg in consideration
   */
  if (lua_istable(L, -1)) {
    int done = 0;
    while (!done) {
      argc++;
      lua_pushinteger(L, argc - 2);
      lua_gettable(L, -2);
      if (lua_isnil(L, -1)) {
        done = 1;
      } else {
        xbt_assert(lua_isstring(L, -1),
                    "argv[%d] got from lua is no string", argc - 1);
        xbt_assert(argc < LUA_MAX_ARGS_COUNT,
                    "Too many arguments, please increase LUA_MAX_ARGS_COUNT in %s before recompiling SimGrid if you insist on having more than %d args on command line",
                    __FILE__, LUA_MAX_ARGS_COUNT - 1);
        argv[argc - 1] = (char *) luaL_checkstring(L, -1);
        lua_pop(L, 1);
        XBT_DEBUG("Got command line argument %s from lua", argv[argc - 1]);
      }
    }
    argv[argc--] = NULL;

    /* Initialize the MSG core */
    MSG_global_init(&argc, argv);
    XBT_DEBUG("Still %d arguments on command line", argc); // FIXME: update the lua's arg table to reflect the changes from SimGrid
  }

  /* Keep the context mechanism informed of our lua world today */
  sglua_maestro_state = L;

  /* initialize access to my tables by children Lua states */
  lua_newtable(L);
  lua_setfield(L, LUA_REGISTRYINDEX, "simgrid.maestro_tables");

  register_c_functions(L);

  return 1;
}

/**
 * \brief Returns whether a Lua state is the maestro state.
 * \param L a Lua state
 * \return true if this is maestro
 */
int sglua_is_maestro(lua_State* L) {
  return L == sglua_maestro_state;
}

/**
 * \brief Returns the maestro state.
 * \return the maestro Lua state
 */
lua_State* sglua_get_maestro(void) {
  return sglua_maestro_state;
}

/**
 * \brief Registers the task functions into the table simgrid.task.
 *
 * Also initialize the metatable of the task userdata type.
 *
 * \param L a lua state
 */
static void register_task_functions(lua_State* L)
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

/**
 * \brief Registers the comm functions into the table simgrid.comm.
 *
 * Also initialize the metatable of the comm userdata type.
 *
 * \param L a lua state
 */
static void register_comm_functions(lua_State* L)
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

/**
 * \brief Registers the host functions into the table simgrid.host.
 *
 * Also initialize the metatable of the host userdata type.
 *
 * \param L a lua state
 */
static void register_host_functions(lua_State* L)
{
  /* create a table simgrid.host and fill it with host functions */
  luaL_openlib(L, HOST_MODULE_NAME, host_functions, 0);
                                  /* simgrid.host */

  /* create the metatable for host, add it to the Lua registry */
  luaL_newmetatable(L, HOST_MODULE_NAME);
                                  /* simgrid.host mt */
  /* fill the metatable */
  luaL_openlib(L, NULL, host_meta, 0);
                                  /* simgrid.host mt */
  lua_pushvalue(L, -2);
                                  /* simgrid.host mt simgrid.host */
  /* metatable.__index = simgrid.host
   * we put the host functions inside the host userdata itself:
   * this allows to write my_host:method(args) for
   * simgrid.host.method(my_host, args) */
  lua_setfield(L, -2, "__index");
                                  /* simgrid.host mt */
  lua_pop(L, 2);
                                  /* -- */
}

/**
 * \brief Registers the process functions into the table simgrid.process.
 * \param L a lua state
 */
static void register_process_functions(lua_State* L)
{
  luaL_openlib(L, PROCESS_MODULE_NAME, process_functions, 0);
                                  /* simgrid.process */
  lua_pop(L, 1);
}

/**
 * \brief Registers the platform functions into the table simgrid.platf.
 * \param L a lua state
 */
static void register_platf_functions(lua_State* L)
{
  luaL_openlib(L, PLATF_MODULE_NAME, platf_functions, 0);
                                  /* simgrid.platf */
  lua_pop(L, 1);
}

/**
 * \brief Makes the core functions available to the Lua world.
 * \param L a Lua world
 */
static void register_core_functions(lua_State *L)
{
  /* register the core C functions to lua */
  luaL_register(L, "simgrid", simgrid_functions);
                                  /* simgrid */

  /* set a finalizer that cleans simgrid, by adding to the simgrid module a
   * dummy userdata whose __gc metamethod calls MSG_clean() */
  lua_newuserdata(L, sizeof(void*));
                                  /* simgrid udata */
  lua_newtable(L);
                                  /* simgrid udata mt */
  lua_pushcfunction(L, simgrid_gc);
                                  /* simgrid udata mt simgrid_gc */
  lua_setfield(L, -2, "__gc");
                                  /* simgrid udata mt */
  lua_setmetatable(L, -2);
                                  /* simgrid udata */
  lua_setfield(L, -2, "__simgrid_loaded");
                                  /* simgrid */
  lua_pop(L, 1);
                                  /* -- */
}

/**
 * \brief Creates the simgrid module and make it available to Lua.
 * \param L a Lua world
 */
static void register_c_functions(lua_State *L)
{
  register_core_functions(L);
  register_task_functions(L);
  register_comm_functions(L);
  register_host_functions(L);
  register_process_functions(L);
  register_platf_functions(L);
}

/**
 * \brief Runs a Lua function as a new simulated process.
 * \param argc number of arguments of the function
 * \param argv name of the Lua function and array of its arguments
 * \return result of the function
 */
static int run_lua_code(int argc, char **argv)
{
  XBT_DEBUG("Run lua code %s", argv[0]);

  /* create a new state, getting globals from maestro */
  lua_State *L = sglua_clone_maestro();
  MSG_process_set_data(MSG_process_self(), L);

  /* start the function */
  lua_getglobal(L, argv[0]);
  xbt_assert(lua_isfunction(L, -1),
              "There is no Lua function with name `%s'", argv[0]);

  /* push arguments onto the stack */
  int i;
  for (i = 1; i < argc; i++)
    lua_pushstring(L, argv[i]);

  /* call the function */
  _XBT_GNUC_UNUSED int err;
  err = lua_pcall(L, argc - 1, 1, 0);
  xbt_assert(err == 0, "Error running function `%s': %s", argv[0],
              lua_tostring(L, -1));

  /* retrieve result */
  int res = 1;
  if (lua_isnumber(L, -1)) {
    res = lua_tonumber(L, -1);
    lua_pop(L, 1);              /* pop returned value */
  }

  XBT_DEBUG("Execution of Lua code %s is over", (argv ? argv[0] : "(null)"));

  return res;
}
