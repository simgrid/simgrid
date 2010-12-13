/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <setjmp.h>
#include "replay.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(replay);
typedef struct s_statem_context {
  s_smx_ctx_base_t base;
  void *user_data;
  jmp_buf jump;
  int syscall_id; /* Identifier of previous syscall to SIMIX */
} s_statem_context_t,*statem_context_t;

static smx_context_t
statem_create_context(xbt_main_func_t code, int argc, char **argv,
    void_pfn_smxprocess_t cleanup_func, void *data);

static void statem_ctx_free(smx_context_t context);
static void statem_ctx_suspend(smx_context_t context);
static void statem_ctx_resume(smx_context_t new_context);
static void statem_ctx_runall(xbt_dynar_t processes);
static smx_context_t statem_ctx_self(void);

void statem_factory_init(smx_context_factory_t * factory) {
  /* instantiate the context factory */
  smx_ctx_base_factory_init(factory);

  (*factory)->create_context = statem_create_context;
  (*factory)->free = statem_ctx_free;
  (*factory)->suspend = statem_ctx_suspend;
  (*factory)->runall = statem_ctx_runall;
  (*factory)->name = "State Machine context factory";
}

static smx_context_t
statem_create_context(xbt_main_func_t code, int argc, char **argv,
    void_pfn_smxprocess_t cleanup_func, void *data){
  if(argc>0 && !replay_func_init) {
    fprintf(stderr,"variable replay_initer_function not set in replay_create_context. Severe issue!");
  }

  statem_context_t res = (statem_context_t)
  smx_ctx_base_factory_create_context_sized(sizeof(s_statem_context_t),
      code, argc, argv,
      cleanup_func,
      data);

  if (argc>0) /* not maestro */
    res->user_data = replay_func_init(argc,argv);
  return (smx_context_t)res;
}

static void statem_ctx_free(smx_context_t context) {
  if (replay_func_fini)
    replay_func_fini(((statem_context_t)context)->user_data);
  else if (((statem_context_t)context)->user_data)
    free(((statem_context_t)context)->user_data);

  smx_ctx_base_free(context);
}
/*static void replay_ctx_stop(smx_context_t context) {
  THROW_UNIMPLEMENTED;
}*/
static void statem_ctx_suspend(smx_context_t context) {
  statem_context_t ctx = (statem_context_t)context;
  ctx->syscall_id = SIMIX_request_last_id();
  longjmp(ctx->jump,1);
}
static void statem_ctx_resume(smx_context_t new_context) {
  THROW_UNIMPLEMENTED;
}
static void statem_ctx_runall(xbt_dynar_t processes) {
  smx_context_t old_context;
  smx_process_t process;
  unsigned int cursor;

  INFO0("Run all");

  xbt_dynar_foreach(processes, cursor, process) {
    statem_context_t ctx = (statem_context_t)SIMIX_process_get_context(process);
    old_context = smx_current_context;
    smx_current_context = SIMIX_process_get_context(process);
    if (!setjmp(ctx->jump))
      replay_func_run(((statem_context_t)smx_current_context)->user_data,
          ctx->syscall_id==0?NULL:SIMIX_request_get_result(ctx->syscall_id));
    smx_current_context = old_context;
  }
  xbt_dynar_reset(processes);
}

