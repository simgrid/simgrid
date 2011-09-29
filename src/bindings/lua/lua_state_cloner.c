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

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(lua_state_cloner, lua, "Lua state management");

static lua_State* sglua_get_father(lua_State* L);
static int l_get_from_father(lua_State* L);

static void sglua_move_value_impl(lua_State* src, lua_State* dst, const char* name);
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
 * @brief Returns the father of a state, i.e. the state that created it.
 * @param L a Lua state
 * @return its father, or NULL if the state was not created by sglua_clone_state()
 */
static lua_State* sglua_get_father(lua_State* L) {

                                  /* ... */
  lua_pushstring(L, "simgrid.father");
                                  /* ... "simgrid.father" */
  lua_rawget(L, LUA_REGISTRYINDEX);
                                  /* ... father */
  lua_State* father = lua_touserdata(L, -1);
  lua_pop(L, 1);
                                  /* ... */
  return father;
}

/**
 * @brief Pops a value from a state and pushes it onto the stack of another
 * state.
 *
 * @param src the source state
 * @param dst the destination state
 */
void sglua_move_value(lua_State* src, lua_State* dst) {

  if (src != dst) {

    /* get the list of visited tables from father at index 1 of dst */
                                  /* src: ... value
                                     dst: ... */
    lua_getfield(dst, LUA_REGISTRYINDEX, "simgrid.father_visited_tables");
                                  /* dst: ... visited */
    lua_insert(dst, 1);
                                  /* dst: visited ... */

    sglua_move_value_impl(src, dst, sglua_tostring(src, -1));
                                  /* src: ...
                                     dst: visited ... value */
    lua_remove(dst, 1);
                                  /* dst: ... value */
    sglua_stack_dump("src after xmove: ", src);
    sglua_stack_dump("dst after xmove: ", dst);
  }
}

/**
 * @brief Pops a value from the stack of a source state and pushes it on the
 * stack of another state.
 *
 * If the value is a table, its content is copied recursively. To avoid cycles,
 * a table of previously visited tables must be present at index 1 of dst.
 * Its keys are pointers to visited tables in src and its values are the tables
 * already built.
 *
 * TODO: add support of closures
 *
 * @param src the source state
 * @param dst the destination state, with a list of visited tables at index 1
 * @param name a name describing the value (for debugging purposes)
 */
static void sglua_move_value_impl(lua_State* src, lua_State *dst, const char* name) {

  luaL_checkany(src, -1);                  /* check the value to copy */
  luaL_checktype(dst, 1, LUA_TTABLE);      /* check the presence of a table of
                                              previously visited tables */

  int indent = (lua_gettop(dst) - 1) * 6;
  XBT_DEBUG("%sCopying data %s", sglua_get_spaces(indent), name);

  sglua_stack_dump("src before copying a value (should be ... value): ", src);
  sglua_stack_dump("dst before copying a value (should be visited ...): ", dst);

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

  /* the value has been copied to dst: remove it from src */
  lua_pop(src, 1);

  indent -= 2;
  XBT_DEBUG("%sData copied", sglua_get_spaces(indent));

  sglua_stack_dump("src after copying a value (should be ...): ", src);
  sglua_stack_dump("dst after copying a value (should be visited ... value): ", dst);
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

static void sglua_copy_string(lua_State* src, lua_State* dst) {

  /* no worries about memory: lua_pushstring makes a copy */
  lua_pushstring(dst, lua_tostring(src, -1));
}

/**
 * @brief Copies the table value on the top of src to the top of dst.
 *
 * A deep copy of the table is made. If the table has a metatable, the
 * metatable is also copied.
 * If the table is already known by the destination state, nothing is
 * done.
 *
 * @param src source state
 * @param dst destination state
 */
static void sglua_copy_table(lua_State* src, lua_State* dst) {

  int indent = (lua_gettop(dst) - 1) * 6  + 2;

  /* first register the table in the source state itself */
  lua_getfield(src, LUA_REGISTRYINDEX, "simgrid.visited_tables");
                              /* src: ... table visited */
  lua_pushvalue(src, -2);
                              /* src: ... table visited table */
  lua_pushlightuserdata(src, (void*) lua_topointer(src, -1));
                              /* src: ... table visited table psrctable */
  lua_pushvalue(src, -1);
                              /* src: ... table visited table psrctable psrctable */
  lua_pushvalue(src, -3);
                              /* src: ... table visited table psrctable psrctable table */
  lua_settable(src, -5);
                              /* src: ... table visited table psrctable */
  lua_settable(src, -3);
                              /* src: ... table visited */
  lua_pop(src, 1);
                              /* src: ... table */

  /* see if this table was already known by dst */
  lua_pushlightuserdata(dst, (void*) lua_topointer(src, -1));
                              /* dst: visited ... psrctable */
  lua_gettable(dst, 1);
                              /* dst: visited ... table/nil */
  if (lua_istable(dst, -1)) {
    XBT_DEBUG("%sNothing to do: table already visited (%p)",
        sglua_get_spaces(indent), lua_topointer(src, -1));
                              /* dst: visited ... table */
  }
  else {
    XBT_DEBUG("%sFirst visit of this table (%p)", sglua_get_spaces(indent),
        lua_topointer(src, -1));
                              /* dst: visited ... nil */
    lua_pop(dst, 1);
                              /* dst: visited ... */

    /* first visit: create the new table in dst */
    lua_newtable(dst);
                              /* dst: visited ... table */

    /* mark the table as visited right now to avoid infinite recursion */
    lua_pushlightuserdata(dst, (void*) lua_topointer(src, -1));
                              /* dst: visited ... table psrctable */
    lua_pushvalue(dst, -2);
                              /* dst: visited ... table psrctable table */
    lua_pushvalue(dst, -1);
                              /* dst: visited ... table psrctable table table */
    lua_pushvalue(dst, -3);
                              /* dst: visited ... table psrctable table table psrctable */
    lua_settable(dst, 1);
                              /* dst: visited ... table psrctable table */
    lua_settable(dst, 1);
                              /* dst: visited ... table */
    XBT_DEBUG("%sTable marked as visited", sglua_get_spaces(indent));

    sglua_stack_dump("dst after marking the table as visited (should be visited ... table): ", dst);

    /* copy the metatable if any */
    int has_meta_table = lua_getmetatable(src, -1);
                              /* src: ... table mt? */
    if (has_meta_table) {
      XBT_DEBUG("%sCopying metatable", sglua_get_spaces(indent));
                              /* src: ... table mt */
      sglua_copy_table(src, dst);
                              /* dst: visited ... table mt */
      lua_pop(src, 1);
                              /* src: ... table */
      lua_setmetatable(dst, -2);
                              /* dst: visited ... table */
    }
    else {
      XBT_DEBUG("%sNo metatable", sglua_get_spaces(indent));
    }

    sglua_stack_dump("src before traversing the table (should be ... table): ", src);
    sglua_stack_dump("dst before traversing the table (should be visited ... table): ", dst);

    /* traverse the table of src and copy each element */
    lua_pushnil(src);
                              /* src: ... table nil */
    while (lua_next(src, -2) != 0) {
                              /* src: ... table key value */

      XBT_DEBUG("%sCopying table element %s", sglua_get_spaces(indent),
          sglua_keyvalue_tostring(src, -2, -1));

      sglua_stack_dump("src before copying table element (should be ... table key value): ", src);
      sglua_stack_dump("dst before copying table element (should be visited ... table): ", dst);

      /* copy the key */
      lua_pushvalue(src, -2);
                              /* src: ... table key value key */
      indent += 2;
      XBT_DEBUG("%sCopying the key part of the table element",
          sglua_get_spaces(indent));
      sglua_move_value_impl(src, dst, sglua_tostring(src, -1));
                              /* src: ... table key value
                                 dst: visited ... table key */
      XBT_DEBUG("%sCopied the key part of the table element",
          sglua_get_spaces(indent));

      /* copy the value */
      XBT_DEBUG("%sCopying the value part of the table element",
          sglua_get_spaces(indent));
      sglua_move_value_impl(src, dst, sglua_tostring(src, -1));
                              /* src: ... table key
                                 dst: visited ... table key value */
      XBT_DEBUG("%sCopied the value part of the table element",
          sglua_get_spaces(indent));
      indent -= 2;

      /* set the table element */
      lua_settable(dst, -3);
                              /* dst: visited ... table */

      /* the key stays on top of src for next iteration */
      sglua_stack_dump("src before next iteration (should be ... table key): ", src);
      sglua_stack_dump("dst before next iteration (should be visited ... table): ", dst);

      XBT_DEBUG("%sTable element copied", sglua_get_spaces(indent));
    }
    XBT_DEBUG("%sFinished traversing the table", sglua_get_spaces(indent));
  }
}

/**
 * @brief Copies the function on the top of src to the top of dst.
 *
 * It can be a Lua function or a C function.
 * Copying upvalues is not implemented yet (TODO).
 *
 * @param src source state
 * @param dst destination state
 */
static void sglua_copy_function(lua_State* src, lua_State* dst) {

  if (lua_iscfunction(src, -1)) {
    /* it's a C function: just copy the pointer */
    lua_CFunction f = lua_tocfunction(src, -1);
    lua_pushcfunction(dst, f);
  }
  else {
    /* it's a Lua function: dump it from src */

    s_sglua_buffer_t buffer;
    buffer.capacity = 64;
    buffer.size = 0;
    buffer.data = xbt_new(char, buffer.capacity);

    /* copy the binary chunk from src into a buffer */
    int error = lua_dump(src, sglua_memory_writer, &buffer);
    xbt_assert(!error, "Failed to dump the function from the source state: error %d",
        error);

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

  int indent = (lua_gettop(dst) - 1) * 6  + 2;

  /* copy the data */
                                  /* src: ... udata
                                     dst: visited ... */
  size_t size = lua_objlen(src, -1);
  void* src_block = lua_touserdata(src, -1);
  void* dst_block = lua_newuserdata(dst, size);
                                  /* dst: visited ... udata */
  memcpy(dst_block, src_block, size);

  /* copy the metatable if any */
  int has_meta_table = lua_getmetatable(src, -1);
                                  /* src: ... udata mt? */
  if (has_meta_table) {
    XBT_DEBUG("%sCopying metatable of userdata (%p)",
        sglua_get_spaces(indent), lua_topointer(src, -1));
                                  /* src: ... udata mt */
    lua_State* father = sglua_get_father(dst);

    if (father != NULL && src != father && sglua_get_father(src) == father) {
      XBT_DEBUG("%sGet the metatable from my father",
          sglua_get_spaces(indent));
      /* I don't want the metatable of src, I want the father's copy of the
         same metatable */

      /* get from src the pointer to the father's copy of this metatable */
      lua_pushstring(src, "simgrid.father_visited_tables");
                                  /* src: ... udata mt "simgrid.visited_tables" */
      lua_rawget(src, LUA_REGISTRYINDEX);
                                  /* src: ... udata mt visited */
      lua_pushvalue(src, -2);
                                  /* src: ... udata mt visited mt */
      lua_gettable(src, -2);
                                  /* src: ... udata mt visited pfathermt */

      /* copy the metatable from the father world into dst */
      lua_pushstring(father, "simgrid.visited_tables");
                                  /* father: ... "simgrid.visited_tables" */
      lua_rawget(father, LUA_REGISTRYINDEX);
                                  /* father: ... visited */
      lua_pushlightuserdata(father, (void*) lua_topointer(src, -1));
                                  /* father: ... visited pfathermt */
      lua_gettable(father, -2);
                                  /* father: ... visited mt */
      sglua_move_value_impl(father, dst, "(father metatable)");
                                  /* father: ... visited
                                     dst: visited ... udata mt */
      lua_pop(father, 1);
                                  /* father: ... */
      lua_pop(src, 3);
                                  /* src: ... udata */

      /* TODO make helper functions for this kind of operations */
    }
    else {
      XBT_DEBUG("%sI have no father", sglua_get_spaces(indent));
      sglua_move_value_impl(src, dst, "metatable");
                                  /* src: ... udata
                                     dst: visited ... udata mt */
    }
    lua_setmetatable(dst, -2);
                                  /* dst: visited ... udata */

    XBT_DEBUG("%sMetatable of userdata copied", sglua_get_spaces(indent));
  }
  else {
    XBT_DEBUG("%sNo metatable for this userdata",
        sglua_get_spaces(indent));
                                  /* src: ... udata */
  }
}

/**
 * @brief This operation is not supported (yet?) so this function pushes nil.
 *
 * @param src source state
 * @param dst destination state
 */
static void sglua_copy_thread(lua_State* src, lua_State* dst) {

  XBT_WARN("Cannot copy a thread from the source state.");
  lua_pushnil(dst);
}

/**
 * @brief Copies a global value or a registry value from the father state.
 *
 * The state L must have a father, i.e. it should have been created by
 * clone_lua_state().
 * This function is meant to be an __index metamethod.
 * Consequently, it assumes that the stack has two elements:
 * a table (either the environment or the registry of L) and the string key of
 * a value that does not exist yet in this table. It copies the corresponding
 * value from the father state and pushes it on the stack of L.
 * If the value does not exist in the father state either, nil is pushed.
 *
 * TODO: make this function thread safe. If the simulation runs in parallel,
 * several simulated processes may trigger this __index metamethod at the same
 * time and get globals from maestro.
 *
 * @param L the current state
 * @return number of return values pushed (always 1)
 */
static int l_get_from_father(lua_State *L) {

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
    XBT_DEBUG("Will get the value from the registry of the father");
  }
  else {
    /* global */
    pseudo_index = LUA_GLOBALSINDEX;
    XBT_DEBUG("Will get the value from the globals of the father");
  }

  /* get the father */
  lua_State* father = sglua_get_father(L);

  if (father == NULL) {
    XBT_WARN("This state has no father");
    lua_pop(L, 3);
    lua_pushnil(L);
    return 1;
  }
                                  /* L:      table key */

  /* get the list of visited tables */
  lua_pushstring(L, "simgrid.father_visited_tables");
                                  /* L:      table key "simgrid.father_visited_tables" */
  lua_rawget(L, LUA_REGISTRYINDEX);
                                  /* L:      table key visited */
  lua_insert(L, 1);
                                  /* L:      visited table key */

  /* get the value from the father */
  lua_getfield(father, pseudo_index, key);
                                  /* father: ... value */

  /* push the value onto the stack of L */
  sglua_move_value_impl(father, L, key);
                                  /* father: ...
                                     L:      visited table key value */
  lua_remove(L, 1);
                                  /* L:      table key value */

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
 * @brief Creates a new Lua state and get its environment from an existing state.
 *
 * The state created is independent from the existing one and has its own
 * copies of global and registry values.
 * However, the global and registry values are not copied right now from
 * the original state; they are copied only the first time they are accessed.
 * This behavior saves time and memory, and is okay for Simgrid's needs.
 *
 * TODO: if the simulation runs in parallel, copy everything right now?
 *
 * @param father an existing state
 * @return the state created
 */
lua_State* sglua_clone_state(lua_State *father) {

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
  lua_pushcfunction(L, l_get_from_father);  /* thread newenv mt reg f */
  lua_setfield(L, -3, "__index");           /* thread newenv mt reg */
  lua_pushvalue(L, -2);                     /* thread newenv mt reg mt */
  lua_setmetatable(L, -2);                  /* thread newenv mt reg */
  lua_pop(L, 1);                            /* thread newenv mt */
  lua_setmetatable(L, -2);                  /* thread newenv */
  lua_setfenv(L, -2);                       /* thread */
  lua_pop(L, 1);                            /* -- */

  /* set a pointer to the father */
  lua_pushstring(L, "simgrid.father");      /* "simgrid.father" */
  lua_pushlightuserdata(L, father);         /* "simgrid.father" father */
  lua_rawset(L, LUA_REGISTRYINDEX);
                                            /* -- */

  /* create the table of visited tables from the father */
  lua_pushstring(L, "simgrid.father_visited_tables");
                                            /* "simgrid.father_visited_tables" */
  lua_newtable(L);                          /* "simgrid.father_visited_tables" visited */
  lua_rawset(L, LUA_REGISTRYINDEX);
                                            /* -- */

  /* create the table of my own visited tables */
  lua_pushstring(L, "simgrid.visited_tables");
                                            /* "simgrid.visited_tables" */
  lua_newtable(L);                          /* "simgrid.visited_tables" visited */
  lua_rawset(L, LUA_REGISTRYINDEX);
                                            /* -- */

  /* open the standard libs (theoretically, this is not necessary as they can
   * be inherited like any global values, but without a proper support of
   * closures, iterators like ipairs don't work). */
  XBT_DEBUG("Metatable of globals and registry set, opening standard libraries now");
  luaL_openlibs(L);

  XBT_DEBUG("New state created");

  return L;
}
