/* $Id$ */

/* context_lua - implementation of context switching with lua coroutines */

/* Copyright (c) 2004-2008 the SimGrid team. All right reserved */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
//#include "context_sysv_config.h"        /* loads context system definitions */
//#include "portable.h"
//#include <ucontext.h>           /* context relative declarations */
#include <lua5.1/lauxlib.h>
#include <lua5.1/lualib.h>

/* lower this if you want to reduce the memory consumption  */
//#define STACK_SIZE 128*1024

//#ifdef HAVE_VALGRIND_VALGRIND_H
//#  include <valgrind/valgrind.h>
//#endif /* HAVE_VALGRIND_VALGRIND_H */

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(lua);

typedef struct s_smx_ctx_sysv {
  SMX_CTX_BASE_T;
#ifdef KILLME
  /* Ucontext info */
  ucontext_t uc;                /* the thread that execute the code */
  char stack[STACK_SIZE];       /* the thread stack size */
  struct s_smx_ctx_sysv *prev;           /* the previous process */
#ifdef HAVE_VALGRIND_VALGRIND_H
  unsigned int valgrind_stack_id;       /* the valgrind stack id */
#endif
#endif /* KILLME */

  /* lua state info */
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
static void smx_ctx_lua_stop(smx_context_t context);
static void smx_ctx_lua_suspend(smx_context_t context);

static void 
smx_ctx_lua_resume(smx_context_t old_context, smx_context_t new_context);

static void smx_ctx_sysv_wrapper(void);

/* Actually, the parameter is a lua_State*, but it got anonymized because that function
 * is defined in a global header where lua may not be defined */
void SIMIX_ctx_lua_factory_set_state(void* state) {
  lua_state = state;
}
void SIMIX_ctx_lua_factory_init(smx_context_factory_t *factory) {

  *factory = xbt_new0(s_smx_context_factory_t, 1);

  (*factory)->create_context = smx_ctx_lua_create_context;
  (*factory)->finalize = smx_ctx_lua_factory_finalize;
  (*factory)->free = smx_ctx_lua_free;
  (*factory)->stop = smx_ctx_lua_stop;
  (*factory)->suspend = smx_ctx_lua_suspend;
  (*factory)->resume = smx_ctx_lua_resume;
  (*factory)->name = "smx_lua_context_factory";

  INFO0("Lua Factory created");
}

static int smx_ctx_lua_factory_finalize(smx_context_factory_t * factory) {
  lua_close(lua_state);

  free(*factory);
  *factory = NULL;
  return 0;
}

static smx_context_t 
smx_ctx_lua_create_context(xbt_main_func_t code, int argc, char** argv, 
    void_f_pvoid_t cleanup_func, void* cleanup_arg) {

  smx_ctx_lua_t context = xbt_new0(s_smx_ctx_lua_t, 1);

  /* If the user provided a function for the process then use it
     otherwise is the context for maestro */
  if (code){
    context->code = code;

    context->argc = argc;
    context->argv = argv;
    context->cleanup_func = cleanup_func;
    context->cleanup_arg = cleanup_arg;
    INFO1("Created context for function %s",argv[0]);

    /* start the coroutine in charge of running that code */
    context->state = lua_newthread(lua_state);
    context->ref = luaL_ref(lua_state, LUA_REGISTRYINDEX); // protect the thread from being garbage collected

    /* Start the co-routine */
    lua_getglobal(context->state,context->argv[0]);
    xbt_assert1(lua_isfunction(context->state,-1),
        "The lua function %s does not seem to exist",context->argv[0]);

    // push arguments onto the stack
    int i;
    for(i=1;i<context->argc;i++)
      lua_pushstring(context->state,context->argv[i]);

    // Call the function (in resume)
    context->nargs = context->argc-1;

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
    DEBUG1("smx_ctx_lua_free_context(%p)",context);

    /* free argv */
    if (context->argv) {
      for (i = 0; i < context->argc; i++)
        if (context->argv[i])
          free(context->argv[i]);

      free(context->argv);
    }

    /* let the lua garbage collector reclaim the thread used for the coroutine */
    luaL_unref(lua_state,LUA_REGISTRYINDEX,context->ref );

    free(context);
    context = NULL;
  }
}

static void smx_ctx_lua_stop(smx_context_t pcontext) {
  smx_ctx_lua_t context = (smx_ctx_lua_t)pcontext;

  INFO1("Stopping '%s' (nothing to do)",context->argv[0]);
  if (context->cleanup_func)
    (*context->cleanup_func) (context->cleanup_arg);

//  smx_ctx_lua_suspend(pcontext);
}

static void smx_ctx_lua_suspend(smx_context_t pcontext) {
  smx_ctx_lua_t context = (smx_ctx_lua_t)pcontext;
  DEBUG1("Suspending '%s' (calling lua_yield)",context->argv[0]);
  //lua_yield(context->state,0);

  lua_getglobal(context->state,"doyield");
  xbt_assert0(lua_isfunction(context->state,-1),
      "Cannot find the coroutine.yield function...");
  INFO0("Call coroutine.yield");
  lua_call(context->state,0,0);
  INFO0("Back from call to coroutine.yield");
}

static void 
smx_ctx_lua_resume(smx_context_t old_context, smx_context_t new_context) {
  smx_ctx_lua_t context = (smx_ctx_lua_t)new_context;
  DEBUG1("Resuming %s",context->argv[0]);
  int ret = lua_resume(context->state,context->nargs);
  INFO3("Function %s yielded back with value %d %s",context->argv[0],ret,(ret==LUA_YIELD?"(ie, LUA_YIELD)":""));
  if (lua_isstring(context->state,-1))
    INFO2("Result of %s seem to be '%s'",context->argv[0],luaL_checkstring(context->state,-1));
  context->nargs=0;
}
