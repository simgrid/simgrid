/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* SimGrid Lua state management                                             */

#include "lua_state_cloner.h"
#include "lua_utils.h"
#include "xbt.h"
#include "xbt/log.h"
#include <lauxlib.h>
#include <lualib.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(lua_state_cloner, bindings, "Lua state management");

static void sglua_add_maestro_table(lua_State* L, int index, void* maestro_table_ptr);
static void sglua_remove_maestro_table(lua_State* L, int index, void* maestro_table_ptr);
static void* sglua_get_maestro_table_ptr(lua_State* L, int index);
static void sglua_get_table_by_ptr(lua_State* L, void* table_ptr);
static int l_get_from_maestro(lua_State* L);

static void sglua_copy_nil(lua_State* src, lua_State* dst);
static void sglua_copy_number(lua_State* src, lua_State* dst);
static void sglua_copy_boolean(lua_State* src, lua_State* dst);
static void sglua_copy_string(lua_State* src, lua_State* dst);
static void sglua_copy_table(lua_State* src, lua_State* dst);
static void sglua_copy_function(lua_State* src, lua_State* dst);
static void sglua_copy_lightuserdata(lua_State* src, lua_State* dst);
static void sglua_copy_userdata(lua_State* src, lua_State* dst);
static void sglua_copy_thread(lua_State* src, lua_State* dst);

/**
 * @brief Adds a reference to a maestro table to the list of known maestro
 * tables of a state.
 *
 * TODO identify maestro's tables with my own IDs instead of pointers
 * to Lua internals
 *
 * @param L a state (can be maestro itself)
 * @param index index of the copy of the maestro table in the stack of L
 * @param maestro_table_ptr pointer to the original table in maestro's world
 */
static void sglua_add_maestro_table(lua_State* L, int index, void* maestro_table_ptr) {

  /* we will set both [ptr] -> table and [table] -> ptr */

                                  /* ... */
  lua_pushvalue(L, index);
                                  /* ... table */
  lua_pushstring(L, "simgrid.maestro_tables");
                                  /* ... table "simgrid.maestro_tables" */
  lua_rawget(L, LUA_REGISTRYINDEX);
                                  /* ... table maestrotbs */
  lua_pushvalue(L, -2);
                                  /* ... table maestrotbs table */
  lua_pushlightuserdata(L, maestro_table_ptr);
                                  /* ... table maestrotbs table tableptr */
  lua_pushvalue(L, -1);
                                  /* ... table maestrotbs table tableptr tableptr */
  lua_pushvalue(L, -3);
                                  /* ... table maestrotbs table tableptr tableptr table */
  lua_settable(L, -5);
                                  /* ... table maestrotbs table tableptr */
  lua_settable(L, -3);
                                  /* ... table maestrotbs */
  lua_pop(L, 2);
                                  /* ... */
}

/**
 * @brief Removes a reference to a maestro table to the list of known maestro
 * tables of a state.
 *
 * @param L a state (can be maestro itself)
 * @param index index of the copy of the maestro table in the stack of L
 * @param maestro_table_ptr pointer to the original table in maestro's world
 */
static void sglua_remove_maestro_table(lua_State* L, int index, void* maestro_table_ptr) {

  /* we will unset both [ptr] -> table and [table] -> ptr */

                                  /* ... */
  lua_pushvalue(L, index);
                                  /* ... table */
  lua_pushstring(L, "simgrid.maestro_tables");
                                  /* ... table "simgrid.maestro_tables" */
  lua_rawget(L, LUA_REGISTRYINDEX);
                                  /* ... table maestrotbs */
  lua_pushvalue(L, -2);
                                  /* ... table maestrotbs table */
  lua_pushnil(L);
                                  /* ... table maestrotbs table nil */
  lua_pushlightuserdata(L, maestro_table_ptr);
                                  /* ... table maestrotbs table nil tableptr */
  lua_pushnil(L);
                                  /* ... table maestrotbs table nil tableptr nil*/
  lua_settable(L, -5);
                                  /* ... table maestrotbs table nil */
  lua_settable(L, -3);
                                  /* ... table maestrotbs */
  lua_pop(L, 2);
                                  /* ... */
}

/**
 * @brief For a table in the stack of L, returns a pointer that identifies the
 * same table in in maestro's world.
 * @param L a Lua state
 * @param index index of a table in the stack of L
 * @return a pointer to maestro's copy of this table, or NULL if this table
 * did not come from maestro
 */
static void* sglua_get_maestro_table_ptr(lua_State* L, int index) {

  void* maestro_table_ptr = NULL;
                                  /* ... */
  lua_pushvalue(L, index);
                                  /* ... table */
  lua_pushstring(L, "simgrid.maestro_tables");
                                  /* ... table "simgrid.maestro_tables" */
  lua_rawget(L, LUA_REGISTRYINDEX);
                                  /* ... table maestrotbs */
  lua_pushvalue(L, -2);
                                  /* ... table maestrotbs table */
  lua_gettable(L, -2);
                                  /* ... table maestrotbs tableptr/nil */
  if (!lua_isnil(L, -1)) {
                                  /* ... table maestrotbs tableptr */
    maestro_table_ptr = (void*) lua_topointer(L, -1);
  }

  lua_pop(L, 3);
                                  /* ... */
  return maestro_table_ptr;
}

/**
 * @brief Pushes a table knowing a pointer to maestro's copy of this table.
 *
 * Pushes nil if L does not know this table in maestro.
 *
 * @param L a Lua state
 * @param maestro_table_ptr pointer that identifies a table in maestro's world
 */
static void sglua_get_table_by_ptr(lua_State* L, void* maestro_table_ptr) {

                                  /* ... */
  lua_pushstring(L, "simgrid.maestro_tables");
                                  /* ... "simgrid.maestro_tables" */
  lua_rawget(L, LUA_REGISTRYINDEX);
                                  /* ... maestrotbs */
  lua_pushlightuserdata(L, maestro_table_ptr);
                                  /* ... maestrotbs tableptr */
  lua_gettable(L, -2);
                                  /* ... maestrotbs table/nil */
  lua_remove(L, -2);
                                  /* ... table/nil */
}

/**
 * @brief Pops a value from the stack of a source state and pushes it on the
 * stack of another state.
 * If the value is a table, its content is copied recursively.
 *
 * This function is similar to lua_xmove() but it allows to move a value
 * between two different global states.
 *
 * @param src the source state (not necessarily maestro)
 * @param dst the destination state
 */
void sglua_move_value(lua_State* src, lua_State* dst) {

  sglua_copy_value(src, dst);
  lua_pop(src, 1);
}

/**
 * @brief Pushes onto the stack a copy of the value on top another stack.
 * If the value is a table, its content is copied recursively.
 *
 * This function allows to move a value between two different global states.
 *
 * @param src the source state (not necessarily maestro)
 * @param dst the destination state
 */
void sglua_copy_value(lua_State* src, lua_State* dst) {

  luaL_checkany(src, -1);                  /* check the value to copy */

  int indent = (lua_gettop(dst) - 1) * 6;
  XBT_DEBUG("%sCopying data %s", sglua_get_spaces(indent), sglua_tostring(src, -1));

  sglua_stack_dump("src before copying a value (should be ... value): ", src);
  sglua_stack_dump("dst before copying a value (should be ...): ", dst);

  switch (lua_type(src, -1)) {

    case LUA_TNIL:
      sglua_copy_nil(src, dst);
      break;

    case LUA_TNUMBER:
      sglua_copy_number(src, dst);
      break;

    case LUA_TBOOLEAN:
      sglua_copy_boolean(src, dst);
      break;

    case LUA_TSTRING:
      sglua_copy_string(src, dst);
      break;

    case LUA_TFUNCTION:
      sglua_copy_function(src, dst);
      break;

    case LUA_TTABLE:
      sglua_copy_table(src, dst);
      break;

    case LUA_TLIGHTUSERDATA:
      sglua_copy_lightuserdata(src, dst);
      break;

    case LUA_TUSERDATA:
      sglua_copy_userdata(src, dst);
      break;

    case LUA_TTHREAD:
      sglua_copy_thread(src, dst);
      break;
  }

  XBT_DEBUG("%sData copied", sglua_get_spaces(indent));

  sglua_stack_dump("src after copying a value (should be ... value): ", src);
  sglua_stack_dump("dst after copying a value (should be ... value): ", dst);
}

/**
 * @brief Copies the nil value on the top of src to the top of dst.
 * @param src source state
 * @param dst destination state
 */
static void sglua_copy_nil(lua_State* src, lua_State* dst) {
  lua_pushnil(dst);
}

/**
 * @brief Copies the number value on the top of src to the top of dst.
 * @param src source state
 * @param dst destination state
 */
static void sglua_copy_number(lua_State* src, lua_State* dst) {
  lua_pushnumber(dst, lua_tonumber(src, -1));
}

/**
 * @brief Copies the boolean value on the top of src to the top of dst.
 * @param src source state
 * @param dst destination state
 */
static void sglua_copy_boolean(lua_State* src, lua_State* dst) {
  lua_pushboolean(dst, lua_toboolean(src, -1));
}

/**
 * @brief Copies the string value on the top of src to the top of dst.
 * @param src source state
 * @param dst destination state
 */
static void sglua_copy_string(lua_State* src, lua_State* dst) {

  /* no worries about memory: lua_pushstring makes a copy */
  lua_pushstring(dst, lua_tostring(src, -1));
}

/**
 * @brief Copies the table value on top of src to the top of dst.
 *
 * A deep copy of the table is made. If the table has a metatable, the
 * metatable is also copied.
 * If the table comes from maestro and is already known by the destination
 * state, it is not copied again.
 *
 * @param src source state
 * @param dst destination state
 */
static void sglua_copy_table(lua_State* src, lua_State* dst) {

                                  /* src: ... table
                                     dst: ... */
  int indent = lua_gettop(dst) * 6  + 2;

  /* get from maestro the pointer that identifies this table */
  void* table_ptr = sglua_get_maestro_table_ptr(src, -1);
  int known_by_maestro = (table_ptr != NULL);

  if (!known_by_maestro) {
    /* the table didn't come from maestro: nevermind, use the pointer of src */
    table_ptr = (void*) lua_topointer(src, -1);
    XBT_DEBUG("%sMaestro does not know this table",
        sglua_get_spaces(indent));
  }

  if (sglua_is_maestro(src)) {
    /* register the table in maestro itself */
    XBT_DEBUG("%sKeeping track of this table in maestro itself",
        sglua_get_spaces(indent));
    sglua_add_maestro_table(src, -1, table_ptr);
    known_by_maestro = 1;
    xbt_assert(sglua_get_maestro_table_ptr(src, -1) == table_ptr);
  }

  /* to avoid infinite recursion, see if this table is already known by dst */
  sglua_get_table_by_ptr(dst, table_ptr);
                                  /* dst: ... table/nil */
  if (!lua_isnil(dst, -1)) {
    XBT_DEBUG("%sNothing to do: table already known (%p)",
        sglua_get_spaces(indent), table_ptr);
                                  /* dst: ... table */
  }
  else {
    XBT_DEBUG("%sFirst visit of this table (%p)", sglua_get_spaces(indent),
        table_ptr);
                                  /* dst: ... nil */
    lua_pop(dst, 1);
                                  /* dst: ... */

    /* first visit: create the new table in dst */
    lua_newtable(dst);
                                  /* dst: ... table */

    /* mark the table as known right now to avoid infinite recursion */
    sglua_add_maestro_table(dst, -1, table_ptr);
    /* we may have added a table with a non-maestro pointer, but if it was the
     * case, we will remove it later */
    XBT_DEBUG("%sTable marked as known", sglua_get_spaces(indent));
    xbt_assert(sglua_get_maestro_table_ptr(dst, -1) == table_ptr);

    sglua_stack_dump("dst after marking the table as known (should be ... table): ", dst);

    /* copy the metatable if any */
    int has_meta_table = lua_getmetatable(src, -1);
                                  /* src: ... table mt? */
    if (has_meta_table) {
      XBT_DEBUG("%sCopying the metatable", sglua_get_spaces(indent));
                                  /* src: ... table mt */
      sglua_copy_table(src, dst);
                                  /* dst: ... table mt */
      lua_pop(src, 1);
                                  /* src: ... table */
      lua_setmetatable(dst, -2);
                                  /* dst: ... table */
    }
    else {
                                  /* src: ... table */
      XBT_DEBUG("%sNo metatable", sglua_get_spaces(indent));
    }

    sglua_stack_dump("src before traversing the table (should be ... table): ", src);
    sglua_stack_dump("dst before traversing the table (should be ... table): ", dst);

    /* traverse the table of src and copy each element */
    lua_pushnil(src);
                                  /* src: ... table nil */
    while (lua_next(src, -2) != 0) {
                                  /* src: ... table key value */

      XBT_DEBUG("%sCopying table element %s", sglua_get_spaces(indent),
          sglua_keyvalue_tostring(src, -2, -1));

      sglua_stack_dump("src before copying table element (should be ... table key value): ", src);
      sglua_stack_dump("dst before copying table element (should be ... table): ", dst);

      /* copy the key */
      lua_pushvalue(src, -2);
                                  /* src: ... table key value key */
      indent += 2;
      XBT_DEBUG("%sCopying the key part of the table element",
          sglua_get_spaces(indent));
      sglua_move_value(src, dst);
                                  /* src: ... table key value
                                     dst: ... table key */
      XBT_DEBUG("%sCopied the key part of the table element",
          sglua_get_spaces(indent));

      /* copy the value */
      XBT_DEBUG("%sCopying the value part of the table element",
          sglua_get_spaces(indent));
      sglua_move_value(src, dst);
                                  /* src: ... table key
                                     dst: ... table key value */
      XBT_DEBUG("%sCopied the value part of the table element",
          sglua_get_spaces(indent));
      indent -= 2;

      /* set the table element */
      lua_settable(dst, -3);
                                  /* dst: ... table */

      /* the key stays on top of src for next iteration */
      sglua_stack_dump("src before next iteration (should be ... table key): ", src);
      sglua_stack_dump("dst before next iteration (should be ... table): ", dst);

      XBT_DEBUG("%sTable element copied", sglua_get_spaces(indent));
    }
    XBT_DEBUG("%sFinished traversing the table", sglua_get_spaces(indent));
  }

  if (!known_by_maestro) {
    /* actually,it was not a maestro table: forget the pointer */
    sglua_remove_maestro_table(dst, -1, table_ptr);
  }
}

/**
 * @brief Copies the function on the top of src to the top of dst.
 *
 * It can be a Lua function or a C function.
 *
 * @param src source state
 * @param dst destination state
 */
static void sglua_copy_function(lua_State* src, lua_State* dst) {

  if (lua_iscfunction(src, -1)) {
    /* it's a C function */

    XBT_DEBUG("It's a C function");
    sglua_stack_dump("src before copying upvalues: ", src);

    /* get the function pointer */
    int function_index = lua_gettop(src);
    lua_CFunction f = lua_tocfunction(src, function_index);

    /* copy the upvalues */
    int i = 0;
    const char* upvalue_name = NULL;
    do {
      i++;
      upvalue_name = lua_getupvalue(src, function_index, i);

      if (upvalue_name != NULL) {
        XBT_DEBUG("Upvalue %s", upvalue_name);
        sglua_move_value(src, dst);
      }
    } while (upvalue_name != NULL);

    sglua_stack_dump("src before copying pointer: ", src);

    /* set the function */
    lua_pushcclosure(dst, f, i - 1);
    XBT_DEBUG("Function pointer copied");
  }
  else {
    /* it's a Lua function: dump it from src */

    s_sglua_buffer_t buffer;
    buffer.capacity = 128; /* an empty function uses 77 bytes */
    buffer.size = 0;
    buffer.data = xbt_new(char, buffer.capacity);

    /* copy the binary chunk from src into a buffer */
    _XBT_GNUC_UNUSED int error = lua_dump(src, sglua_memory_writer, &buffer);
    xbt_assert(!error, "Failed to dump the function from the source state: error %d",
        error);
    XBT_DEBUG("Fonction dumped: %zu bytes", buffer.size);

    /*
    fwrite(buffer.data, buffer.size, buffer.size, stderr);
    fprintf(stderr, "\n");
    */

    /* load the chunk into dst */
    error = luaL_loadbuffer(dst, buffer.data, buffer.size, "(dumped function)");
    xbt_assert(!error, "Failed to load the function into the destination state: %s",
        lua_tostring(dst, -1));
  }
}

/**
 * @brief Copies the light userdata on the top of src to the top of dst.
 * @param src source state
 * @param dst destination state
 */
static void sglua_copy_lightuserdata(lua_State* src, lua_State* dst) {
  lua_pushlightuserdata(dst, lua_touserdata(src, -1));
}

/**
 * @brief Copies the full userdata on the top of src to the top of dst.
 *
 * If the userdata has a metatable, the metatable is also copied.
 *
 * @param src source state
 * @param dst destination state
 */
static void sglua_copy_userdata(lua_State* src, lua_State* dst) {

  int indent = lua_gettop(dst) * 6  + 2;

  /* copy the data */
                                  /* src: ... udata
                                     dst: ... */
  size_t size = lua_objlen(src, -1);
  void* src_block = lua_touserdata(src, -1);
  void* dst_block = lua_newuserdata(dst, size);
                                  /* dst: ... udata */
  memcpy(dst_block, src_block, size);

  /* copy the metatable if any */
  int has_meta_table = lua_getmetatable(src, -1);
                                  /* src: ... udata mt? */
  if (has_meta_table) {
    XBT_DEBUG("%sCopying metatable of userdata (%p)",
        sglua_get_spaces(indent), lua_topointer(src, -1));
                                  /* src: ... udata mt */
    sglua_copy_table(src, dst);
                                  /* src: ... udata mt
                                     dst: ... udata mt */
    lua_pop(src, 1);
                                  /* src: ... udata */
    lua_setmetatable(dst, -2);
                                  /* dst: ... udata */

    XBT_DEBUG("%sMetatable of userdata copied", sglua_get_spaces(indent));
  }
  else {
    XBT_DEBUG("%sNo metatable for this userdata",
        sglua_get_spaces(indent));
                                  /* src: ... udata */
  }
}

/**
 * @brief This operation is not supported (yet?) so it just pushes nil.
 *
 * @param src source state
 * @param dst destination state
 */
static void sglua_copy_thread(lua_State* src, lua_State* dst) {

  XBT_WARN("Copying a thread from another state is not implemented (yet?).");
  lua_pushnil(dst);
}

/**
 * @brief Copies a global value or a registry value from the maestro state.
 *
 * The state L must have been created by sglua_clone_maestro_state().
 * This function is meant to be an __index metamethod.
 * Consequently, it assumes that the stack has two elements:
 * a table (either the environment or the registry of L) and the string key of
 * a value that does not exist yet in this table. It copies the corresponding
 * value from maestro and pushes it on the stack of L.
 * If the value does not exist in maestro state either, nil is pushed.
 *
 * TODO: make this function thread safe. If the simulation runs in parallel,
 * several simulated processes may trigger this __index metamethod at the same
 * time and get globals from maestro.
 *
 * @param L the current state
 * @return number of return values pushed (always 1)
 */
static int l_get_from_maestro(lua_State *L) {

  /* check the arguments */
  luaL_checktype(L, 1, LUA_TTABLE);
  const char* key = luaL_checkstring(L, 2);
                                  /* L:      table key */
  XBT_DEBUG("__index of '%s' begins", key);

  /* want a global or a registry value? */
  int pseudo_index;
  if (lua_equal(L, 1, LUA_REGISTRYINDEX)) {
    /* registry */
    pseudo_index = LUA_REGISTRYINDEX;
    XBT_DEBUG("Will get the value from the registry of maestro");
  }
  else {
    /* global */
    pseudo_index = LUA_GLOBALSINDEX;
    XBT_DEBUG("Will get the value from the globals of maestro");
  }

  /* get the father */
  lua_State* maestro = sglua_get_maestro();

                                  /* L:      table key */

  /* get the value from maestro */
  lua_getfield(maestro, pseudo_index, key);
                                  /* maestro: ... value */

  /* push the value onto the stack of L */
  sglua_move_value(maestro, L);
                                  /* maestro: ...
                                     L:      table key value */

  /* prepare the return value of __index */
  lua_pushvalue(L, -1);
                                  /* L:      table key value value */
  lua_insert(L, 1);
                                  /* L:      value table key value */

  /* save the copied value in the table for subsequent accesses */
  lua_settable(L, -3);
                                  /* L:      value table */
  lua_settop(L, 1);
                                  /* L:      value */

  XBT_DEBUG("__index of '%s' returns %s", key, sglua_tostring(L, -1));

  return 1;
}

/**
 * @brief Creates a new Lua state and get its environment from the maestro
 * state.
 *
 * The state created is independent from maestro and has its own copies of
 * global and registry values.
 * However, the global and registry values are not copied right now from
 * the original state; they are copied only the first time they are accessed.
 * This behavior saves time and memory, and is okay for Simgrid's needs.
 *
 * TODO: if the simulation runs in parallel, copy everything right now?
 *
 * @return the state created
 */
lua_State* sglua_clone_maestro(void) {

  /* create the new state */
  lua_State *L = luaL_newstate();

  /* set its environment and its registry:
   * - create a table newenv
   * - create a metatable mt
   * - set mt.__index = a function that copies the global from the father state
   * - set mt as the metatable of the registry
   * - set mt as the metatable of newenv
   * - set newenv as the environment of the new state
   */
  lua_pushthread(L);                        /* thread */
  lua_newtable(L);                          /* thread newenv */
  lua_newtable(L);                          /* thread newenv mt */
  lua_pushvalue(L, LUA_REGISTRYINDEX);      /* thread newenv mt reg */
  lua_pushcfunction(L, l_get_from_maestro); /* thread newenv mt reg f */
  lua_setfield(L, -3, "__index");           /* thread newenv mt reg */
  lua_pushvalue(L, -2);                     /* thread newenv mt reg mt */
  lua_setmetatable(L, -2);                  /* thread newenv mt reg */
  lua_pop(L, 1);                            /* thread newenv mt */
  lua_setmetatable(L, -2);                  /* thread newenv */
  lua_setfenv(L, -2);                       /* thread */
  lua_pop(L, 1);                            /* -- */

  /* create the table of known tables from maestro */
  lua_pushstring(L, "simgrid.maestro_tables");
                                            /* "simgrid.maestro_tables" */
  lua_newtable(L);                          /* "simgrid.maestro_tables" maestrotbs */
  lua_rawset(L, LUA_REGISTRYINDEX);
                                            /* -- */

  /* opening the standard libs is not necessary as they are
   * inherited like any global values */
  /* luaL_openlibs(L); */

  XBT_DEBUG("New state created");

  return L;
}
