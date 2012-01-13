/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* SimGrid Lua bindings                                                     */

#include "lua_private.h"
#include "lua_state_cloner.h"
#include "lua_utils.h"
#include "xbt.h"
#include "msg/msg.h"
#include "simdag/simdag.h"
#include "surf/surfxml_parse.h"
#include "gras.h"
#include <lauxlib.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(lua, bindings, "Lua Bindings");

static lua_State* sglua_maestro_state;

int luaopen_simgrid(lua_State *L);
static void sglua_register_c_functions(lua_State *L);
static int run_lua_code(int argc, char **argv);

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
  if (sglua_is_maestro(L)) {
    MSG_clean();
  }
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
    MSG_process_set_data_cleanup((void_f_pvoid_t) lua_close);
    XBT_DEBUG("Still %d arguments on command line", argc); // FIXME: update the lua's arg table to reflect the changes from SimGrid
  }

  /* Keep the context mechanism informed of our lua world today */
  sglua_maestro_state = L;

  /* initialize access to my tables by children Lua states */
  lua_newtable(L);
  lua_setfield(L, LUA_REGISTRYINDEX, "simgrid.maestro_tables");

  sglua_register_c_functions(L);

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
 * \brief Makes the core functions available to the Lua world.
 * \param L a Lua world
 */
static void sglua_register_core_functions(lua_State *L)
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
static void sglua_register_c_functions(lua_State *L)
{
  sglua_register_core_functions(L);
  sglua_register_task_functions(L);
  sglua_register_comm_functions(L);
  sglua_register_host_functions(L);
  sglua_register_process_functions(L);
  sglua_register_platf_functions(L);
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

/**
 * \brief Returns a string corresponding to an MSG error code.
 * \param err an MSG error code
 * \return a string describing this error
 */
const char* sglua_get_msg_error(MSG_error_t err) {

  static const char* msg_errors[] = {
      NULL,
      "timeout",
      "transfer failure",
      "host failure",
      "task canceled"
  };

  return msg_errors[err];
}
