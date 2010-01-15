/* $Id$ */

/* context_lua - implementation of context switching with lua coroutines */

/* Copyright (c) 2004-2008 the SimGrid team. All right reserved */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "context_sysv_config.h"        /* loads context system definitions */
#include "portable.h"
#include <lua5.1/lauxlib.h>
#include <lua5.1/lualib.h>

// FIXME: better location for that
extern void MSG_register(lua_State *L);

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(smx_lua);

typedef struct s_smx_ctx_sysv {
  SMX_CTX_BASE_T;
  lua_State *state;
  int ref; /* to prevent the lua GC from collecting my threads, I ref them explicitely */
  int nargs; /* argument to lua_resume. First time: argc-1, afterward: 0 */
} s_smx_ctx_lua_t, *smx_ctx_lua_t;

static lua_State *lua_state;

static smx_context_t 
smx_ctx_lua_create_context(xbt_main_func_t code, int argc, char** argv, 
                                    void_f_pvoid_t cleanup_func, void* cleanup_arg);

static int smx_ctx_lua_factory_finalize(smx_context_factory_t *factory);

static void smx_ctx_lua_free(smx_context_t context);

static void smx_ctx_lua_start(smx_context_t context);

static void smx_ctx_lua_stop(smx_context_t context);

static void smx_ctx_lua_suspend(smx_context_t context);

static void 
  smx_ctx_lua_resume(smx_context_t old_context, smx_context_t new_context);

static void smx_ctx_sysv_wrapper(void);

void SIMIX_ctx_lua_factory_loadfile(const char *file) {
  if (luaL_loadfile(lua_state, file) || lua_pcall(lua_state, 0, 0, 0))
    THROW2(unknown_error,0,"error while parsing %s: %s", file, lua_tostring(lua_state, -1));
  INFO1("File %s loaded",file);
}
void SIMIX_ctx_lua_factory_init(smx_context_factory_t *factory) {

  *factory = xbt_new0(s_smx_context_factory_t, 1);

  (*factory)->create_context = smx_ctx_lua_create_context;
  (*factory)->finalize = smx_ctx_lua_factory_finalize;
  (*factory)->free = smx_ctx_lua_free;
  (*factory)->start = smx_ctx_lua_start;
  (*factory)->stop = smx_ctx_lua_stop;
  (*factory)->suspend = smx_ctx_lua_suspend;
  (*factory)->resume = smx_ctx_lua_resume;
  (*factory)->name = "smx_lua_context_factory";

  lua_state = lua_open();
  luaL_openlibs(lua_state);
  MSG_register(lua_state);
  INFO0("Lua Factory created");
}

static int smx_ctx_lua_factory_finalize(smx_context_factory_t * factory)
{
  lua_close(lua_state);

  free(*factory);
  *factory = NULL;
  return 0;
}

static smx_context_t 
smx_ctx_lua_create_context(xbt_main_func_t code, int argc, char** argv, 
                                    void_f_pvoid_t cleanup_func, void* cleanup_arg)
{
  smx_ctx_lua_t context = xbt_new0(s_smx_ctx_lua_t, 1);

  /* If the user provided a function for the process then use it
     otherwise is the context for maestro */
  if(code){
    context->code = code;
    context->state = lua_newthread(lua_state);

    context->ref = luaL_ref(lua_state, LUA_REGISTRYINDEX);
    //lua_pop(lua_state,1);

    context->argc = argc;
    context->argv = argv;
    context->cleanup_func = cleanup_func;
    context->cleanup_arg = cleanup_arg;
    INFO1("Created context for function %s",argv[0]);
  } else {
    INFO0("Created context for maestro");
  }
  
  return (smx_context_t)context;
}

static void smx_ctx_lua_free(smx_context_t pcontext)
{
  int i;
  smx_ctx_lua_t context = (smx_ctx_lua_t)pcontext;
  if (context){

    /* free argv */
    if (context->argv) {
      for (i = 0; i < context->argc; i++)
        if (context->argv[i])
          free(context->argv[i]);

      free(context->argv);
    }
    
    /* destroy the context */
    luaL_unref(lua_state,LUA_REGISTRYINDEX,context->ref );
    free(context);
  }
}

static void smx_ctx_lua_start(smx_context_t pcontext) {
  smx_ctx_lua_t context = (smx_ctx_lua_t)pcontext;
  INFO1("Starting '%s'",context->argv[0]);

  lua_getglobal(context->state,context->argv[0]);
  if(!lua_isfunction(context->state,-1)) {
    lua_pop(context->state,1);
    THROW1(arg_error,0,"The lua function %s does not seem to exist",context->argv[0]);
  }

  // push arguments onto the stack
  int i;
  for(i=1;i<context->argc;i++)
    lua_pushstring(context->state,context->argv[i]);

  // Call the function
  context->nargs = context->argc-1;
}

static void smx_ctx_lua_stop(smx_context_t pcontext) {
  smx_ctx_lua_t context = (smx_ctx_lua_t)pcontext;
  
  if (context->cleanup_func)
    (*context->cleanup_func) (context->cleanup_arg);

  // FIXME: DO it
}


static void smx_ctx_lua_suspend(smx_context_t context)
{
  INFO1("Suspending %s",context->argv[0]);
  lua_yield(((smx_ctx_lua_t)context)->state,0); // Should be the last line of the function
//  INFO1("Suspended %s",context->argv[0]);
}

static void 
smx_ctx_lua_resume(smx_context_t old_context, smx_context_t new_context) {
  smx_ctx_lua_t context = (smx_ctx_lua_t)new_context;

  INFO1("Resuming %s",context->argv[0]);
  lua_resume(context->state,context->nargs);
  context->nargs = 0;
  INFO1("Resumed %s",context->argv[0]);

//  INFO1("Process %s done ?",context->argv[0]);
//  lua_resume(((smx_ctx_lua_t)new_context)->state,0);
}
