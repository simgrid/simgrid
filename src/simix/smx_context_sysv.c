/* context_sysv - context switching with ucontextes from System V           */

/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

 /* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdarg.h>

#include "smx_context_sysv_private.h"
#include "xbt/parmap.h"
#include "simix/private.h"
#include "gras_config.h"

#ifdef HAVE_VALGRIND_VALGRIND_H
#  include <valgrind/valgrind.h>
#endif                          /* HAVE_VALGRIND_VALGRIND_H */

#ifdef _XBT_WIN32
#include "win32_ucontext.h"
#include "win32_ucontext.c"
#else
#include "ucontext.h"
#endif

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_context);

static xbt_parmap_t parmap;

static smx_context_t
smx_ctx_sysv_create_context(xbt_main_func_t code, int argc, char **argv,
    void_pfn_smxprocess_t cleanup_func, void* data);

static void smx_ctx_sysv_wrapper(int count, ...);

void SIMIX_ctx_sysv_factory_init(smx_context_factory_t *factory)
{
  smx_ctx_base_factory_init(factory);
  VERB0("Activating SYSV context factory");

  (*factory)->finalize = smx_ctx_sysv_factory_finalize;
  (*factory)->create_context = smx_ctx_sysv_create_context;
  /* Do not overload that method (*factory)->finalize */
  (*factory)->free = smx_ctx_sysv_free;
  (*factory)->stop = smx_ctx_sysv_stop;
  (*factory)->suspend = smx_ctx_sysv_suspend;
  (*factory)->name = "smx_sysv_context_factory";

  if (smx_parallel_contexts) {
#ifdef CONTEXT_THREADS	/* To use parallel ucontexts a thread pool is needed */
    parmap = xbt_parmap_new(2);
    (*factory)->runall = smx_ctx_sysv_runall_parallel;
    (*factory)->self = smx_ctx_sysv_self_parallel;
#else
    THROW0(arg_error, 0, "No thread support for parallel context execution");
#endif
  }else{
    (*factory)->runall = smx_ctx_sysv_runall;
  }    
}

int smx_ctx_sysv_factory_finalize(smx_context_factory_t *factory)
{ 
  if(parmap)
    xbt_parmap_destroy(parmap);
  return smx_ctx_base_factory_finalize(factory);
}

smx_context_t
smx_ctx_sysv_create_context_sized(size_t size, xbt_main_func_t code,
                                  int argc, char **argv,
                                  void_pfn_smxprocess_t cleanup_func,
                                  void *data)
{
  uintptr_t ctx_addr;
  smx_ctx_sysv_t context =
      (smx_ctx_sysv_t) smx_ctx_base_factory_create_context_sized(size,
                                                                 code,
                                                                 argc,
                                                                 argv,
                                                                 cleanup_func,
                                                                 data);

  /* If the user provided a function for the process then use it
     otherwise is the context for maestro */
  if (code) {

    xbt_assert2(getcontext(&(context->uc)) == 0,
                "Error in context saving: %d (%s)", errno,
                strerror(errno));

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
#endif                          /* HAVE_VALGRIND_VALGRIND_H */
    ctx_addr = (uintptr_t)context;
    makecontext(&((smx_ctx_sysv_t) context)->uc, (void (*)())smx_ctx_sysv_wrapper,
                SIZEOF_VOIDP / SIZEOF_INT + 1, SIZEOF_VOIDP / SIZEOF_INT,
#if (SIZEOF_VOIDP == SIZEOF_INT)
                (int)ctx_addr
#elif (SIZEOF_VOIDP == 2 * SIZEOF_INT)
                (int)(ctx_addr >> (8 * sizeof(int))),
                (int)(ctx_addr)
#else
#error Your architecture is not supported yet
#endif
   );
  }else{
    maestro_context = context;
  }

  return (smx_context_t) context;

}

static smx_context_t
smx_ctx_sysv_create_context(xbt_main_func_t code, int argc, char **argv,
    void_pfn_smxprocess_t cleanup_func,
    void *data)
{

  return smx_ctx_sysv_create_context_sized(sizeof(s_smx_ctx_sysv_t),
                                           code, argc, argv, cleanup_func,
                                           data);

}

void smx_ctx_sysv_free(smx_context_t context)
{

  if (context) {

#ifdef HAVE_VALGRIND_VALGRIND_H
    VALGRIND_STACK_DEREGISTER(((smx_ctx_sysv_t)
                               context)->valgrind_stack_id);
#endif                          /* HAVE_VALGRIND_VALGRIND_H */

  }
  smx_ctx_base_free(context);
}

void smx_ctx_sysv_stop(smx_context_t context)
{
  smx_ctx_base_stop(context);
  smx_ctx_sysv_suspend(context);
}

void smx_ctx_sysv_wrapper(int count, ...)
{ 
  uintptr_t ctx_addr = 0;
  va_list ap;
  smx_ctx_sysv_t context;
  int i;

  va_start(ap, count);
  for(i = 0; i < count; i++) {
     ctx_addr <<= 8*sizeof(int);
     ctx_addr |= (uintptr_t)va_arg(ap, int);
  }
  va_end(ap);
  context = (smx_ctx_sysv_t)ctx_addr;
  (context->super.code) (context->super.argc, context->super.argv);

  smx_ctx_sysv_stop((smx_context_t) context);
}

void smx_ctx_sysv_suspend(smx_context_t context)
{
  smx_current_context = (smx_context_t)maestro_context;
  int rv = swapcontext(&((smx_ctx_sysv_t) context)->uc, &((smx_ctx_sysv_t)context)->old_uc);

  xbt_assert0((rv == 0), "Context swapping failure");
}

void smx_ctx_sysv_resume(smx_context_t context)
{
  smx_current_context = context; 
  int rv = swapcontext(&((smx_ctx_sysv_t)context)->old_uc, &((smx_ctx_sysv_t) context)->uc);

  xbt_assert0((rv == 0), "Context swapping failure");
}

void smx_ctx_sysv_runall(xbt_dynar_t processes)
{
  smx_process_t process;
  unsigned int cursor;

  xbt_dynar_foreach(processes, cursor, process) {
    DEBUG2("Schedule item %u of %lu",cursor,xbt_dynar_length(processes));
    smx_ctx_sysv_resume(process->context);
  }
  xbt_dynar_reset(processes);
}

void smx_ctx_sysv_resume_parallel(smx_process_t process)
{
  smx_context_t context = process->context;
  xbt_os_thread_set_extra_data(context);
  int rv = swapcontext(&((smx_ctx_sysv_t)context)->old_uc, &((smx_ctx_sysv_t) context)->uc);
  xbt_os_thread_set_extra_data(NULL);

  xbt_assert0((rv == 0), "Context swapping failure");
}

void smx_ctx_sysv_runall_parallel(xbt_dynar_t processes)
{
  xbt_parmap_apply(parmap, (void_f_pvoid_t)smx_ctx_sysv_resume_parallel, processes);
  xbt_dynar_reset(processes);
}

smx_context_t smx_ctx_sysv_self_parallel(void)
{
  smx_context_t self_context = (smx_context_t) xbt_os_thread_get_extra_data();
  return self_context ? self_context : (smx_context_t) maestro_context;
}
