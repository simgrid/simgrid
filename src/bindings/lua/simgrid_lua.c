/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* SimGrid Lua bindings                                                     */

#include "simgrid_lua.h"
#include "lua_state_cloner.h"
#include "lua_utils.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(lua, bindings, "Lua Bindings");

static lua_State *lua_maestro_state;

#define TASK_MODULE_NAME "simgrid.Task"
#define HOST_MODULE_NAME "simgrid.Host"
// Surf (bypass XML)
#define LINK_MODULE_NAME "simgrid.Link"
#define ROUTE_MODULE_NAME "simgrid.Route"
#define AS_MODULE_NAME "simgrid.AS"
#define TRACE_MODULE_NAME "simgrid.Trace"

static void register_c_functions(lua_State *L);

static void *my_checkudata (lua_State *L, int ud, const char *tname) {

  XBT_DEBUG("Checking the task: ud = %d", ud);
  sglua_stack_dump("my_checkudata: ", L);
  void *p = lua_touserdata(L, ud);
  lua_getfield(L, LUA_REGISTRYINDEX, tname);
  const void* correct_mt = lua_topointer(L, -1);

  int has_mt = lua_getmetatable(L, ud);
  XBT_DEBUG("Checking the task: has metatable ? %d", has_mt);
  const void* actual_mt = NULL;
  if (has_mt) { actual_mt = lua_topointer(L, -1); lua_pop(L, 1); }
  XBT_DEBUG("Checking the task's metatable: expected %p, found %p", correct_mt, actual_mt);
  sglua_stack_dump("my_checkudata: ", L);

  if (p == NULL || !lua_getmetatable(L, ud) || !lua_rawequal(L, -1, -2))
    luaL_typerror(L, ud, tname);
  lua_pop(L, 2);
  return p;
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
  XBT_DEBUG("Lua task: %s", sglua_tostring(L, index));
  luaL_checktype(L, index, LUA_TTABLE);
  lua_getfield(L, index, "__simgrid_task");

  pi = (m_task_t *) luaL_checkudata(L, lua_gettop(L), TASK_MODULE_NAME);

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
  //stack_dump("send ", L);
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
    sglua_move_value(sender_stack, L);        // copy the data directly from sender's stack
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
  pi = (m_host_t *) luaL_checkudata(L, lua_gettop(L), HOST_MODULE_NAME);
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

  lua_State *L = sglua_clone_maestro();
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

  /* initialize access to my tables by children Lua states */
  lua_newtable(L);
  lua_setfield(L, LUA_REGISTRYINDEX, "simgrid.maestro_tables");

  register_c_functions(L);

  return 1;
}

/**
 * @brief Returns whether a Lua state is the maestro state.
 * @param L a Lua state
 * @return true if this is maestro
 */
int sglua_is_maestro(lua_State* L) {
  return L == lua_maestro_state;
}

/**
 * @brief Returns the maestro state.
 * @return true the maestro Lua state
 */
lua_State* sglua_get_maestro(void) {
  return lua_maestro_state;
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
