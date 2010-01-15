#include <stdio.h>
#include "lauxlib.h"
#include "lualib.h"

// Msg Includes
#include <stdio.h>
#include "msg/msg.h"         
#include "xbt/sysdep.h"        
#include "xbt/log.h"
#include "xbt/asserts.h"




/*
=============================================

	Example Lua Bindings for Msg

=============================================
*/




#define TASK "Task"
#define HOST "Host"
#define PROCESS "Process"

typedef m_task_t Task;
typedef m_host_t Host;
typedef m_process_t Process;

//**********************************************************TASK SECTION***************************************************


static Task toTask(lua_State *L,int index)
	{
	Task *pi = (Task*) lua_touserdata(L,index);
	if(pi == NULL) luaL_typerror(L,index,TASK);
	return *pi;
	}
/**
*checkTask ensures that a userdata on the stack is the correct type, and returns the Task pointer inside the userdata
*/

static Task checkTask (lua_State *L,int index)
	{
	Task *pi,tk;
	luaL_checktype(L,index,LUA_TUSERDATA);
	pi = (Task*)luaL_checkudata(L,index,TASK);
	if(pi == NULL ) luaL_typerror(L,index,TASK);
	tk = *pi;
	if(!tk)
		luaL_error(L,"null Task");
	return  tk;
	}

/**
*pushTask leaves a new userdata on top of the stack, sets its metatable, and sets the Task pointer inside the userdata.
*/

static Task *pushTask (lua_State *L,Task tk)
	{
	Task *pi = (Task*)lua_newuserdata(L,sizeof(Task));
	*pi=tk;
	luaL_getmetatable(L,TASK);
	lua_setmetatable(L,-2);
	return pi;

	}

/**
*Task.new is a constructor that returns a userdata containing a pointer the Task to be manipulated
*/


static int Task_new(lua_State* L)
	{
	char *name=luaL_checkstring(L,1);
	int comp_size = luaL_checkint(L,2);
	int msg_size = luaL_checkint(L,3);
	// We Will Set The Data as a Null for The Moment
	pushTask(L,MSG_task_create(name,comp_size,msg_size,NULL));
	return 1;		

	}

/**
* Function to return the Task Name
*/


static int Task_name(lua_State *L)
	{
	Task tk = checkTask(L,1);
	lua_pushstring(L,MSG_task_get_name(tk));
	return 1;
	}


/**
* Function to return the Computing Size of a Task
*/

static int Task_comp(lua_State *L)
	{
	Task tk = checkTask(L,1);
	lua_pushnumber(L,MSG_task_get_compute_duration (tk));
	return 1;
	}


/**
*Function to Excute a Task
*/

static int Task_execute(lua_State *L)
	{
	Task tk = checkTask(L,1);
	int res = MSG_task_execute(tk);
	lua_pushnumber(L,res);
	return 1;
	}

/**
*Function ro Desroy a Task
*/
static int Task_destroy(lua_State *L)
	{	
	Task tk = checkTask(L,1);
	int res = MSG_task_destroy(tk);
	lua_pushnumber(L,res);
	return 1;
	}






//***************************************************************  HOST SECTION  ***************************************************************************



static Host toHost(lua_State *L,int index)
	{
	Host *pi = (Host*) lua_touserdata(L,index);
	if(pi == NULL) luaL_typerror(L,index,HOST);
	return *pi;
	}



static Host checkHost (lua_State *L,int index)
	{
	Host *pi,ht;
	luaL_checktype(L,index,LUA_TUSERDATA);
	pi = (Host*)luaL_checkudata(L,index,HOST);
	if(pi == NULL ) luaL_typerror(L,index,HOST);
	ht = *pi;
	if(!ht)
		luaL_error(L,"null Host");
	return  ht;
	}



static Host *pushHost (lua_State *L,Host ht)
	{
	Host *pi = (Host*)lua_newuserdata(L,sizeof(Host));
	*pi=ht;
	luaL_getmetatable(L,HOST);
	lua_setmetatable(L,-2);
	return pi;

	}


static int Host_new(lua_State* L)
	{
	char *host_name=luaL_checkstring(L,1);
	pushHost(L,MSG_get_host_by_name(host_name));
	return 1;		
	}



static int Host_name(lua_State* L)
	{
	Host ht = checkHost(L,1);
	lua_pushstring(L,MSG_host_get_name(ht));	
	return 1;
	}


//*******************************************************   PROCESS SECTION      ***************************************************************************
static Process toProcess(lua_State *L,int index)
	{
	Process *pi = (Process*) lua_touserdata(L,index);
	if(pi == NULL) luaL_typerror(L,index,PROCESS);
	return *pi;
	}



static Process checkProcess (lua_State *L,int index)
	{
	Process *pi,ps;
	luaL_checktype(L,index,LUA_TUSERDATA);
	pi = (Process*)luaL_checkudata(L,index,PROCESS);
	if(pi == NULL ) luaL_typerror(L,index,PROCESS);
	ps = *pi;
	if(!ps)
		luaL_error(L,"null Process");
	return  ps;
	}



static Process *pushProcess (lua_State *L,Process ps)
	{
	Process *pi = (Process*)lua_newuserdata(L,sizeof(Process));
	*pi=ps;
	luaL_getmetatable(L,PROCESS);
	lua_setmetatable(L,-2);
	return pi;

	}

/*
m_process_t MSG_process_create  	(  	const char *   	 name,
		xbt_main_func_t  	code,
		void *  	data,
		m_host_t  	host	 
	) 	
*/



static int Process_new(lua_State* L)
	{
	char *name=luaL_checkstring(L,1);
	//int code = luaL_checkint(L,2);
	//xbt_main_func_t << code  
	// We Will Set The Data as a Null for The Moment
	Host ht = checkHost(L,4); 
	pushProcess(L,MSG_process_create(name,NULL,NULL,ht));
	return 1;		

	}


static int Process_kill(lua_State *L)
	{
	Process ps = checkProcess (L,1);
	MSG_process_kill(ps);
	return 0;
	}



//**********************************************************MSG Operating System Functions******************************************************************
/**
* Function to Send a Task
*/
static void Task_send(lua_State *L)	{
  Task tk = checkTask(L,1);
	char *mailbox = luaL_checkstring(L,2);
	
  int res = MSG_task_send(tk,mailbox);
}
/**
* Function to Get ( Recieve !! ) a Task
*/
static int Task_recv(lua_State *L)	{
  m_task_t tk = NULL;

	char *mailbox = luaL_checkstring(L,1);
	int res = MSG_task_receive(&tk,mailbox);
	pushTask(L,tk);
	return 1;
}


//*******************************************************   PLATFORM & APPLICATION SECTION  ****************************************************************


 //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> WE WONT NEED IT (!!??) 
/**
* Launch Application Function
*/

static int launch_application(lua_State *L)
	{

	const char * file = luaL_checkstring(L,1);
	MSG_launch_application(file);
	return 0;

	}

/**
* Create Environment Function
*/

static int create_environment(lua_State *L)
	{

	const char *file = luaL_checkstring(L,1);
	MSG_create_environment(file);
	return 0;

	}

/**
* Function Register
*/

static int function_register()
	{

	//Code ??!!!

	}

//******************************************************************       REGISTERING          ************************************************************
/**
*Registering Into Lua
*/


static const luaL_reg Task_methods[] = {
{"new",		Task_new},
{"name",	Task_name},
{"comp",	Task_comp},
{"execute",	Task_execute},
{"destroy",	Task_destroy},
{"send",		Task_send},
{"recv",		Task_recv},
{0,0}
};


static const luaL_reg Host_methods[] = {
{"new",		Host_new},
{"name",	Host_name},
{0,0}
};


static const luaL_reg Process_methods[] = {
{"new",		Process_new},
{"kill",	Process_kill},
{0,0}
};




/**
* Task_meta
*/
static int Task_gc(lua_State *L)
	{
	Task tk=toTask(L,1);
	if (tk) MSG_task_destroy(tk);
	printf("GoodBye Task(%p)\n",lua_touserdata(L,1));
	return 0; 
	}

static int Task_tostring(lua_State *L)
	{
	lua_pushfstring(L,"Task :%p",lua_touserdata(L,1));
	return 1;	
	}


static const luaL_reg Task_meta[] = {
	{"__gc",	Task_gc},
	{"__tostring",	Task_tostring},
	{0,0}
};




/**
* Host_meta
*/
static int Host_gc(lua_State *L)
	{
	Host ht=toHost(L,1);
	if (ht) ht=NULL;
	printf("GoodBye Host(%p)\n",lua_touserdata(L,1));
	return 0; 
	}

static int Host_tostring(lua_State *L)
	{
	lua_pushfstring(L,"Host :%p",lua_touserdata(L,1));
	return 1;	
	}


static const luaL_reg Host_meta[] = {
	{"__gc",	Host_gc},
	{"__tostring",	Host_tostring},
	{0,0}
};


/**
* Process_meta
*/
static int Process_gc(lua_State *L)
	{
	Process ps=toProcess(L,1);
	if (ps) MSG_process_kill(ps) ;
	printf("GoodBye Process(%p)\n",lua_touserdata(L,1));
	return 0; 
	}

static int Process_tostring(lua_State *L)
	{
	lua_pushfstring(L,"Process :%p",lua_touserdata(L,1));
	return 1;	
	}


static const luaL_reg Process_meta[] = {
	{"__gc",	Process_gc},
	{"__tostring",	Process_tostring},
	{0,0}
};


/**
*The metatable for the userdata is put in the registry, and the __index field points to the table of methods so that the object:method() syntax will work. The methods table is stored in the table of globals so that scripts can add methods written in Lua. 
*/

/**
* Task Register
*/

int Task_register(lua_State *L)
	{	
	luaL_openlib(L,TASK,Task_methods,0); //create methods table,add it to the globals
	luaL_newmetatable(L,TASK); //create metatable for Task,add it to the Lua registry
	luaL_openlib(L,0,Task_meta,0);// fill metatable
	lua_pushliteral(L,"__index");
	lua_pushvalue(L,-3);	//dup methods table
	lua_rawset(L,-3);	//matatable.__index = methods
	lua_pushliteral(L,"__metatable");
	lua_pushvalue(L,-3);	//dup methods table
	lua_rawset(L,-3);	//hide metatable:metatable.__metatable = methods 
	lua_pop(L,1);		//drop metatable
	return 1;
	}
	  
/**
* Host Register
*/

int Host_register(lua_State *L)
	{	
	luaL_openlib(L,HOST,Host_methods,0); 
	luaL_newmetatable(L,HOST); 
	luaL_openlib(L,0,Host_meta,0);
	lua_pushliteral(L,"__index");
	lua_pushvalue(L,-3);	
	lua_rawset(L,-3);	
	lua_pushliteral(L,"__metatable");
	lua_pushvalue(L,-3);	
	lua_rawset(L,-3);	
	lua_pop(L,1);		
	return 1;
	}

/**
*  Process Register
*/

int Process_register(lua_State *L)
	{	
	luaL_openlib(L,PROCESS,Process_methods,0); 
	luaL_newmetatable(L,PROCESS); 
	luaL_openlib(L,0,Host_meta,0);
	lua_pushliteral(L,"__index");
	lua_pushvalue(L,-3);	
	lua_rawset(L,-3);	
	lua_pushliteral(L,"__metatable");
	lua_pushvalue(L,-3);	
	lua_rawset(L,-3);	
	lua_pop(L,1);		
	return 1;
	}

