/* $Id$ */

/* context_sysv - implementation of context switching with ucontextes from Sys V */

/* Copyright (c) 2004-2008 the SimGrid team. All right reserved */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"
#include "context_sysv_config.h"        /* loads context system definitions */
#include "portable.h"
#include <ucontext.h>           /* context relative declarations */

/* lower this if you want to reduce the memory consumption  */
#define STACK_SIZE 128*1024

#ifdef HAVE_VALGRIND_VALGRIND_H
#  include <valgrind/valgrind.h>
#endif /* HAVE_VALGRIND_VALGRIND_H */

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_context);

typedef struct s_smx_ctx_sysv {
  SMX_CTX_BASE_T;
  ucontext_t uc;                /* the thread that execute the code */
  char stack[STACK_SIZE];       /* the thread stack size */
  struct s_smx_ctx_sysv *prev;           /* the previous process */
#ifdef HAVE_VALGRIND_VALGRIND_H
  unsigned int valgrind_stack_id;       /* the valgrind stack id */
#endif                          
} s_smx_ctx_sysv_t, *smx_ctx_sysv_t;

static smx_context_t 
smx_ctx_sysv_factory_create_context(xbt_main_func_t code, int argc, char** argv, 
    void_f_pvoid_t cleanup_func, void* cleanup_arg);

static int smx_ctx_sysv_factory_finalize(smx_context_factory_t *factory);

static void smx_ctx_sysv_free(smx_context_t context);

static void smx_ctx_sysv_start(smx_context_t context);

static void smx_ctx_sysv_stop(smx_context_t context);

static void smx_ctx_sysv_suspend(smx_context_t context);

static void 
smx_ctx_sysv_resume(smx_context_t old_context, smx_context_t new_context);

static void smx_ctx_sysv_wrapper(void);

void SIMIX_ctx_sysv_factory_init(smx_context_factory_t *factory)
{
  *factory = xbt_new0(s_smx_context_factory_t, 1);

  (*factory)->create_context = smx_ctx_sysv_factory_create_context;
  (*factory)->finalize = smx_ctx_sysv_factory_finalize;
  (*factory)->free = smx_ctx_sysv_free;
  (*factory)->start = smx_ctx_sysv_start;
  (*factory)->stop = smx_ctx_sysv_stop;
  (*factory)->suspend = smx_ctx_sysv_suspend;
  (*factory)->resume = smx_ctx_sysv_resume;
  (*factory)->name = "smx_sysv_context_factory";
}

static int smx_ctx_sysv_factory_finalize(smx_context_factory_t * factory)
{
  free(*factory);
  *factory = NULL;
  return 0;
}

static smx_context_t 
smx_ctx_sysv_factory_create_context(xbt_main_func_t code, int argc, char** argv, 
    void_f_pvoid_t cleanup_func, void* cleanup_arg)
{
  smx_ctx_sysv_t context = xbt_new0(s_smx_ctx_sysv_t, 1);

  /* If the user provided a function for the process then use it
     otherwise is the context for maestro */
  if(code){
    context->code = code;

    xbt_assert2(getcontext(&(context->uc)) == 0,
        "Error in context saving: %d (%s)", errno, strerror(errno));

    context->uc.uc_link = NULL;

    context->uc.uc_stack.ss_sp =
        pth_skaddr_makecontext(context->stack, STACK_SIZE);

    context->uc.uc_stack.ss_size =
        pth_sksize_makecontext(context->stack, STACK_SIZE);

#ifdef HAVE_VALGRIND_VALGRIND_H
    context->valgrind_stack_id =
        VALGRIND_STACK_REGISTER(context->uc.uc_stack.ss_sp,
            ((char *) context->uc.uc_stack.ss_sp) +
            context->uc.uc_stack.ss_size);
#endif /* HAVE_VALGRIND_VALGRIND_H */

    context->argc = argc;
    context->argv = argv;
    context->cleanup_func = cleanup_func;
    context->cleanup_arg = cleanup_arg;
  }

  return (smx_context_t)context;
}

static void smx_ctx_sysv_free(smx_context_t pcontext)
{
  int i;
  smx_ctx_sysv_t context = (smx_ctx_sysv_t)pcontext;   
  if (context){

#ifdef HAVE_VALGRIND_VALGRIND_H
    VALGRIND_STACK_DEREGISTER(((smx_ctx_sysv_t) context)->valgrind_stack_id);
#endif /* HAVE_VALGRIND_VALGRIND_H */

    /* free argv */
    if (context->argv) {
      for (i = 0; i < context->argc; i++)
        if (context->argv[i])
          free(context->argv[i]);

      free(context->argv);
    }

    /* destroy the context */
    free(context);
  }
}

static void smx_ctx_sysv_start(smx_context_t context)
{  
  makecontext(&((smx_ctx_sysv_t)context)->uc, smx_ctx_sysv_wrapper, 0);
}

static void smx_ctx_sysv_stop(smx_context_t pcontext)
{
  smx_ctx_sysv_t context = (smx_ctx_sysv_t)pcontext;

  if (context->cleanup_func)
    (*context->cleanup_func) (context->cleanup_arg);

  smx_ctx_sysv_suspend(pcontext);
}

static void smx_ctx_sysv_wrapper()
{
  /*FIXME: I would like to avoid accessing simix_global to get the current
    context by passing it as an argument of the wrapper function. The problem
    is that this function is called from smx_ctx_sysv_start, and uses
    makecontext for calling it, and the stupid posix specification states that
    all the arguments of the function should be int(32 bits), making it useless
    in 64-bit architectures where pointers are 64 bit long.
   */
  smx_ctx_sysv_t context = 
      (smx_ctx_sysv_t)simix_global->current_process->context;

  (context->code) (context->argc, context->argv);

  smx_ctx_sysv_stop((smx_context_t)context);
}

static void smx_ctx_sysv_suspend(smx_context_t context)
{
  int rv;

  smx_ctx_sysv_t prev_context = ((smx_ctx_sysv_t) context)->prev;

  ((smx_ctx_sysv_t) context)->prev = NULL;

  rv = swapcontext(&((smx_ctx_sysv_t) context)->uc, &prev_context->uc);

  xbt_assert0((rv == 0), "Context swapping failure");
}

static void 
smx_ctx_sysv_resume(smx_context_t old_context, smx_context_t new_context)
{
  int rv;

  ((smx_ctx_sysv_t) new_context)->prev = (smx_ctx_sysv_t)old_context;

  rv = swapcontext(&((smx_ctx_sysv_t)old_context)->uc,
      &((smx_ctx_sysv_t)new_context)->uc);

  xbt_assert0((rv == 0), "Context swapping failure");
}
