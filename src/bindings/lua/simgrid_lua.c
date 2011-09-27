/* SimGrid Lua bindings                                                     */

/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include "simgrid_lua.h"
#include <string.h> // memcpy

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(lua, bindings, "Lua Bindings");

static lua_State *lua_maestro_state;

#define TASK_MODULE_NAME "simgrid.Task"
#define HOST_MODULE_NAME "simgrid.Host"
// Surf (bypass XML)
#define LINK_MODULE_NAME "simgrid.Link"
#define ROUTE_MODULE_NAME "simgrid.Route"
#define AS_MODULE_NAME "simgrid.AS"
#define TRACE_MODULE_NAME "simgrid.Trace"

/**
 * @brief A chunk of memory.
 *
 * TODO replace this by a dynar
 */
typedef struct s_buffer {
  char* data;
  size_t size;
  size_t capacity;
} s_buffer_t, *buffer_t;

static const char* value_tostring(lua_State* L, int index);
static const char* keyvalue_tostring(lua_State* L, int key_index, int value_index);
static void stack_dump(const char *msg, lua_State* L);
static int writer(lua_State* L, const void* source, size_t size, void* userdata);
static void move_value(lua_State* dst, lua_State* src, const char* name);
static int l_get_from_father(lua_State* L);
static lua_State *clone_lua_state(lua_State* L);
static m_task_t check_task(lua_State *L, int index);
static void register_c_functions(lua_State *L);

/* ********************************************************************************* */
/*                            helper functions                                       */
/* ********************************************************************************* */

/**
 * @brief Returns a string representation of a value in the Lua stack.
 *
 * This function is for debugging purposes.
 * It always returns the same pointer.
 *
 * @param L the Lua state
 * @param index index in the stack
 * @return a string representation of the value at this index
 */
static const char* value_tostring(lua_State* L, int index) {

  static char buff[64];

  switch (lua_type(L, index)) {

    case LUA_TNIL:
      sprintf(buff, "nil");
      break;

    case LUA_TNUMBER:
      sprintf(buff, "%.3f", lua_tonumber(L, index));
      break;

    case LUA_TBOOLEAN:
      sprintf(buff, "%s", lua_toboolean(L, index) ? "true" : "false");
      break;

    case LUA_TSTRING:
      snprintf(buff, 63, "'%s'", lua_tostring(L, index));
      break;

    case LUA_TFUNCTION:
      if (lua_iscfunction(L, index)) {
        sprintf(buff, "C-function");
      }
      else {
        sprintf(buff, "function");
      }
      break;

    case LUA_TTABLE:
      sprintf(buff, "table(%zu)", lua_objlen(L, index));
      break;

    case LUA_TLIGHTUSERDATA:
    case LUA_TUSERDATA:
      sprintf(buff, "userdata(%p)", lua_touserdata(L, index));
      break;

    case LUA_TTHREAD:
      sprintf(buff, "thread");
      break;
  }
  return buff;
}

/**
 * @brief Returns a string representation of a key-value pair.
 *
 * This function is for debugging purposes.
 * It always returns the same pointer.
 *
 * @param L the Lua state
 * @param key_index index of the key
 * @param value_index index of the value
 * @return a string representation of the key-value pair
 */
static const char* keyvalue_tostring(lua_State* L, int key_index, int value_index) {

  static char buff[64];
  /* value_tostring also always returns the same pointer */
  int len = snprintf(buff, 63, "%s -> ", value_tostring(L, key_index));
  snprintf(buff + len, 63 - len, "%s", value_tostring(L, value_index));
  return buff;
}

/**
 * @brief Returns a string composed of the specified number of spaces.
 *
 * This function is for debugging purposes.
 * It always returns the same pointer.
 *
 * @param length length of the string
 * @return a string of this length with only spaces
 */
static const char* get_spaces(int length) {

  static char spaces[128];

  xbt_assert(length < 128);
  memset(spaces, ' ', length);
  spaces[length] = '\0';
  return spaces;
}

/**
 * @brief Dumps the Lua stack if debug logs are enabled.
 * @param msg a message to print
 * @param L a Lua state
 */
static void stack_dump(const char* msg, lua_State* L)
{
  if (XBT_LOG_ISENABLED(lua, xbt_log_priority_debug)) {
    char buff[2048];
    char* p = buff;
    int i;
    int top = lua_gettop(L);

    if (1) return;

    fflush(stdout);

    p[0] = '\0';
    for (i = 1; i <= top; i++) {  /* repeat for each level */

      p += sprintf(p, "%s", value_tostring(L, i));
      p += sprintf(p, " ");       /* put a separator */
    }
    XBT_DEBUG("%s%s", msg, buff);
  }
}

/**
 * @brief Writes the specified data into a memory buffer.
 *
 * This function is a valid lua_Writer that writes into a memory buffer passed
 * as userdata.
 *
 * @param L a lua state
 * @param source some data
 * @param sz number of bytes of data
 * @param user_data the memory buffer to write
 */
static int writer(lua_State* L, const void* source, size_t size, void* userdata) {

  buffer_t buffer = (buffer_t) userdata;
  while (buffer->capacity < buffer->size + size) {
    buffer->capacity *= 2;
    buffer->data = xbt_realloc(buffer->data, buffer->capacity);
  }
  memcpy(buffer->data + buffer->size, source, size);
  buffer->size += size;

  return 0;
}

/**
 * @brief Pops a value from the stack of a source state and pushes it on the
 * stack of another state.
 *
 * If the value is a table, its content is copied recursively. To avoid cycles,
 * a table of previsously visited tables must be present at index 1 of dst.
 * Its keys are pointers to visited tables in src and its values are the tables
 * already built.
 *
 * TODO: add support of closures
 *
 * @param src the source state
 * @param dst the destination state
 * @param name a name describing the value
 */
static void move_value(lua_State* dst, lua_State *src, const char* name) {

  luaL_checkany(src, -1);                  /* check the value to copy */
  luaL_checktype(dst, 1, LUA_TTABLE);      /* check the presence of a table of
                                              previously visited tables */

  int indentation_level = (lua_gettop(dst) - 1) * 6;
  const char* indent = get_spaces(indentation_level);

  XBT_DEBUG("%sCopying value %s", indent, name);
  indent = get_spaces(indentation_level + 2);

  stack_dump("src before copying a value (should be ... value): ", src);
  stack_dump("dst before copying a value (should be visited ...): ", dst);

  switch (lua_type(src, -1)) {

    case LUA_TNIL:
      lua_pushnil(dst);
      break;

    case LUA_TNUMBER:
      lua_pushnumber(dst, lua_tonumber(src, -1));
      break;

    case LUA_TBOOLEAN:
      lua_pushboolean(dst, lua_toboolean(src, -1));
      break;

    case LUA_TSTRING:
      /* no worries about memory: lua_pushstring makes a copy */
      lua_pushstring(dst, lua_tostring(src, -1));
      break;

    case LUA_TFUNCTION:
      /* it's a function that does not exist yet in L2 */

      if (lua_iscfunction(src, -1)) {
        /* it's a C function: just copy the pointer */
        lua_CFunction f = lua_tocfunction(src, -1);
        lua_pushcfunction(dst, f);
      }
      else {
        /* it's a Lua function: dump it from src */
        XBT_DEBUG("%sDumping Lua function '%s'", indent, name);

        s_buffer_t buffer;
        buffer.capacity = 64;
        buffer.size = 0;
        buffer.data = xbt_new(char, buffer.capacity);

        /* copy the binary chunk from src into a buffer */
        int error = lua_dump(src, writer, &buffer);
        xbt_assert(!error, "Failed to dump function '%s' from the source state: error %d",
              name, error);

        /* load the chunk into dst */
        error = luaL_loadbuffer(dst, buffer.data, buffer.size, name);
        xbt_assert(!error, "Failed to load function '%s' from the source state: %s",
            name, lua_tostring(dst, -1));
        XBT_DEBUG("%sFunction '%s' successfully dumped from source state.", indent, name);
      }
      break;

    case LUA_TTABLE:

      /* see if this table was already visited */
      lua_pushlightuserdata(dst, (void*) lua_topointer(src, -1));
                                  /* dst: visited ... psrctable */
      lua_gettable(dst, 1);
                                  /* dst: visited ... table/nil */
      if (lua_istable(dst, -1)) {
        XBT_DEBUG("%sNothing to do: table already visited", indent);
                                  /* dst: visited ... table */
      }
      else {
        XBT_DEBUG("%sFirst visit of this table", indent);
                                  /* dst: visited ... nil */
        lua_pop(dst, 1);
                                  /* dst: visited ... */

        /* first visit: create the new table in dst */
        lua_newtable(dst);
                                  /* dst: visited ... table */

        /* mark the table as visited to avoid infinite recursion */
        lua_pushlightuserdata(dst, (void*) lua_topointer(src, -1));
                                  /* dst: visited ... table psrctable */
        lua_pushvalue(dst, -2);
                                  /* dst: visited ... table psrctable table */
        lua_settable(dst, 1);
                                  /* dst: visited ... table */
        XBT_DEBUG("%sTable marked as visited", indent);

        stack_dump("dst after marking the table as visited (should be visited ... table): ", dst);

        /* copy the metatable if any */
        int has_meta_table = lua_getmetatable(src, -1);
                                  /* src: ... table mt? */
        if (has_meta_table) {
          XBT_DEBUG("%sCopying metatable", indent);
                                  /* src: ... table mt */
          move_value(dst, src, "metatable");
                                  /* src: ... table
                                     dst: visited ... table mt */
          lua_setmetatable(dst, -2);
                                  /* dst: visited ... table */
        }
        else {
          XBT_DEBUG("%sNo metatable", indent);
        }

        stack_dump("src before traversing the table (should be ... table): ", src);
        stack_dump("dst before traversing the table (should be visited ... table): ", dst);

        /* traverse the table of src and copy each element */
        lua_pushnil(src);
                                  /* src: ... table nil */
        while (lua_next(src, -2) != 0) {
                                  /* src: ... table key value */

          XBT_DEBUG("%sCopying table element %s", indent, keyvalue_tostring(src, -2, -1));
          indent = get_spaces(indentation_level + 4);

          stack_dump("src before copying table element (should be ... table key value): ", src);
          stack_dump("dst before copying table element (should be visited ... table): ", dst);

          /* copy the key */
          lua_pushvalue(src, -2);
                                  /* src: ... table key value key */
          XBT_DEBUG("%sCopying the key part of the table element", indent);
          move_value(dst, src, value_tostring(src, -1));
                                  /* src: ... table key value
                                     dst: visited ... table key */
          XBT_DEBUG("%sCopied the key part of the table element", indent);

          /* copy the value */
          XBT_DEBUG("%sCopying the value part of the table element", indent);
          move_value(dst, src, value_tostring(src, -1));
                                  /* src: ... table key
                                     dst: visited ... table key value */
          XBT_DEBUG("%sCopied the value part of the table element", indent);

          /* set the table element */
          lua_settable(dst, -3);
                                  /* dst: visited ... table */

          /* the key stays on top of src for next iteration */
          stack_dump("src before next iteration (should be ... table key): ", src);
          stack_dump("dst before next iteration (should be visited ... table): ", dst);

          indent = get_spaces(indentation_level + 2);
        }
        XBT_DEBUG("%sFinished traversing the table", indent);
      }
      break;

    case LUA_TLIGHTUSERDATA:
      lua_pushlightuserdata(dst, lua_touserdata(src, -1));
      break;

    case LUA_TUSERDATA:
      XBT_WARN("Copying a full userdata is not supported yet.");
      lua_pushnil(dst);
      break;

    case LUA_TTHREAD:
      XBT_WARN("Cannot copy a thread from the source state.");
      lua_pushnil(dst);
      break;
  }

  /* pop the value from src */
  lua_pop(src, 1);

  indent = get_spaces(indentation_level);
  XBT_DEBUG("%sCopied the value", indent);

  stack_dump("src after copying a value (should be ...): ", src);
  stack_dump("dst after copying a value (should be visited ... value): ", dst);
}

/**
 * @brief Copies a global value from the father state.
 *
 * The state L must have a father, i.e. it should have been created by
 * clone_lua_state().
 * This function is meant to be an __index metamethod.
 * Consequently, it assumes that the stack has two elements:
 * a table (usually the environment of L) and the string key of a value
 * that does not exist yet in this table. It copies the corresponding global
 * value from the father state and pushes it on the stack of L.
 * If the global value does not exist in the father state either, nil is
 * pushed.
 *
 * TODO: make this function thread safe. If the simulation runs in parallel,
 * several simulated processes may trigger this __index metamethod at the same
 * time and get globals from maestro.
 *
 * @param L the current state
 * @return number of return values pushed (always 1)
 */
static int l_get_from_father(lua_State *L) {

  /* retrieve the father */
  lua_getfield(L, LUA_REGISTRYINDEX, "simgrid.father_state");
  lua_State* father = lua_touserdata(L, -1);
  xbt_assert(father != NULL, "This Lua state has no father");
  lua_pop(L, 1);

  /* get the global from the father */
  const char* key = luaL_checkstring(L, 2);    /* L:      table key */
  lua_getglobal(father, key);                  /* father: ... value */
  XBT_DEBUG("__index of '%s' begins", key);

  /* push the value onto the stack of L */
  lua_newtable(L);                             /* L:      table key visited */
  lua_insert(L, 1);                            /* L:      visited table key */
  move_value(L, father, key);                  /* father: ...
                                                  L:      visited table key value */
  lua_remove(L, 1);                            /* L:      table key value */

  /* prepare the return value of __index */
  lua_pushvalue(L, -1);                        /* L:      table key value value */
  lua_insert(L, 1);                            /* L:      value table key value */

  /* save the copied value in the table for subsequent accesses */
  lua_settable(L, -3);                         /* L:      value table */
  lua_remove(L, 2);                            /* L:      value */

  XBT_DEBUG("__index of '%s' returns %s", key, value_tostring(L, -1));

  return 1;
}

/**
 * @brief Creates a new Lua state and get its environment from an existing state.
 *
 * The state created is independent from the existing one and has its own
 * copies of global variables and functions.
 * However, the global variables and functions are not copied right now from
 * the original state; they are copied only the first time they are accessed.
 * This behavior saves time and memory, and is okay for Simgrid's needs.
 *
 * @param father an existing state
 * @return the state created
 */
static lua_State* clone_lua_state(lua_State *father) {

  /* create the new state */
  lua_State *L = luaL_newstate();

  /* set its environment:
   * - create a table newenv
   * - create a metatable mt
   * - set mt.__index = a function that copies the global from the father state
   * - set mt as the metatable of newenv
   * - set newenv as the environment of the new state
   */
  lua_pushthread(L);                        /* thread */
  lua_newtable(L);                          /* thread newenv */
  lua_newtable(L);                          /* thread newenv mt */
  lua_pushcfunction(L, l_get_from_father);  /* thread newenv mt f */
  lua_setfield(L, -2, "__index");           /* thread newenv mt */
  lua_setmetatable(L, -2);                  /* thread newenv */
  lua_setfenv(L, -2);                       /* thread */
  lua_pop(L, 1);                            /* -- */

  /* open the standard libs (theoretically, this is not necessary as they can
   * be inherited like any global values, but without a proper support of
   * closures, iterators like ipairs don't work). */
  luaL_openlibs(L);

  /* set a pointer to the father */
  lua_pushlightuserdata(L, father);
  lua_setfield(L, LUA_REGISTRYINDEX, "simgrid.father_state");

  /* copy the registry */
  lua_pushvalue(father, LUA_REGISTRYINDEX);    /* father: ... reg */
  lua_newtable(L);                             /* L: visited */
  move_value(L, father, "registry");           /* father: ...
                                                  L: visited newreg */

  /* set the pointer again */
  lua_pushlightuserdata(L, father);            /* L: visited newreg father */
  lua_setfield(L, -1, "simgrid.father_state"); /* L: visited newreg */
  lua_replace(L, LUA_REGISTRYINDEX);           /* L: visited */
  lua_pop(L, 1);                               /* L: -- */

  XBT_DEBUG("New state created");

  return L;
}

/**
 * @brief Ensures that a userdata on the stack is a task
 * and returns the pointer inside the userdata.
 * @param L a Lua state
 * @param index an index in the Lua stack
 * @return the task at this index
 */
static m_task_t checkTask(lua_State * L, int index)
{
  m_task_t *pi, tk;
  luaL_checktype(L, index, LUA_TTABLE);
  lua_getfield(L, index, "__simgrid_task");
  pi = (m_task_t *) luaL_checkudata(L, -1, TASK_MODULE_NAME);
  if (pi == NULL)
    luaL_typerror(L, index, TASK_MODULE_NAME);
  tk = *pi;
  if (!tk)
    luaL_error(L, "null Task");
  lua_pop(L, 1);
  return tk;
}

/* ********************************************************************************* */
/*                           wrapper functions                                       */
/* ********************************************************************************* */

/**
 * A task is either something to compute somewhere, or something to exchange between two hosts (or both).
 * It is defined by a computing amount and a message size.
 *
 */

/* *              * *
 * * Constructors * *
 * *              * */

/**
 * @brief Constructs a new task with the specified processing amount and amount
 * of data needed.
 *
 * @param name	Task's name
 *
 * @param computeDuration 	A value of the processing amount (in flop) needed to process the task.
 *				If 0, then it cannot be executed with the execute() method.
 *				This value has to be >= 0.
 *
 * @param messageSize		A value of amount of data (in bytes) needed to transfert this task.
 *				If 0, then it cannot be transfered with the get() and put() methods.
 *				This value has to be >= 0.
 */
static int Task_new(lua_State * L)
{
  XBT_DEBUG("Task new...");
  const char *name = luaL_checkstring(L, 1);
  int comp_size = luaL_checkint(L, 2);
  int msg_size = luaL_checkint(L, 3);
  m_task_t msg_task = MSG_task_create(name, comp_size, msg_size, NULL);
  lua_newtable(L);              /* create a table, put the userdata on top of it */
  m_task_t *lua_task = (m_task_t *) lua_newuserdata(L, sizeof(m_task_t));
  *lua_task = msg_task;
  luaL_getmetatable(L, TASK_MODULE_NAME);
  lua_setmetatable(L, -2);
  lua_setfield(L, -2, "__simgrid_task");        /* put the userdata as field of the table */
  /* remove the args from the stack */
  lua_remove(L, 1);
  lua_remove(L, 1);
  lua_remove(L, 1);
  return 1;
}

static int Task_get_name(lua_State * L)
{
  m_task_t tk = checkTask(L, -1);
  lua_pushstring(L, MSG_task_get_name(tk));
  return 1;
}

static int Task_computation_duration(lua_State * L)
{
  m_task_t tk = checkTask(L, -1);
  lua_pushnumber(L, MSG_task_get_compute_duration(tk));
  return 1;
}

static int Task_execute(lua_State * L)
{
  m_task_t tk = checkTask(L, -1);
  int res = MSG_task_execute(tk);
  lua_pushnumber(L, res);
  return 1;
}

static int Task_destroy(lua_State * L)
{
  m_task_t tk = checkTask(L, -1);
  int res = MSG_task_destroy(tk);
  lua_pushnumber(L, res);
  return 1;
}

static int Task_send(lua_State * L)
{
  //stackDump("send ",L);
  m_task_t tk = checkTask(L, 1);
  const char *mailbox = luaL_checkstring(L, 2);
  lua_pop(L, 1);                // remove the string so that the task is on top of it
  MSG_task_set_data(tk, L);     // Copy my stack into the task, so that the receiver can copy the lua task directly
  MSG_error_t res = MSG_task_send(tk, mailbox);
  while (MSG_task_get_data(tk) != NULL) // Don't mess up with my stack: the receiver didn't copy the data yet
    MSG_process_sleep(0);       // yield

  if (res != MSG_OK)
    switch (res) {
    case MSG_TIMEOUT:
      XBT_DEBUG("MSG_task_send failed : Timeout");
      break;
    case MSG_TRANSFER_FAILURE:
      XBT_DEBUG("MSG_task_send failed : Transfer Failure");
      break;
    case MSG_HOST_FAILURE:
      XBT_DEBUG("MSG_task_send failed : Host Failure ");
      break;
    default:
      XBT_ERROR
          ("MSG_task_send failed : Unexpected error , please report this bug");
      break;
    }
  return 0;
}

static int Task_recv_with_timeout(lua_State *L)
{
  m_task_t tk = NULL;
  const char *mailbox = luaL_checkstring(L, -2);
  int timeout = luaL_checknumber(L, -1);
  MSG_error_t res = MSG_task_receive_with_timeout(&tk, mailbox, timeout);

  if (res == MSG_OK) {
    lua_State *sender_stack = MSG_task_get_data(tk);
    lua_xmove(sender_stack, L, 1);        // copy the data directly from sender's stack
    MSG_task_set_data(tk, NULL);
  }
  else {
    switch (res) {
    case MSG_TIMEOUT:
      XBT_DEBUG("MSG_task_receive failed : Timeout");
      break;
    case MSG_TRANSFER_FAILURE:
      XBT_DEBUG("MSG_task_receive failed : Transfer Failure");
      break;
    case MSG_HOST_FAILURE:
      XBT_DEBUG("MSG_task_receive failed : Host Failure ");
      break;
    default:
      XBT_ERROR("MSG_task_receive failed : Unexpected error , please report this bug");
      break;
    }
    lua_pushnil(L);
  }
  return 1;
}

static int Task_recv(lua_State * L)
{
  lua_pushnumber(L, -1.0);
  return Task_recv_with_timeout(L);
}

static const luaL_reg Task_methods[] = {
  {"new", Task_new},
  {"name", Task_get_name},
  {"computation_duration", Task_computation_duration},
  {"execute", Task_execute},
  {"destroy", Task_destroy},
  {"send", Task_send},
  {"recv", Task_recv},
  {"recv_timeout", Task_recv_with_timeout},
  {NULL, NULL}
};

static int Task_gc(lua_State * L)
{
  m_task_t tk = checkTask(L, -1);
  if (tk)
    MSG_task_destroy(tk);
  return 0;
}

static int Task_tostring(lua_State * L)
{
  lua_pushfstring(L, "Task :%p", lua_touserdata(L, 1));
  return 1;
}

static const luaL_reg Task_meta[] = {
  {"__gc", Task_gc},
  {"__tostring", Task_tostring},
  {NULL, NULL}
};

/**
 * Host
 */
static m_host_t checkHost(lua_State * L, int index)
{
  m_host_t *pi, ht;
  luaL_checktype(L, index, LUA_TTABLE);
  lua_getfield(L, index, "__simgrid_host");
  pi = (m_host_t *) luaL_checkudata(L, -1, HOST_MODULE_NAME);
  if (pi == NULL)
    luaL_typerror(L, index, HOST_MODULE_NAME);
  ht = *pi;
  if (!ht)
    luaL_error(L, "null Host");
  lua_pop(L, 1);
  return ht;
}

static int Host_get_by_name(lua_State * L)
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

static int Host_get_name(lua_State * L)
{
  m_host_t ht = checkHost(L, -1);
  lua_pushstring(L, MSG_host_get_name(ht));
  return 1;
}

static int Host_number(lua_State * L)
{
  lua_pushnumber(L, MSG_get_host_number());
  return 1;
}

static int Host_at(lua_State * L)
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

static int Host_self(lua_State * L)
{
  m_host_t host = MSG_host_self();
  lua_newtable(L);
  m_host_t *lua_host =(m_host_t *)lua_newuserdata(L,sizeof(m_host_t));
  *lua_host = host;
  luaL_getmetatable(L, HOST_MODULE_NAME);
  lua_setmetatable(L, -2);
  lua_setfield(L, -2, "__simgrid_host");
  return 1;
}

static int Host_get_property_value(lua_State * L)
{
  m_host_t ht = checkHost(L, -2);
  const char *prop = luaL_checkstring(L, -1);
  lua_pushstring(L,MSG_host_get_property_value(ht,prop));
  return 1;
}

static int Host_sleep(lua_State *L)
{
  int time = luaL_checknumber(L, -1);
  MSG_process_sleep(time);
  return 1;
}

static int Host_destroy(lua_State *L)
{
  m_host_t ht = checkHost(L, -1);
  __MSG_host_destroy(ht);
  return 1;
}

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
    process_function_set = xbt_dict_new();
    process_list = xbt_dynar_new(sizeof(s_process_t), s_process_free);
    machine_set = xbt_dict_new();
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

/***********************************
 * 	Tracing
 **********************************/
static int trace_start(lua_State *L)
{
#ifdef HAVE_TRACING
  TRACE_start();
#endif
  return 1;
}

static int trace_category(lua_State * L)
{
#ifdef HAVE_TRACING
  TRACE_category(luaL_checkstring(L, 1));
#endif
  return 1;
}

static int trace_set_task_category(lua_State *L)
{
#ifdef HAVE_TRACING
  TRACE_msg_set_task_category(checkTask(L, -2), luaL_checkstring(L, -1));
#endif
  return 1;
}

static int trace_end(lua_State *L)
{
#ifdef HAVE_TRACING
  TRACE_end();
#endif
  return 1;
}

// *********** Register Methods ******************************************* //

/*
 * Host Methods
 */
static const luaL_reg Host_methods[] = {
  {"getByName", Host_get_by_name},
  {"name", Host_get_name},
  {"number", Host_number},
  {"at", Host_at},
  {"self", Host_self},
  {"getPropValue", Host_get_property_value},
  {"sleep", Host_sleep},
  {"destroy", Host_destroy},
  // Bypass XML Methods
  {"setFunction", console_set_function},
  {"setProperty", console_host_set_property},
  {NULL, NULL}
};

static int Host_gc(lua_State * L)
{
  m_host_t ht = checkHost(L, -1);
  if (ht)
    ht = NULL;
  return 0;
}

static int Host_tostring(lua_State * L)
{
  lua_pushfstring(L, "Host :%p", lua_touserdata(L, 1));
  return 1;
}

static const luaL_reg Host_meta[] = {
  {"__gc", Host_gc},
  {"__tostring", Host_tostring},
  {0, 0}
};

/*
 * AS Methods
 */
static const luaL_reg AS_methods[] = {
  {"new", console_add_AS},
  {"addHost", console_add_host},
  {"addLink", console_add_link},
  {"addRoute", console_add_route},
  {NULL, NULL}
};

/**
 * Tracing Functions
 */
static const luaL_reg Trace_methods[] = {
  {"start", trace_start},
  {"category", trace_category},
  {"setTaskCategory", trace_set_task_category},
  {"finish", trace_end},
  {NULL, NULL}
};

/*
 * Environment related
 */

/**
 * @brief Runs a Lua function as a new simulated process.
 * @param argc number of arguments of the function
 * @param argv name of the Lua function and array of its arguments
 * @return result of the function
 */
static int run_lua_code(int argc, char **argv)
{
  XBT_DEBUG("Run lua code %s", argv[0]);

  lua_State *L = clone_lua_state(lua_maestro_state);
  int res = 1;

  /* start the function */
  lua_getglobal(L, argv[0]);
  xbt_assert(lua_isfunction(L, -1),
              "The lua function %s does not seem to exist", argv[0]);

  /* push arguments onto the stack */
  int i;
  for (i = 1; i < argc; i++)
    lua_pushstring(L, argv[i]);

  /* call the function */
  int err;
  err = lua_pcall(L, argc - 1, 1, 0);
  xbt_assert(err == 0, "error running function `%s': %s", argv[0],
              lua_tostring(L, -1));

  /* retrieve result */
  if (lua_isnumber(L, -1)) {
    res = lua_tonumber(L, -1);
    lua_pop(L, 1);              /* pop returned value */
  }

  XBT_DEBUG("Execution of Lua code %s is over", (argv ? argv[0] : "(null)"));

  return res;
}

static int launch_application(lua_State * L)
{
  const char *file = luaL_checkstring(L, 1);
  MSG_function_register_default(run_lua_code);
  MSG_launch_application(file);
  return 0;
}

static int create_environment(lua_State * L)
{
  const char *file = luaL_checkstring(L, 1);
  XBT_DEBUG("Loading environment file %s", file);
  MSG_create_environment(file);
  return 0;
}

static int debug(lua_State * L)
{
  const char *str = luaL_checkstring(L, 1);
  XBT_DEBUG("%s", str);
  return 0;
}

static int info(lua_State * L)
{
  const char *str = luaL_checkstring(L, 1);
  XBT_INFO("%s", str);
  return 0;
}

static int run(lua_State * L)
{
  MSG_main();
  return 0;
}

static int clean(lua_State * L)
{
  MSG_clean();
  return 0;
}

/*
 * Bypass XML Parser (lua console)
 */

/*
 * Register platform for MSG
 */
static int msg_register_platform(lua_State * L)
{
  /* Tell Simgrid we dont wanna use its parser */
  surf_parse = console_parse_platform;
  surf_parse_reset_callbacks();
  surf_config_models_setup(NULL);
  MSG_create_environment(NULL);
  return 0;
}

/*
 * Register platform for Simdag
 */

static int sd_register_platform(lua_State * L)
{
  surf_parse = console_parse_platform_wsL07;
  surf_parse_reset_callbacks();
  surf_config_models_setup(NULL);
  SD_create_environment(NULL);
  return 0;
}

/*
 * Register platform for gras
 */
static int gras_register_platform(lua_State * L)
{
  /* Tell Simgrid we dont wanna use surf parser */
  surf_parse = console_parse_platform;
  surf_parse_reset_callbacks();
  surf_config_models_setup(NULL);
  gras_create_environment(NULL);
  return 0;
}

/**
 * Register applicaiton for MSG
 */
static int msg_register_application(lua_State * L)
{
  MSG_function_register_default(run_lua_code);
  surf_parse = console_parse_application;
  MSG_launch_application(NULL);
  return 0;
}

/*
 * Register application for gras
 */
static int gras_register_application(lua_State * L)
{
  gras_function_register_default(run_lua_code);
  surf_parse = console_parse_application;
  gras_launch_application(NULL);
  return 0;
}

static const luaL_Reg simgrid_funcs[] = {
  {"create_environment", create_environment},
  {"launch_application", launch_application},
  {"debug", debug},
  {"info", info},
  {"run", run},
  {"clean", clean},
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
/*                       module management functions                                 */
/* ********************************************************************************* */

#define LUA_MAX_ARGS_COUNT 10   /* maximum amount of arguments we can get from lua on command line */

int luaopen_simgrid(lua_State *L);     // Fuck gcc: we don't need that prototype

/**
 * This function is called automatically by the Lua interpreter when some Lua code requires
 * the "simgrid" module.
 * @param L the Lua state
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
  lua_maestro_state = L;

  register_c_functions(L);

  return 1;
}

/**
 * Makes the appropriate Simgrid functions available to the Lua world.
 * @param L a Lua world
 */
void register_c_functions(lua_State *L) {

  /* register the core C functions to lua */
  luaL_register(L, "simgrid", simgrid_funcs);

  /* register the task methods to lua */
  luaL_openlib(L, TASK_MODULE_NAME, Task_methods, 0);   // create methods table, add it to the globals
  luaL_newmetatable(L, TASK_MODULE_NAME);       // create metatable for Task, add it to the Lua registry
  luaL_openlib(L, 0, Task_meta, 0);     // fill metatable
  lua_pushliteral(L, "__index");
  lua_pushvalue(L, -3);         // dup methods table
  lua_rawset(L, -3);            // matatable.__index = methods
  lua_pushliteral(L, "__metatable");
  lua_pushvalue(L, -3);         // dup methods table
  lua_rawset(L, -3);            // hide metatable:metatable.__metatable = methods
  lua_pop(L, 1);                // drop metatable

  /* register the hosts methods to lua */
  luaL_openlib(L, HOST_MODULE_NAME, Host_methods, 0);
  luaL_newmetatable(L, HOST_MODULE_NAME);
  luaL_openlib(L, 0, Host_meta, 0);
  lua_pushliteral(L, "__index");
  lua_pushvalue(L, -3);
  lua_rawset(L, -3);
  lua_pushliteral(L, "__metatable");
  lua_pushvalue(L, -3);
  lua_rawset(L, -3);
  lua_pop(L, 1);

  /* register the links methods to lua */
  luaL_openlib(L, AS_MODULE_NAME, AS_methods, 0);
  luaL_newmetatable(L, AS_MODULE_NAME);
  lua_pop(L, 1);

  /* register the Tracing functions to lua */
  luaL_openlib(L, TRACE_MODULE_NAME, Trace_methods, 0);
  luaL_newmetatable(L, TRACE_MODULE_NAME);
  lua_pop(L, 1);
}
