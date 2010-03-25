/* context_lua - implementation of context switching with lua coroutines */

/* Copyright (c) 2010 the SimGrid team. All right reserved */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <lauxlib.h>
#include <lualib.h>

#include "smx_context_private.h"

/* We don't use that factory at all for now. This may change at some point,
 * to reduce the amount of memory per user thread in sysv
 */

lua_State *simgrid_lua_state;
void SIMIX_ctx_lua_factory_init(smx_context_factory_t *factory) {
}

#ifdef KILLME
/* lua can run with ultra tiny stacks since the user code lives in lua stacks, not the main one */
//#define CONTEXT_STACK_SIZE 4*1024

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(lua);


static smx_context_t 
smx_ctx_lua_create_context(xbt_main_func_t code, int argc, char** argv, 
    void_f_pvoid_t cleanup_func, void* cleanup_arg);

static int smx_ctx_lua_factory_finalize(smx_context_factory_t *factory);

static void smx_ctx_lua_free(smx_context_t context);
static void smx_ctx_lua_stop(smx_context_t context);
static void smx_ctx_lua_suspend(smx_context_t context);
static void smx_ctx_lua_resume(smx_context_t new_context);


void SIMIX_ctx_lua_factory_init(smx_context_factory_t *factory) {

  smx_ctx_base_factory_init(factory);

  (*factory)->create_context = smx_ctx_lua_create_context;
  /* don't override (*factory)->finalize */;
  (*factory)->free = smx_ctx_sysv_free;
  (*factory)->stop = smx_ctx_sysv_stop;
  (*factory)->suspend = smx_ctx_sysv_suspend;
  (*factory)->resume = smx_ctx_sysv_resume;
  (*factory)->name = "smx_lua_context_factory";

  INFO0("Lua Factory created");
}

static smx_context_t 
smx_ctx_lua_create_context(xbt_main_func_t code, int argc, char** argv, 
    void_f_pvoid_t cleanup_func, void* cleanup_arg) {

  return smx_ctx_sysv_create_context_sized(sizeof(s_smx_ctx_sysv_t),
      code,argc,argv,cleanup_func,cleanup_arg);
}
#if KILLME
static void smx_ctx_lua_free(smx_context_t context) {

  if (context){
    DEBUG1("smx_ctx_lua_free_context(%p)",context);

  }

  smx_ctx_sysv_free(context);
}

static void smx_ctx_lua_stop(smx_context_t pcontext) {
  DEBUG1("Stopping '%s' (nothing to do)",pcontext->argv[0]);
  smx_ctx_sysv_stop(pcontext);
}

static void smx_ctx_lua_suspend(smx_context_t pcontext) {
  smx_ctx_lua_t context = (smx_ctx_lua_t)pcontext;
  DEBUG1("Suspending '%s' (using sysv facilities)",context->super.super.argv[0]);
  smx_ctx_sysv_suspend(pcontext);

  DEBUG0("Back from call yielding");
}

static void 
smx_ctx_lua_resume(smx_context_t new_context) {
  smx_ctx_lua_t context = (smx_ctx_lua_t)new_context;
  DEBUG1("Resuming %s",context->super.super.argv[0]);
  smx_ctx_sysv_resume(new_context);
}
#endif

#endif
