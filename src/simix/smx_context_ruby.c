/* context_Ruby - implementation of context switching with/for ruby         */

/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/function_types.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/asserts.h"

#include "smx_context_private.h"
#include "bindings/ruby_bindings.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(ruby);

static smx_context_t
smx_ctx_ruby_create_context(xbt_main_func_t code, int argc, char **argv,
                            void_f_pvoid_t cleanup_func,
                            void *cleanup_arg);

static void smx_ctx_ruby_stop(smx_context_t context);
static void smx_ctx_ruby_suspend(smx_context_t context);
static void smx_ctx_ruby_resume(smx_context_t new_context);


void SIMIX_ctx_ruby_factory_init(smx_context_factory_t * factory)
{
  smx_ctx_base_factory_init(factory);

  (*factory)->create_context = smx_ctx_ruby_create_context;
  /* Do not overload that method (*factory)->finalize */
  /* Do not overload that method (*factory)->free */
  (*factory)->stop = smx_ctx_ruby_stop;
  (*factory)->suspend = smx_ctx_ruby_suspend;
  (*factory)->resume = smx_ctx_ruby_resume;
  (*factory)->name = "smx_ruby_context_factory";
  ruby_init();
  ruby_init_loadpath();
}

static smx_context_t
smx_ctx_ruby_create_context(xbt_main_func_t code, int argc, char **argv,
                            void_f_pvoid_t cleanup_func, void *cleanup_arg)
{

  smx_ctx_ruby_t context = (smx_ctx_ruby_t)
      smx_ctx_base_factory_create_context_sized(sizeof(s_smx_ctx_ruby_t),
                                                code, argc, argv,
                                                cleanup_func, cleanup_arg);

  /* if the user provided a function for the process , then use it
     Otherwise it's the context for maestro */
  if (code) {
    context->process = (VALUE) code;

    DEBUG1("smx_ctx_ruby_create_context(%s)...Done", argv[0]);
  }
  return (smx_context_t) context;
}

static void smx_ctx_ruby_stop(smx_context_t context)
{
  DEBUG0("smx_ctx_ruby_stop()");
  VALUE process = Qnil;
  smx_ctx_ruby_t ctx_ruby, current;

  smx_ctx_base_stop(context);

  ctx_ruby = (smx_ctx_ruby_t) context;

  if (simix_global->current_process->iwannadie) {
    if (ctx_ruby->process) {

      //if the Ruby Process still Alive ,let's Schedule it
      if (rb_process_isAlive(ctx_ruby->process)) {

        current = (smx_ctx_ruby_t) simix_global->current_process->context;
        rb_process_schedule(current->process);
        process = ctx_ruby->process;
        // interupt/kill The Ruby Process
        rb_process_kill_up(process);
      }
    }
  } else {

    if (ctx_ruby->process)
      ctx_ruby->process = Qnil;

  }
}

static void smx_ctx_ruby_suspend(smx_context_t context)
{

  DEBUG1("smx_ctx_ruby_suspend(%s)", context->argv[0]);
  smx_ctx_ruby_t ctx_ruby = (smx_ctx_ruby_t) context;
  if (ctx_ruby->process)
    rb_process_unschedule(ctx_ruby->process);
}

static void smx_ctx_ruby_resume(smx_context_t new_context)
{
  DEBUG1("smx_ctx_ruby_resume(%s)",
         (new_context->argc ? new_context->argv[0] : "maestro"));

  smx_ctx_ruby_t ctx_ruby = (smx_ctx_ruby_t) new_context;
  rb_process_schedule(ctx_ruby->process);

}
