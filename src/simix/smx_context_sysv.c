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

#ifdef CONTEXT_THREADS
static xbt_parmap_t parmap;
#endif

static smx_context_t
smx_ctx_sysv_create_context(xbt_main_func_t code, int argc, char **argv,
    void_pfn_smxprocess_t cleanup_func, void* data);

static void smx_ctx_sysv_wrapper(int count, ...);

/* This is a bit paranoid about SIZEOF_VOIDP not being a multiple of SIZEOF_INT,
 * but it doesn't harm. */
#define CTX_ADDR_LEN (SIZEOF_VOIDP / SIZEOF_INT + !!(SIZEOF_VOIDP % SIZEOF_INT))
union u_ctx_addr {
  void *addr;
  int intv[CTX_ADDR_LEN];
};
#if (CTX_ADDR_LEN == 1)
#  define CTX_ADDR_SPLIT(u) (u).intv[0]
#elif (CTX_ADDR_LEN == 2)
#  define CTX_ADDR_SPLIT(u) (u).intv[0], (u).intv[1]
#else
#  error Your architecture is not supported yet
#endif

void SIMIX_ctx_sysv_factory_init(smx_context_factory_t *factory)
{
  smx_ctx_base_factory_init(factory);
  XBT_VERB("Activating SYSV context factory");

  (*factory)->finalize = smx_ctx_sysv_factory_finalize;
  (*factory)->create_context = smx_ctx_sysv_create_context;
  /* Do not overload that method (*factory)->finalize */
  (*factory)->free = smx_ctx_sysv_free;
  (*factory)->stop = smx_ctx_sysv_stop;
  (*factory)->suspend = smx_ctx_sysv_suspend;
  (*factory)->name = "smx_sysv_context_factory";

  if (SIMIX_context_is_parallel()) {
#ifdef CONTEXT_THREADS	/* To use parallel ucontexts a thread pool is needed */
    parmap = xbt_parmap_new(2);
    (*factory)->runall = smx_ctx_sysv_runall_parallel;
    (*factory)->self = smx_ctx_sysv_self_parallel;
    (*factory)->get_thread_id = smx_ctx_sysv_get_thread_id;
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
  union u_ctx_addr ctx_addr;
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
        pth_skaddr_makecontext(context->stack, smx_context_stack_size);

    context->uc.uc_stack.ss_size =
        pth_sksize_makecontext(context->stack, smx_context_stack_size);

#ifdef HAVE_VALGRIND_VALGRIND_H
    context->valgrind_stack_id =
        VALGRIND_STACK_REGISTER(context->uc.uc_stack.ss_sp,
                                ((char *) context->uc.uc_stack.ss_sp) +
                                context->uc.uc_stack.ss_size);
#endif                          /* HAVE_VALGRIND_VALGRIND_H */
    ctx_addr.addr = context;
    makecontext(&context->uc, (void (*)())smx_ctx_sysv_wrapper,
                CTX_ADDR_LEN, CTX_ADDR_SPLIT(ctx_addr));
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

  return smx_ctx_sysv_create_context_sized(sizeof(s_smx_ctx_sysv_t) + smx_context_stack_size,
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

void smx_ctx_sysv_wrapper(int first, ...)
{ 
  union u_ctx_addr ctx_addr;
  smx_ctx_sysv_t context;

  ctx_addr.intv[0] = first;
  if (CTX_ADDR_LEN > 1) {
    va_list ap;
    int i;
    va_start(ap, first);
    for(i = 1; i < CTX_ADDR_LEN; i++)
      ctx_addr.intv[i] = va_arg(ap, int);
    va_end(ap);
  }
  context = ctx_addr.addr;
  (context->super.code) (context->super.argc, context->super.argv);

  smx_ctx_sysv_stop((smx_context_t) context);
}

void smx_ctx_sysv_suspend(smx_context_t context)
{
  smx_current_context = (smx_context_t)maestro_context;
  int rv;
  rv = swapcontext(&((smx_ctx_sysv_t) context)->uc, &((smx_ctx_sysv_t)context)->old_uc);

  xbt_assert0((rv == 0), "Context swapping failure");
}

void smx_ctx_sysv_resume(smx_context_t context)
{
  smx_current_context = context; 
  int rv;
  rv = swapcontext(&((smx_ctx_sysv_t)context)->old_uc, &((smx_ctx_sysv_t) context)->uc);

  xbt_assert0((rv == 0), "Context swapping failure");
}

void smx_ctx_sysv_runall(xbt_dynar_t processes)
{
  smx_process_t process;
  unsigned int cursor;

  xbt_dynar_foreach(processes, cursor, process) {
    XBT_DEBUG("Schedule item %u of %lu",cursor,xbt_dynar_length(processes));
    smx_ctx_sysv_resume(process->context);
  }
  xbt_dynar_reset(processes);
}

void smx_ctx_sysv_resume_parallel(smx_process_t process)
{
  smx_context_t context = process->context;
  smx_current_context = (smx_context_t)context;
  int rv;
  rv = swapcontext(&((smx_ctx_sysv_t)context)->old_uc, &((smx_ctx_sysv_t) context)->uc);
  smx_current_context = (smx_context_t)maestro_context;

  xbt_assert0((rv == 0), "Context swapping failure");
}

void smx_ctx_sysv_runall_parallel(xbt_dynar_t processes)
{
  xbt_parmap_apply(parmap, (void_f_pvoid_t)smx_ctx_sysv_resume_parallel, processes);
  xbt_dynar_reset(processes);
}

smx_context_t smx_ctx_sysv_self_parallel(void)
{
  /*smx_context_t self_context = (smx_context_t) xbt_os_thread_get_extra_data();
  return self_context ? self_context : (smx_context_t) maestro_context;*/
  return smx_current_context;
}

int smx_ctx_sysv_get_thread_id(void)
{
  return (int)(unsigned long)xbt_os_thread_get_extra_data();
}
