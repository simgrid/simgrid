/* SimGrid Lua bindings                                                     */

/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <lauxlib.h>
#include <lualib.h>
#include "msg/msg.h"
#include "simdag/simdag.h"
#include <gras.h>
#include "xbt.h"
#include "lua_stub_generator.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(lua,bindings,"Lua Bindings");

#define TASK_MODULE_NAME "simgrid.Task"
#define HOST_MODULE_NAME "simgrid.Host"
// Surf ( bypass XML )
#define LINK_MODULE_NAME "simgrid.Link"
#define ROUTE_MODULE_NAME "simgrid.Route"
#define AS_MODULE_NAME "simgrid.AS"

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

typedef struct t_AS_attr
{
	const char* id;
	const char *mode;
}AS_attr,*p_AS_attr;

typedef struct t_host_attr
{
	//platform attribute
	// Mandatory attributes
	const char* id;
	double power_peak;
	// Optional attributes
	double power_scale;
	const char *power_trace;
	int state_initial;
	const char *state_trace;
	//deployment attribute
	const char* function;
	xbt_dynar_t args_list;
}host_attr,*p_host_attr;

typedef struct t_link_attr
{
	//mandatory attributes
	const char* id;
	double bandwidth;
	double latency;
	// Optional attributes
	const char* bandwidth_trace;
	const char* latency_trace;
	const char*	state_trace;
	int state_initial;
	int policy;
}link_attr,*p_link_attr;

typedef struct t_route_attr
{
	const char *src_id;
	const char *dest_id;
	xbt_dynar_t links_id;

}route_attr,*p_route_attr;

//AS : One struct needed
static p_AS_attr AS;

//using xbt_dynar_t :
static xbt_dynar_t host_list_d ;
static xbt_dynar_t link_list_d ;
static xbt_dynar_t route_list_d ;


/*
 * Initialize platform model routing
 */
static void create_AS(const char* id,const char * mode)
{
	surf_AS_new(id,mode);
}

/**
 * create host resource via CPU model [for MSG]
 */

static void create_host(const char* id,double power_peak,double power_sc,
						const char* power_tr,int state_init,
						const char* state_tr)
{

	double power_scale = 1.0;
	tmgr_trace_t power_trace = NULL;
	e_surf_resource_state_t state_initial;
	tmgr_trace_t state_trace;
	if(power_sc) // !=0
		power_scale = power_sc;
	if (state_init == -1)
		state_initial = SURF_RESOURCE_OFF;
	else
		state_initial = SURF_RESOURCE_ON;
	if(power_tr)
		power_trace = tmgr_trace_new(power_tr);
	else
		power_trace = tmgr_trace_new("");
	if(state_tr)
		state_trace = tmgr_trace_new(state_tr);
	else
		state_trace = tmgr_trace_new("");
	current_property_set = xbt_dict_new();
	surf_host_create_resource(xbt_strdup(id), power_peak, power_scale,
					       power_trace, state_initial, state_trace, current_property_set);

}

/**
 * create link resource via network model
 */
static void create_link(const char *name,
        double bw_initial,const char *trace,double lat_initial,
        const char* latency_trace,int state_init, const char* state_trace,int policy)
{
	tmgr_trace_t bw_trace;
	tmgr_trace_t lat_trace;
	e_surf_resource_state_t state_initial_link = SURF_RESOURCE_ON;
	e_surf_link_sharing_policy_t policy_initial_link = SURF_LINK_SHARED;
	tmgr_trace_t st_trace;
	if(trace)
		bw_trace = tmgr_trace_new(trace);
	else
		bw_trace = tmgr_trace_new("");

	if(latency_trace)
		lat_trace = tmgr_trace_new(latency_trace);
	else
		lat_trace = tmgr_trace_new("");

	if(state_trace)
		st_trace = tmgr_trace_new(state_trace);
	else
		st_trace = tmgr_trace_new("");

	if(state_init == -1)
		state_initial_link = SURF_RESOURCE_OFF;
	if(policy == -1)
		policy_initial_link = SURF_LINK_FATPIPE;

	surf_link_create_resource(xbt_strdup(name), bw_initial, bw_trace,
	           lat_initial, lat_trace, state_initial_link, st_trace,
	           policy_initial_link, xbt_dict_new());
}


/*
 *create host resource via workstation_ptask_L07 model [for SimDag]
 */
static void create_host_wsL07(const char* id,double power_peak,double power_sc,
						const char* power_tr,int state_init,
						const char* state_tr)
{
	double power_scale = 1.0;
	tmgr_trace_t power_trace = NULL;
	e_surf_resource_state_t state_initial;
	tmgr_trace_t state_trace;
	if(power_sc) // !=0
		power_scale = power_sc;
	if (state_init == -1)
		state_initial = SURF_RESOURCE_OFF;
	else
		state_initial = SURF_RESOURCE_ON;
	if(power_tr)
		power_trace = tmgr_trace_new(power_tr);
	else
		power_trace = tmgr_trace_new("");
	if(state_tr)
		state_trace = tmgr_trace_new(state_tr);
	else
		state_trace = tmgr_trace_new("");
	current_property_set = xbt_dict_new();
	surf_wsL07_host_create_resource(xbt_strdup(id), power_peak, power_scale,
					       power_trace, state_initial, state_trace, current_property_set);

}

/**
 * create link resource via workstation_ptask_L07 model [for SimDag]
 */

static void create_link_wsL07(const char *name,
        double bw_initial,const char *trace,double lat_initial,
        const char* latency_trace,int state_init, const char* state_trace,int policy)
{
	tmgr_trace_t bw_trace;
	tmgr_trace_t lat_trace;
	e_surf_resource_state_t state_initial_link = SURF_RESOURCE_ON;
	e_surf_link_sharing_policy_t policy_initial_link = SURF_LINK_SHARED;
	tmgr_trace_t st_trace;
	if(trace)
		bw_trace = tmgr_trace_new(trace);
	else
		bw_trace = tmgr_trace_new("");

	if(latency_trace)
		lat_trace = tmgr_trace_new(latency_trace);
	else
		lat_trace = tmgr_trace_new("");

	if(state_trace)
		st_trace = tmgr_trace_new(state_trace);
	else
		st_trace = tmgr_trace_new("");

	if(state_init == -1)
		state_initial_link = SURF_RESOURCE_OFF;
	if(policy == -1)
		policy_initial_link = SURF_LINK_FATPIPE;

	surf_wsL07_link_create_resource(xbt_strdup(name), bw_initial, bw_trace,
	           lat_initial, lat_trace, state_initial_link, st_trace,
	           policy_initial_link, xbt_dict_new());
}


/*
 * init AS
 */

static int AS_new(lua_State *L)
{
	const char *id;
	const char *mode;
	if(lua_istable(L,1))
	{
		lua_pushstring(L,"id");
		lua_gettable(L,-2);
		id = lua_tostring(L,-1);
		lua_pop(L,1);

		lua_pushstring(L,"mode");
		lua_gettable(L,-2);
		mode = lua_tostring(L,-1);
		lua_pop(L,1);
	}
	else {
        ERROR0("Bad Arguments to AS.new, Should be a table with named arguments");
        return -1;
		}
	AS = malloc(sizeof(AS_attr));
	AS->id = id;
	AS->mode = mode;

	return 0;
}

/*
 * add new host to platform hosts list
 */
static int Host_new(lua_State *L)
{

	if(xbt_dynar_is_empty(host_list_d))
		host_list_d = xbt_dynar_new(sizeof(p_host_attr), &xbt_free_ref);

	p_host_attr host;
	const char * id;
	const char *power_trace;
	const char *state_trace;
	double power,power_scale;
	int state_initial;
	//get values from the table passed as argument
    if (lua_istable(L,-1)) {

            // get Id Value
            lua_pushstring(L,"id");
            lua_gettable(L, -2 );
            id = lua_tostring(L,-1);
            lua_pop(L,1);

            // get power value
            lua_pushstring(L,"power");
            lua_gettable(L, -2 );
            power = lua_tonumber(L,-1);
            lua_pop(L,1);

            //get power_scale
            lua_pushstring(L,"power_scale");
            lua_gettable(L, -2 );
            power_scale = lua_tonumber(L,-1);
            lua_pop(L,1);

            //get power_trace
            lua_pushstring(L,"power_trace");
            lua_gettable(L, -2 );
            power_trace = lua_tostring(L,-1);
            lua_pop(L,1);

            //get state initial
            lua_pushstring(L,"state_initial");
            lua_gettable(L, -2 );
            state_initial = lua_tonumber(L,-1);
            lua_pop(L,1);

            //get trace state
            lua_pushstring(L,"state_trace");
            lua_gettable(L, -2 );
            state_trace = lua_tostring(L,-1);
            lua_pop(L,1);

    } else {
            ERROR0("Bad Arguments to create host, Should be a table with named arguments");
            return -1;
    }

	    host = malloc(sizeof(host_attr));
		host->id = id;
		host->power_peak = power;
		host->power_scale = power_scale;
		host->power_trace = power_trace;
		host->state_initial = state_initial;
		host->state_trace = state_trace;
		host->function = NULL;
		xbt_dynar_push(host_list_d, &host);

    return 0;
}

/**
 * add link to platform links list
 */
static int Link_new(lua_State *L) // (id,bandwidth,latency)
{
	if(xbt_dynar_is_empty(link_list_d))
		link_list_d = xbt_dynar_new(sizeof(p_link_attr), &xbt_free_ref);

	const char* id;
	double bandwidth,latency;
	const char* bandwidth_trace;
	const char* latency_trace;
	const char* state_trace;
	int state_initial,policy;

	//get values from the table passed as argument
	if (lua_istable(L,-1)) {
	            // get Id Value
	            lua_pushstring(L,"id");
	            lua_gettable(L, -2 );
	            id = lua_tostring(L,-1);
	            lua_pop(L,1);

	            // get bandwidth value
	            lua_pushstring(L,"bandwidth");
	            lua_gettable(L, -2 );
	            bandwidth = lua_tonumber(L,-1);
	            lua_pop(L,1);

	            //get latency value
	            lua_pushstring(L,"latency");
	            lua_gettable(L, -2 );
	            latency = lua_tonumber(L,-1);
	            lua_pop(L,1);

	            /*Optional Arguments  */

	            //get bandwidth_trace value
	            lua_pushstring(L,"bandwidth_trace");
	            lua_gettable(L, -2 );
	            bandwidth_trace = lua_tostring(L,-1);
	            lua_pop(L,1);

	            //get latency_trace value
	            lua_pushstring(L,"latency_trace");
	            lua_gettable(L, -2 );
	            latency_trace = lua_tostring(L,-1);
	            lua_pop(L,1);

	            //get state_trace value
	            lua_pushstring(L,"state_trace");
	            lua_gettable(L, -2 );
	            state_trace = lua_tostring(L,-1);
	            lua_pop(L,1);

				//get state_initial value
	            lua_pushstring(L,"state_initial");
	            lua_gettable(L, -2 );
	            state_initial = lua_tonumber(L,-1);
	            lua_pop(L,1);


				//get policy value
	            lua_pushstring(L,"policy");
	            lua_gettable(L, -2 );
	            policy = lua_tonumber(L,-1);
	            lua_pop(L,1);

	    } else {
	            ERROR0("Bad Arguments to create link, Should be a table with named arguments");
	            return -1;
	    }

	p_link_attr link = malloc(sizeof(link_attr));
	link->id = id;
	link->bandwidth = bandwidth;
	link->latency = latency;
	link->bandwidth_trace = bandwidth_trace;
	link->latency_trace = latency_trace;
	link->state_trace = state_trace;
	link->state_initial= state_initial;
	link->policy = policy;
	xbt_dynar_push(link_list_d,&link);
	return 0;
}

/**
 * add route to platform routes list
 */
static int Route_new(lua_State *L) // (src_id,dest_id,links_number,link_table)
{
	if(xbt_dynar_is_empty(route_list_d))
		route_list_d = xbt_dynar_new(sizeof(p_route_attr), &xbt_free_ref);
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

/**
 * set function to process
 */
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
			if(lua_istable(L,3))
			{
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
			}
			lua_pop(L, 1);
			return 0;
		}
	}
	ERROR1("Host : %s Not Found !!",host_id);
	return 1;
}

/*
 * surf parse bypass platform
 * through CPU/network Models
 */

static int surf_parse_bypass_platform()
{
	unsigned int i;
	p_host_attr p_host;
	p_link_attr p_link;
	p_route_attr p_route;

	// Init routing mode
	create_AS(AS->id,AS->mode);


	// Add Hosts
	xbt_dynar_foreach(host_list_d,i,p_host)
	{
		create_host(p_host->id,p_host->power_peak,p_host->power_scale,p_host->power_trace,
					p_host->state_initial,p_host->state_trace);
		//add to routing model host list
		surf_route_add_host((char*)p_host->id);

	}

	//add Links
	xbt_dynar_foreach(link_list_d,i,p_link)
	{
		create_link(p_link->id,p_link->bandwidth,p_link->bandwidth_trace,p_link->latency,
				p_link->latency_trace,p_link->state_initial,p_link->state_trace,p_link->policy);
	}
	// add route
	xbt_dynar_foreach(route_list_d,i,p_route)
	{
		surf_routing_add_route((char*)p_route->src_id,(char*)p_route->dest_id,p_route->links_id);
	}
	/* </platform> */

	// Finalize AS
	surf_AS_finalize(AS->id);

	// add traces
	surf_add_host_traces();
	surf_add_link_traces();

	return 0; // must return 0 ?!!

}

/**
 *
 * surf parse bypass platform
 * through workstation_ptask_L07 Model
 */


static int surf_wsL07_parse_bypass_platform()
{
	unsigned int i;
	p_host_attr p_host;
	p_link_attr p_link;
	p_route_attr p_route;

	// Init routing mode
	create_AS(AS->id,AS->mode);

	// Add Hosts
	xbt_dynar_foreach(host_list_d,i,p_host)
	{
		create_host_wsL07(p_host->id,p_host->power_peak,p_host->power_scale,p_host->power_trace,
					p_host->state_initial,p_host->state_trace);
		//add to routing model host list
		surf_route_add_host((char*)p_host->id);
	}

	//add Links
	xbt_dynar_foreach(link_list_d,i,p_link)
	{
		create_link_wsL07(p_link->id,p_link->bandwidth,p_link->bandwidth_trace,p_link->latency,
						p_link->latency_trace,p_link->state_initial,p_link->state_trace,p_link->policy);
	}
	// add route
	xbt_dynar_foreach(route_list_d,i,p_route)
	{
		surf_routing_add_route((char*)p_route->src_id,(char*)p_route->dest_id,p_route->links_id);
	}
	/* </platform> */

	// Finalize AS
	surf_AS_finalize(AS->id);
	// add traces
	surf_wsL07_add_traces();

	return 0;
}

/*
 * surf parse bypass application for MSG Module
 */
static int surf_parse_bypass_application()
{
	  unsigned int i;
	  p_host_attr p_host;
	  xbt_dynar_foreach(host_list_d,i,p_host)
	  	  {
		  if(p_host->function)
			  MSG_set_function(p_host->id,p_host->function,p_host->args_list);
	  	  }
	  return 0;
}


/**
 *  Generate Gras Templates File from lua
 */

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

static int gras_add_process_function(lua_State *L)
{
	const char * arg;
	const char* process_host = luaL_checkstring(L,1);
	const char *process_function = luaL_checkstring(L,2);

	if(xbt_dict_is_empty(machine_set) || xbt_dict_is_empty(process_function_set)
			|| xbt_dynar_is_empty(process_list))
		{
		  process_function_set = xbt_dict_new();
		  process_list = xbt_dynar_new(sizeof(s_process_t), s_process_free);
		  machine_set = xbt_dict_new();
		}

	xbt_dict_set(machine_set,process_host, NULL, NULL);
	xbt_dict_set(process_function_set,process_function, NULL, NULL);

	process.argc = 1;
	process.argv = xbt_new(char *, 1);
	process.argv[0] = xbt_strdup(process_function);
	process.host = strdup(process_host);

	lua_pushnil(L);
	while (lua_next(L,3) != 0) {
		arg = lua_tostring(L, -1);
		process.argc++;
		process.argv = xbt_realloc(process.argv, (process.argc) * sizeof(char *));
		process.argv[(process.argc) - 1] = xbt_strdup(arg);

		DEBUG2("index = %f , arg = %s \n",lua_tonumber(L, -2),lua_tostring(L, -1));
		lua_pop(L, 1);
		}
	lua_pop(L, 1);
	//add to the process list
	xbt_dynar_push(process_list, &process);

	return 0;

}


static int gras_generate(lua_State *L)
{
  const char *project_name = luaL_checkstring(L,1);
  generate_sim(project_name);
  generate_rl(project_name);
  generate_makefile_local(project_name);

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
 * AS Methods
 */
static const luaL_reg AS_methods[] = {
    {"new",AS_new},
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
   {0,0}
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
 * Bypass XML Parser
 */

/*
 * Register platform for MSG
 */
static int msg_register_platform(lua_State *L)
{
	/* Tell Simgrid we dont wanna use its parser*/
	surf_parse = surf_parse_bypass_platform;
	MSG_create_environment(NULL);
	return 0;
}

/*
 * Register platform for Simdag
 */

static int sd_register_platform(lua_State *L)
{
	surf_parse = surf_wsL07_parse_bypass_platform;
	SD_create_environment(NULL);
	return 0;
}
/*
 * Register platform for gras
 */
static int gras_register_platform(lua_State *L)
{
	/* Tell Simgrid we dont wanna use surf parser*/
	surf_parse = surf_parse_bypass_platform;
	gras_create_environment(NULL);
	return 0;
}

/**
 * Register applicaiton for MSG
 */
static int msg_register_application(lua_State *L)
{
	 MSG_function_register_default(run_lua_code);
	 surf_parse = surf_parse_bypass_application;
	 MSG_launch_application(NULL);
	 return 0;
}

/*
 * Register application for gras
 */
static int gras_register_application(lua_State *L)
{
	gras_function_register_default(run_lua_code);
	surf_parse = surf_parse_bypass_application;
	gras_launch_application(NULL);
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
    { "msg_register_platform",msg_register_platform},
    { "sd_register_platform",sd_register_platform},
    { "msg_register_application",msg_register_application},
    { "gras_register_platform",gras_register_platform},
    { "gras_register_application",gras_register_application},
    /* gras sub generator method*/
    {"gras_set_process_function",gras_add_process_function},
    {"gras_generate",gras_generate},
    { NULL, NULL }
};

/* ********************************************************************************* */
/*                       module management functions                                 */
/* ********************************************************************************* */

extern const char*xbt_ctx_factory_to_use; /*Hack: let msg load directly the right factory */

#define LUA_MAX_ARGS_COUNT 10 /* maximum amount of arguments we can get from lua on command line */
#define TEST
int luaopen_simgrid(lua_State* L); // Fuck gcc: we don't need that prototype
int luaopen_simgrid(lua_State* L) {

  //xbt_ctx_factory_to_use = "lua";
  char **argv=malloc(sizeof(char*)*LUA_MAX_ARGS_COUNT);
  int argc=1;
  argv[0] = (char*)"/usr/bin/lua"; /* Lie on the argv[0] so that the stack dumping facilities find the right binary. FIXME: what if lua is not in that location? */
  /* Get the command line arguments from the lua interpreter */
  lua_getglobal(L,"arg");
  /* if arg is a null value, it means we use lua only as a script to init platform
   * else it should be a table and then take arg in consideration
   */
  if(lua_istable(L,-1))
  {
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
 }
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
  luaL_openlib(L,AS_MODULE_NAME,AS_methods,0);
  luaL_newmetatable(L,AS_MODULE_NAME);
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
