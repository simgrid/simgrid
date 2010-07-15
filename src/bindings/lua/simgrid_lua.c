/* SimGrid Lua bindings                                                     */

/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

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
// Surf ( bypass XML )
#define LINK_MODULE_NAME "simgrid.Link"
#define ROUTE_MODULE_NAME "simgrid.Route"
#undef BYPASS_MODEL

/* ********************************************************************************* */
/*                            helper functions                                       */
/* ********************************************************************************* */

static void stackDump (const char *msg, lua_State *L) {
  char buff[2048];
  char *p=buff;
  int i;
  int top = lua_gettop(L);

  fflush(stdout);
  p+=sprintf(p,"STACK(top=%d): ",top);

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
 * Construct an new task with the specified processing amount and amount
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
  //stackDump("send ",L);
  m_task_t tk = checkTask(L,-2);
  const char *mailbox = luaL_checkstring(L,-1);
  lua_pop(L,1); // remove the string so that the task is on top of it
  MSG_task_set_data(tk,L); // Copy my stack into the task, so that the receiver can copy the lua task directly
  MSG_error_t res = MSG_task_send(tk,mailbox);
  while (MSG_task_get_data(tk)!=NULL) // Don't mess up with my stack: the receiver didn't copy the data yet
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
		luaL_error(L,"null Host : MSG_get_host_by_name failled");
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

static int Host_at(lua_State *L)
{
	int index = luaL_checkinteger(L,1);
	m_host_t host = MSG_get_host_table()[index-1]; // lua indexing start by 1 (lua[1] <=> C[0])
	lua_newtable (L); /* create a table, put the userdata on top of it */
	m_host_t *lua_host = (m_host_t*)lua_newuserdata(L,sizeof(m_host_t));
	*lua_host = host;
	luaL_getmetatable(L,HOST_MODULE_NAME);
	lua_setmetatable(L,-2);
	lua_setfield (L, -2, "__simgrid_host"); /* put the userdata as field of the table */
	return 1;

}

/*****************************************************************************************
							     * BYPASS XML SURF Methods *
								 ***************************
								 ***************************
******************************************************************************************/
#include "surf/surfxml_parse.h" /* to override surf_parse and bypass the parser */
#include "surf/surf_private.h"
typedef struct t_host_attr
{
	//platform attribute
	const char* id;
	double power;
	//deployment attribute
	const char* function;
	xbt_dynar_t args_list;
}host_attr,*p_host_attr;

typedef struct t_link_attr
{
	const char* id;
	double bandwidth;
	double latency;
}link_attr,*p_link_attr;

typedef struct t_route_attr
{
	const char *src_id;
	const char *dest_id;
	xbt_dynar_t links_id;

}route_attr,*p_route_attr;

//using xbt_dynar_t :
static xbt_dynar_t host_list_d ;
static xbt_dynar_t link_list_d ;
static xbt_dynar_t route_list_d ;


//create resource

/*FIXME : still have to use a dictionary as argument make it possible to
consider some arguments as optional and removes the importance of the
parameter order */

static void create_host(const char* id,double power)
{
	double power_peak = 0.0;
	double power_scale = 0.0;
	tmgr_trace_t power_trace = NULL;
	//FIXME : hard coded value
	e_surf_resource_state_t state_initial = SURF_RESOURCE_ON;
	tmgr_trace_t state_trace = NULL;
	power_peak = power;
	//FIXME : hard coded value !!!
	surf_parse_get_double(&power_scale, "1.0");
	power_trace = tmgr_trace_new("");

	//state_trace = tmgr_trace_new(A_surfxml_host_state_file);
	current_property_set = xbt_dict_new();
	surf_host_create_resource(xbt_strdup(id), power_peak, power_scale,
					       power_trace, state_initial, state_trace, current_property_set);

}

static int Host_new(lua_State *L) //(id,power)
{
	// if it's the first time ,instanciate the dynar
	if(xbt_dynar_is_empty(host_list_d))
		host_list_d = xbt_dynar_new(sizeof(p_host_attr), &xbt_free_ref);

	p_host_attr host = malloc(sizeof(host_attr));
	host->id = luaL_checkstring(L,1);
	host->power = luaL_checknumber(L,2);
	host->function = NULL;
	xbt_dynar_push(host_list_d, &host);
	return 0;
}

static int Link_new(lua_State *L) // (id,bandwidth,latency)
{
	if(xbt_dynar_is_empty(link_list_d))
		link_list_d = xbt_dynar_new(sizeof(p_link_attr), &xbt_free_ref);

	p_link_attr link = malloc(sizeof(link_attr));
	link->id = luaL_checkstring(L,1);
	link->bandwidth = luaL_checknumber(L,2);
	link->latency = luaL_checknumber(L,3);
	xbt_dynar_push(link_list_d,&link);
	return 0;
}

static int Route_new(lua_State *L) // (src_id,dest_id,links_number,link_table)
{
	if(xbt_dynar_is_empty(route_list_d))
		route_list_d = xbt_dynar_new(sizeof(p_route_attr), &xbt_free_ref);
    int i;
	const char * link_id;
	p_route_attr route = malloc(sizeof(route_attr));
	route->src_id = luaL_checkstring(L,1);
	route->dest_id = luaL_checkstring(L,2);
	route->links_id = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
	lua_pushnil(L);
	while (lua_next(L,3) != 0) {
		link_id = lua_tostring(L, -1);
		xbt_dynar_push(route->links_id, &link_id);
	    DEBUG2("index = %f , Link_id = %s \n",lua_tonumber(L, -2),lua_tostring(L, -1));
	    lua_pop(L, 1);
	}
	lua_pop(L, 1);

	//add route to platform's route list
	xbt_dynar_push(route_list_d,&route);
	return 0;
}

static int Host_set_function(lua_State *L) //(host,function,nb_args,list_args)
{
	// look for the index of host in host_list
	const char *host_id = luaL_checkstring(L,1);
	const char* argument;
	unsigned int i;
	p_host_attr p_host;

	xbt_dynar_foreach(host_list_d,i,p_host)
	{
		if(p_host->id == host_id)
		{
			p_host->function = luaL_checkstring(L,2);
			p_host->args_list = xbt_dynar_new(sizeof(char *), &xbt_free_ref);
			// fill the args list
			lua_pushnil(L);
			int j = 0;
			while (lua_next(L,3) != 0) {
					argument = lua_tostring(L, -1);
					xbt_dynar_push(p_host->args_list, &argument);
				    DEBUG2("index = %f , Arg_id = %s \n",lua_tonumber(L, -2),lua_tostring(L, -1));
				    j++;
				    lua_pop(L, 1);
				}
			lua_pop(L, 1);
			return 0;
		}
	}
	ERROR1("Host : %s Not Fount !!",host_id);
	return 1;
}

/*
 * surf parse bypass platform
 */
static int surf_parse_bypass_platform()
{
	char buffer[22];
	unsigned int i,j;
	char* link_id;

	p_host_attr p_host;
	p_link_attr p_link;
	p_route_attr p_route;

	static int AX_ptr = 0;
	static int surfxml_bufferstack_size = 2048;

	  /* FIXME allocating memory for the buffer, I think 2kB should be enough */
	surfxml_bufferstack = xbt_new0(char, surfxml_bufferstack_size);
	  /* <platform> */
	SURFXML_BUFFER_SET(platform_version, "2");
#ifndef BYPASS_CPU
	SURFXML_START_TAG(platform);
#endif

	// Add Hosts
	xbt_dynar_foreach(host_list_d,i,p_host)
	{

#ifdef BYPASS_MODEL
		INFO0("Bypass_Cpu");
		create_host(p_host->id,p_host->power);
#else

		SURFXML_BUFFER_SET(host_id,p_host->id);
		sprintf(buffer,"%f",p_host->power);
		SURFXML_BUFFER_SET(host_power,buffer);
		SURFXML_BUFFER_SET(host_availability, "1.0");
		SURFXML_BUFFER_SET(host_availability_file, "");
		A_surfxml_host_state = A_surfxml_host_state_ON;
		SURFXML_BUFFER_SET(host_state_file, "");
		SURFXML_BUFFER_SET(host_interference_send, "1.0");
		SURFXML_BUFFER_SET(host_interference_recv, "1.0");
		SURFXML_BUFFER_SET(host_interference_send_recv, "1.0");
		SURFXML_BUFFER_SET(host_max_outgoing_rate, "-1.0");
		SURFXML_START_TAG(host);
		SURFXML_END_TAG(host);
#endif
	}

	//add Links
	INFO0("Start Adding Links");
	xbt_dynar_foreach(link_list_d,i,p_link)
	{
#ifdef BYPASS_MODEL

		INFO0("Bypass_Network");
		surf_link_create_resouce((char*)p_link->id,p_link->bandwidth,p_link->latency);
#else

		SURFXML_BUFFER_SET(link_id,p_link->id);
		sprintf(buffer,"%f",p_link->bandwidth);
		SURFXML_BUFFER_SET(link_bandwidth,buffer);
		SURFXML_BUFFER_SET(link_bandwidth_file, "");
		sprintf(buffer,"%f",p_link->latency);
		SURFXML_BUFFER_SET(link_latency,buffer);
		SURFXML_BUFFER_SET(link_latency_file, "");
		A_surfxml_link_state = A_surfxml_link_state_ON;
		SURFXML_BUFFER_SET(link_state_file, "");
		A_surfxml_link_sharing_policy = A_surfxml_link_sharing_policy_SHARED;
		SURFXML_START_TAG(link);
		SURFXML_END_TAG(link);
#endif
	}

	// add route
	INFO0("Start Adding routes");
	xbt_dynar_foreach(route_list_d,i,p_route)
	{
#ifndef BYPASS_MODEL

		SURFXML_BUFFER_SET(route_src,p_route->src_id);
		SURFXML_BUFFER_SET(route_dst,p_route->dest_id);
		SURFXML_BUFFER_SET(route_impact_on_src, "0.0");
		SURFXML_BUFFER_SET(route_impact_on_dst, "0.0");
		SURFXML_BUFFER_SET(route_impact_on_src_with_other_recv, "0.0");
		SURFXML_BUFFER_SET(route_impact_on_dst_with_other_send, "0.0");
		SURFXML_START_TAG(route);

		xbt_dynar_foreach(p_route->links_id,j,link_id)
		{
			SURFXML_BUFFER_SET(link_c_ctn_id,link_id);
			SURFXML_START_TAG(link_c_ctn);
			SURFXML_END_TAG(link_c_ctn);
		}

		SURFXML_END_TAG(route);
#endif
	}
	/* </platform> */
#ifndef BYPASS_MODEL
	SURFXML_END_TAG(platform);
#endif

	free(surfxml_bufferstack);
	return 0; // must return 0 ?!!

}
/*
 * surf parse bypass application
 */
static int surf_parse_bypass_application()
{
	  unsigned int i,j;
	  p_host_attr p_host;
	  char * arg;
	  static int AX_ptr;
	  static int surfxml_bufferstack_size = 2048;
	  /* FIXME ( should be manual )allocating memory to the buffer, I think 2MB should be enough */
	  surfxml_bufferstack = xbt_new0(char, surfxml_bufferstack_size);
	  /* <platform> */
	  SURFXML_BUFFER_SET(platform_version, "2");
	  SURFXML_START_TAG(platform);

	  xbt_dynar_foreach(host_list_d,i,p_host)
	  {
		  if(p_host->function)
		  {
			  SURFXML_BUFFER_SET(process_host, p_host->id);
			  SURFXML_BUFFER_SET(process_function, p_host->function);
			  SURFXML_BUFFER_SET(process_start_time, "-1.0");
			  SURFXML_BUFFER_SET(process_kill_time, "-1.0");
			  SURFXML_START_TAG(process);
			  //args
			  xbt_dynar_foreach(p_host->args_list,j,arg)
			  {
				  SURFXML_BUFFER_SET(argument_value,arg);
				  SURFXML_START_TAG(argument);
				  SURFXML_END_TAG(argument);
			  }
			  SURFXML_END_TAG(process);
		  }
	  }
	  /* </platform> */
	  SURFXML_END_TAG(platform);
	  free(surfxml_bufferstack);
	  return 0;
}

//***********Register Methods *******************************************//
/*
 * Host Methods
 */
static const luaL_reg Host_methods[] = {
    {"getByName",   Host_get_by_name},
    {"name",  		Host_get_name},
    {"number",    	Host_number},
    {"at",			Host_at},
    // Bypass XML Methods
    {"new",			Host_new},
    {"setFunction",	Host_set_function},
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
 * Link Methods
 */
static const luaL_reg Link_methods[] = {
    {"new",Link_new},
    {0,0}
};
/*
 * Route Methods
 */
static const luaL_reg Route_methods[] ={
   {"new",Route_new},
};

/*
 * Environment related
 */

extern lua_State *simgrid_lua_state;

static int run_lua_code(int argc,char **argv) {
  DEBUG1("Run lua code %s",argv[0]);
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

/*
 * Bypass XML Pareser
 */
static int register_platform(lua_State *L)
{
	/* Tell Simgrid we dont wanna use its parser*/
	surf_parse = surf_parse_bypass_platform;
	MSG_create_environment(NULL);
	return 0;
}

static int register_application(lua_State *L)
{
	 MSG_function_register_default(run_lua_code);
	 surf_parse = surf_parse_bypass_application;
	 MSG_launch_application(NULL);
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
    /* methods to bypass XML parser*/
    { "register_platform",register_platform},
    { "register_application",register_application},
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

  /* register the links methods to lua*/
  luaL_openlib(L,LINK_MODULE_NAME,Link_methods,0);
  luaL_newmetatable(L,LINK_MODULE_NAME);
  lua_pop(L,1);

  /*register the routes methods to lua*/
  luaL_openlib(L,ROUTE_MODULE_NAME,Route_methods,0);
  luaL_newmetatable(L,LINK_MODULE_NAME);
  lua_pop(L,1);

  /* Keep the context mechanism informed of our lua world today */
  simgrid_lua_state = L;
  return 1;
}
