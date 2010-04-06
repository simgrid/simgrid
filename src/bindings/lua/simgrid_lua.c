/* SimGrid Lua bindings                                                     */

/* Copyright (c) 2010, the SimGrid team. All right reserved.                */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <lauxlib.h>
#include <lualib.h>

#include "msg/msg.h"
#include "xbt.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(lua,bindings,"Lua Bindings");

#define TASK_MODULE_NAME "simgrid.Task"
#define HOST_MODULE_NAME "simgrid.Host"

/* ********************************************************************************* */
/*                            helper functions                                       */
/* ********************************************************************************* */

static void stackDump (const char *msg, lua_State *L) {
  char buff[2048];
  char *p=buff;
  int i;
  int top = lua_gettop(L);
//  void *ptr;
  fflush(stdout);
  p+=sprintf(p,"STACK(top=%d): ",top);
/*  lua_getglobal(L,"vardump");
  xbt_assert0(lua_isfunction(L,-1), "Vardump not found");
*/

  for (i = 1; i <= top; i++) {  /* repeat for each level */

    int t = lua_type(L, i);
    switch (t) {

    case LUA_TSTRING:  /* strings */
      p+=sprintf(p,"`%s'", lua_tostring(L, i));
      break;

    case LUA_TBOOLEAN:  /* booleans */
      p+=sprintf(p,lua_toboolean(L, i) ? "true" : "false");
      break;

    case LUA_TNUMBER:  /* numbers */
      p+=sprintf(p,"%g", lua_tonumber(L, i));
      break;

    case LUA_TTABLE:
      p+=sprintf(p, "Table");
      break;

    default:  /* other values */
      p+=sprintf(p, "???");
/*      if ((ptr = luaL_checkudata(L,i,TASK_MODULE_NAME))) {
        p+=sprintf(p,"task");
      } else {
        p+=printf(p,"%s", lua_typename(L, t));
      }*/
      break;

    }
    p+=sprintf(p,"  ");  /* put a separator */
  }
  INFO2("%s%s",msg,buff);
}

/** @brief ensures that a userdata on the stack is a task and returns the pointer inside the userdata */
static m_task_t checkTask (lua_State *L,int index) {
  m_task_t *pi,tk;
  luaL_checktype(L,index,LUA_TTABLE);
  lua_getfield(L,index,"__simgrid_task");
  pi = (m_task_t*)luaL_checkudata(L,-1,TASK_MODULE_NAME);
  if(pi == NULL)
	 luaL_typerror(L,index,TASK_MODULE_NAME);
  tk = *pi;
  if(!tk)
	 luaL_error(L,"null Task");
  lua_pop(L,1);
  return  tk;
}

/** @brief leaves a new userdata on top of the stack, sets its metatable, and sets the Task pointer inside the userdata */
/*static m_task_t *pushTask (lua_State *L,m_task_t tk) {

  m_task_t *pi = NULL;
  pi = (m_task_t*)lua_newuserdata(L,sizeof(m_task_t));
  *pi=tk;

  DEBUG1("push lua task with Name : %s \n",MSG_task_get_name(*pi));
  luaL_getmetatable(L,TASK_MODULE_NAME);
  lua_setmetatable(L,-2);
  return pi;

}*/

/* ********************************************************************************* */
/*                           wrapper functions                                       */
/* ********************************************************************************* */

/**
 *  Task
 */

static int Task_new(lua_State* L) {
  const char *name=luaL_checkstring(L,1);
  int comp_size = luaL_checkint(L,2);
  int msg_size = luaL_checkint(L,3);
  INFO0("Creating task");
  m_task_t msg_task = MSG_task_create(name,comp_size,msg_size,NULL);
  lua_newtable (L); /* create a table, put the userdata on top of it */
  m_task_t *lua_task = (m_task_t*)lua_newuserdata(L,sizeof(m_task_t));
  *lua_task = msg_task;
  luaL_getmetatable(L,TASK_MODULE_NAME);
  lua_setmetatable(L,-2);
  lua_setfield (L, -2, "__simgrid_task"); /* put the userdata as field of the table */
  /* remove the args from the stack */
  lua_remove(L,1);
  lua_remove(L,1);
  lua_remove(L,1);
  return 1;
}

static int Task_get_name(lua_State *L) {
  m_task_t tk = checkTask(L,-1);
  lua_pushstring(L,MSG_task_get_name(tk));
  return 1;
}

static int Task_computation_duration(lua_State *L){
  m_task_t tk = checkTask(L,-1);
  lua_pushnumber(L,MSG_task_get_compute_duration (tk));
  return 1;
}

static int Task_execute(lua_State *L){
  m_task_t tk = checkTask(L,-1);
  int res = MSG_task_execute(tk);
  lua_pushnumber(L,res);
  return 1;
}

static int Task_destroy(lua_State *L) {
  m_task_t tk = checkTask(L,-1);
  int res = MSG_task_destroy(tk);
  lua_pushnumber(L,res);
  return 1;
}
static int Task_send(lua_State *L)  {
  stackDump("send ",L);
  m_task_t tk = checkTask(L,-2);
  const char *mailbox = luaL_checkstring(L,-1);
  lua_pop(L,1); // remove the string so that the task is on top of it
  MSG_task_set_data(tk,L); // Copy my stack into the task, so that the receiver can copy the lua task directly
  MSG_error_t res = MSG_task_send(tk,mailbox);
  while (MSG_task_has_data(tk)) // Don't mess up with my stack: the receiver didn't copy the data yet
    MSG_process_sleep(0); // yield

  if (res != MSG_OK) switch(res) {
    case MSG_TIMEOUT :
      ERROR0("MSG_task_send failed : Timeout");
      break;
    case MSG_TRANSFER_FAILURE :
      ERROR0("MSG_task_send failed : Transfer Failure");
      break;
    case MSG_HOST_FAILURE :
      ERROR0("MSG_task_send failed : Host Failure ");
      break;
    default :
      ERROR0("MSG_task_send failed : Unexpected error , please report this bug");
      break;
    }
  return 0;
}
static int Task_recv(lua_State *L)  {
  m_task_t tk = NULL;
  const char *mailbox = luaL_checkstring(L,-1);
  MSG_error_t res = MSG_task_receive(&tk,mailbox);

  lua_State *sender_stack = MSG_task_get_data(tk);
  lua_xmove(sender_stack,L,1); // copy the data directly from sender's stack
  MSG_task_set_data(tk,NULL);

  if(res != MSG_OK) switch(res){
	  case MSG_TIMEOUT :
		  ERROR0("MSG_task_receive failed : Timeout");
		  break;
	  case MSG_TRANSFER_FAILURE :
		  ERROR0("MSG_task_receive failed : Transfer Failure");
		  break;
	  case MSG_HOST_FAILURE :
		  ERROR0("MSG_task_receive failed : Host Failure ");
		  break;
	  default :
		  ERROR0("MSG_task_receive failed : Unexpected error , please report this bug");
		  break;
		  }

  return 1;
}

static const luaL_reg Task_methods[] = {
    {"new",   Task_new},
    {"name",  Task_get_name},
    {"computation_duration",  Task_computation_duration},
    {"execute", Task_execute},
    {"destroy", Task_destroy},
    {"send",    Task_send},
    {"recv",    Task_recv},
    {0,0}
};
static int Task_gc(lua_State *L) {
  m_task_t tk=checkTask(L,-1);
  if (tk) MSG_task_destroy(tk);
  return 0;
}

static int Task_tostring(lua_State *L) {
  lua_pushfstring(L, "Task :%p",lua_touserdata(L,1));
  return 1;
}

static const luaL_reg Task_meta[] = {
    {"__gc",  Task_gc},
    {"__tostring",  Task_tostring},
    {0,0}
};

/**
 * Host
 */
static m_host_t checkHost (lua_State *L,int index) {
  m_host_t *pi,ht;
  luaL_checktype(L,index,LUA_TTABLE);
  lua_getfield(L,index,"__simgrid_host");
  pi = (m_host_t*)luaL_checkudata(L,-1,HOST_MODULE_NAME);
  if(pi == NULL)
	 luaL_typerror(L,index,HOST_MODULE_NAME);
  ht = *pi;
  if(!ht)
	 luaL_error(L,"null Host");
  lua_pop(L,1);
  return  ht;
}


static int Host_get_by_name(lua_State *L)
{
	const char *name=luaL_checkstring(L,1);
	DEBUG0("Getting Host from name...");
	m_host_t msg_host = MSG_get_host_by_name(name);
	if (!msg_host)
		{
		//ERROR1("MSG_get_host_by_name(%s) failled",name);
		luaL_error(L,"null Host : MSG_get_host_by_name failled");
		//return -1;
		}
    lua_newtable (L); /* create a table, put the userdata on top of it */
	m_host_t *lua_host = (m_host_t*)lua_newuserdata(L,sizeof(m_host_t));
	*lua_host = msg_host;
	luaL_getmetatable(L,HOST_MODULE_NAME);
	lua_setmetatable(L,-2);
	lua_setfield (L, -2, "__simgrid_host"); /* put the userdata as field of the table */
	/* remove the args from the stack */
	lua_remove(L,1);
	return 1;
}


static int Host_get_name(lua_State *L) {
  m_host_t ht = checkHost(L,-1);
  lua_pushstring(L,MSG_host_get_name(ht));
  return 1;
}

static int Host_number(lua_State *L) {
  lua_pushnumber(L,MSG_get_host_number());
  return 1;
}

static int Host_table(lua_State *L)
{
	// FIXME
	int nb = MSG_get_host_number();
    m_host_t *msg_hosts = MSG_get_host_table();
    m_host_t *lua_hosts = (m_host_t*)lua_newuserdata(L,sizeof(m_host_t)*nb);
    *lua_hosts = msg_hosts;

    lua_newtable (L);
    // TODO : fill the table <ith hosts
    return 1;
}

static const luaL_reg Host_methods[] = {
    {"getByName",   Host_get_by_name},
    {"name",  		Host_get_name},
    {"number",    	Host_number},
    {"list",    	Host_table},
    {0,0}
};

static int Host_gc(lua_State *L)
{
  m_host_t ht = checkHost(L,-1);
  if (ht) ht = NULL;
  return 0;
}

static int Host_tostring(lua_State *L)
{
  lua_pushfstring(L,"Host :%p",lua_touserdata(L,1));
  return 1;
}

static const luaL_reg Host_meta[] = {
    {"__gc",  Host_gc},
    {"__tostring",  Host_tostring},
    {0,0}
};

/*
 * Environment related
 */
extern lua_State *simgrid_lua_state;

static int run_lua_code(int argc,char **argv) {
  DEBUG1("Run lua code %s",argv[0]);
//  fprintf(stderr,"Run lua code %s\n", (argv ? argv[0] : "(null)"));
  lua_State *L = lua_newthread(simgrid_lua_state);
  int ref = luaL_ref(simgrid_lua_state, LUA_REGISTRYINDEX); // protect the thread from being garbage collected
  int res = 1;

  /* Start the co-routine */
  lua_getglobal(L,argv[0]);
  xbt_assert1(lua_isfunction(L,-1),
      "The lua function %s does not seem to exist",argv[0]);

  // push arguments onto the stack
  int i;
  for(i=1;i<argc;i++)
    lua_pushstring(L,argv[i]);

  // Call the function (in resume)
  xbt_assert2(lua_pcall(L, argc-1, 1, 0) == 0,
    "error running function `%s': %s",argv[0], lua_tostring(L, -1));

  /* retrieve result */
  if (lua_isnumber(L, -1)) {
    res = lua_tonumber(L, -1);
    lua_pop(L, 1);  /* pop returned value */
  }

  // cleanups
  luaL_unref(simgrid_lua_state,LUA_REGISTRYINDEX,ref );
  DEBUG1("Execution of lua code %s is over", (argv ? argv[0] : "(null)"));
  return res;
}
static int launch_application(lua_State *L) {
  const char * file = luaL_checkstring(L,1);
  MSG_function_register_default(run_lua_code);
  MSG_launch_application(file);
  return 0;
}
#include "simix/simix.h" //FIXME: KILLME when debugging on simix internals become useless
static int create_environment(lua_State *L) {
  const char *file = luaL_checkstring(L,1);
  DEBUG1("Loading environment file %s",file);
  MSG_create_environment(file);
  smx_host_t *hosts = SIMIX_host_get_table();
  int i;
  for (i=0;i<SIMIX_host_get_number();i++) {
    DEBUG1("We have an host %s", SIMIX_host_get_name(hosts[i]));
  }

  return 0;
}
static int debug(lua_State *L) {
  const char *str = luaL_checkstring(L,1);
  DEBUG1("%s",str);
  return 0;
}
static int info(lua_State *L) {
  const char *str = luaL_checkstring(L,1);
  INFO1("%s",str);
  return 0;
}
static int run(lua_State *L) {
  MSG_main();
  return 0;
}
static int clean(lua_State *L) {
  MSG_clean();
  return 0;
}
static const luaL_Reg simgrid_funcs[] = {
    { "create_environment", create_environment},
    { "launch_application", launch_application},
    { "debug", debug},
    { "info", info},
    { "run", run},
    { "clean", clean},
    /* short names */
    { "platform", create_environment},
    { "application", launch_application},
    { NULL, NULL }
};

/* ********************************************************************************* */
/*                       module management functions                                 */
/* ********************************************************************************* */

extern const char*xbt_ctx_factory_to_use; /*Hack: let msg load directly the right factory */

#define LUA_MAX_ARGS_COUNT 10 /* maximum amount of arguments we can get from lua on command line */

int luaopen_simgrid(lua_State* L); // Fuck gcc: we don't need that prototype
int luaopen_simgrid(lua_State* L) {
  //xbt_ctx_factory_to_use = "lua";

  char **argv=malloc(sizeof(char*)*LUA_MAX_ARGS_COUNT);
  int argc=1;
  argv[0] = (char*)"/usr/bin/lua"; /* Lie on the argv[0] so that the stack dumping facilities find the right binary. FIXME: what if lua is not in that location? */
  /* Get the command line arguments from the lua interpreter */
  lua_getglobal(L,"arg");
  xbt_assert1(lua_istable(L,-1),"arg parameter is not a table but a %s",lua_typename(L,-1));
  int done=0;
  while (!done) {
    argc++;
    lua_pushinteger(L,argc-2);
    lua_gettable(L,-2);
    if (lua_isnil(L,-1)) {
      done = 1;
    } else {
      xbt_assert1(lua_isstring(L,-1),"argv[%d] got from lua is no string",argc-1);
      xbt_assert2(argc<LUA_MAX_ARGS_COUNT,
           "Too many arguments, please increase LUA_MAX_ARGS_COUNT in %s before recompiling SimGrid if you insist on having more than %d args on command line",
           __FILE__,LUA_MAX_ARGS_COUNT-1);
      argv[argc-1] = (char*)luaL_checkstring(L,-1);
      lua_pop(L,1);
      DEBUG1("Got command line argument %s from lua",argv[argc-1]);
    }
  }
  argv[argc--]=NULL;

  /* Initialize the MSG core */
  MSG_global_init(&argc,argv);
  DEBUG1("Still %d arguments on command line",argc); // FIXME: update the lua's arg table to reflect the changes from SimGrid

  /* register the core C functions to lua */
  luaL_register(L, "simgrid", simgrid_funcs);
  /* register the task methods to lua */
  luaL_openlib(L,TASK_MODULE_NAME,Task_methods,0); //create methods table,add it to the globals
  luaL_newmetatable(L,TASK_MODULE_NAME); //create metatable for Task,add it to the Lua registry
  luaL_openlib(L,0,Task_meta,0);// fill metatable
  lua_pushliteral(L,"__index");
  lua_pushvalue(L,-3);  //dup methods table
  lua_rawset(L,-3); //matatable.__index = methods
  lua_pushliteral(L,"__metatable");
  lua_pushvalue(L,-3);  //dup methods table
  lua_rawset(L,-3); //hide metatable:metatable.__metatable = methods
  lua_pop(L,1);   //drop metatable

  /* register the hosts methods to lua*/
  luaL_openlib(L,HOST_MODULE_NAME,Host_methods,0);
  luaL_newmetatable(L,HOST_MODULE_NAME);
  luaL_openlib(L,0,Host_meta,0);
  lua_pushliteral(L,"__index");
  lua_pushvalue(L,-3);
  lua_rawset(L,-3);
  lua_pushliteral(L,"__metatable");
  lua_pushvalue(L,-3);
  lua_rawset(L,-3);
  lua_pop(L,1);

  /* Keep the context mechanism informed of our lua world today */
  simgrid_lua_state = L;

  return 1;
}
