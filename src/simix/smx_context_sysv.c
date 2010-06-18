/* context_sysv - context switching with ucontextes from System V           */

/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

 /* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */



#include "smx_context_sysv_private.h"

#ifdef HAVE_VALGRIND_VALGRIND_H
#  include <valgrind/valgrind.h>
#endif /* HAVE_VALGRIND_VALGRIND_H */

#ifdef _XBT_WIN32
	#include "ucontext.h"
	#include "ucontext.c"
#endif

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_context);

static smx_context_t 
smx_ctx_sysv_create_context(xbt_main_func_t code, int argc, char** argv,
    void_f_pvoid_t cleanup_func, void* cleanup_arg);


static void smx_ctx_sysv_wrapper(void);

void SIMIX_ctx_sysv_factory_init(smx_context_factory_t *factory) {

  smx_ctx_base_factory_init(factory);

  (*factory)->create_context = smx_ctx_sysv_create_context;
  /* Do not overload that method (*factory)->finalize */
  (*factory)->free = smx_ctx_sysv_free;
  (*factory)->stop = smx_ctx_sysv_stop;
  (*factory)->suspend = smx_ctx_sysv_suspend;
  (*factory)->resume = smx_ctx_sysv_resume;
  (*factory)->name = "smx_sysv_context_factory";
}

smx_context_t
smx_ctx_sysv_create_context_sized(size_t size, xbt_main_func_t code, int argc, char** argv,
    void_f_pvoid_t cleanup_func, void* cleanup_arg) {

  smx_ctx_sysv_t context = (smx_ctx_sysv_t)smx_ctx_base_factory_create_context_sized
      (size, code,argc,argv,cleanup_func,cleanup_arg);

  /* If the user provided a function for the process then use it
     otherwise is the context for maestro */
  if(code){

    xbt_assert2(getcontext(&(context->uc)) == 0,
        "Error in context saving: %d (%s)", errno, strerror(errno));

    context->uc.uc_link = NULL;

    context->uc.uc_stack.ss_sp =
        pth_skaddr_makecontext(context->stack, CONTEXT_STACK_SIZE);

    context->uc.uc_stack.ss_size =
        pth_sksize_makecontext(context->stack, CONTEXT_STACK_SIZE);

#ifdef HAVE_VALGRIND_VALGRIND_H
    context->valgrind_stack_id =
        VALGRIND_STACK_REGISTER(context->uc.uc_stack.ss_sp,
            ((char *) context->uc.uc_stack.ss_sp) +
            context->uc.uc_stack.ss_size);
#endif /* HAVE_VALGRIND_VALGRIND_H */

    makecontext(&((smx_ctx_sysv_t)context)->uc, smx_ctx_sysv_wrapper, 0);
  }

  return (smx_context_t)context;

}

static smx_context_t
smx_ctx_sysv_create_context(xbt_main_func_t code, int argc, char** argv,
    void_f_pvoid_t cleanup_func, void* cleanup_arg) {

  return smx_ctx_sysv_create_context_sized(sizeof(s_smx_ctx_sysv_t),
      code,argc,argv,cleanup_func,cleanup_arg);

}

void smx_ctx_sysv_free(smx_context_t context) {

  if (context){

#ifdef HAVE_VALGRIND_VALGRIND_H
    VALGRIND_STACK_DEREGISTER(((smx_ctx_sysv_t) context)->valgrind_stack_id);
#endif /* HAVE_VALGRIND_VALGRIND_H */

  }
  smx_ctx_base_free(context);
}

void smx_ctx_sysv_stop(smx_context_t context) {
  smx_ctx_base_stop(context);

  smx_ctx_sysv_suspend(context);
}

void smx_ctx_sysv_wrapper() {
  /*FIXME: I would like to avoid accessing simix_global to get the current
    context by passing it as an argument of the wrapper function. The problem
    is that this function is called from smx_ctx_sysv_start, and uses
    makecontext for calling it, and the stupid posix specification states that
    all the arguments of the function should be int(32 bits), making it useless
    in 64-bit architectures where pointers are 64 bit long.
   */
  smx_ctx_sysv_t context = 
      (smx_ctx_sysv_t)simix_global->current_process->context;

  (context->super.code) (context->super.argc, context->super.argv);

  smx_ctx_sysv_stop((smx_context_t)context);
}

void smx_ctx_sysv_suspend(smx_context_t context) {
  ucontext_t maestro_ctx = ((smx_ctx_sysv_t)simix_global->maestro_process->context)->uc;

  int rv = swapcontext(&((smx_ctx_sysv_t) context)->uc, &maestro_ctx);

  xbt_assert0((rv == 0), "Context swapping failure");
}

void smx_ctx_sysv_resume(smx_context_t new_context) {
  smx_ctx_sysv_t maestro = (smx_ctx_sysv_t)simix_global->maestro_process->context;

  int rv = swapcontext(&(maestro->uc), &((smx_ctx_sysv_t)new_context)->uc);

  xbt_assert0((rv == 0), "Context swapping failure");
}
