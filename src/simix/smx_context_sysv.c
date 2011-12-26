/* context_sysv - context switching with ucontextes from System V           */

/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

 /* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdarg.h>

#include "xbt/parmap.h"
#include "simix/private.h"
#include "gras_config.h"
#include "context_sysv_config.h"        /* loads context system definitions */

#ifdef _XBT_WIN32
#  include <win32_ucontext.h>     /* context relative declarations */
#else
#  include <ucontext.h>           /* context relative declarations */
#endif

#ifdef HAVE_VALGRIND_VALGRIND_H
#  include <valgrind/valgrind.h>
#endif                          /* HAVE_VALGRIND_VALGRIND_H */

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_context);

typedef struct s_smx_ctx_sysv {
  s_smx_ctx_base_t super;       /* Fields of super implementation */
  ucontext_t uc;                /* the ucontext that executes the code */
#ifdef HAVE_VALGRIND_VALGRIND_H
  unsigned int valgrind_stack_id;       /* the valgrind stack id */
#endif
  char stack[0];                /* the thread stack (must remain the last element of the structure) */
} s_smx_ctx_sysv_t, *smx_ctx_sysv_t;

#ifdef CONTEXT_THREADS
static xbt_parmap_t sysv_parmap;
static ucontext_t* sysv_workers_stacks;        /* space to save the worker's stack in each thread */
static unsigned long sysv_threads_working;     /* number of threads that have started their work */
static xbt_os_thread_key_t sysv_worker_id_key; /* thread-specific storage for the thread id */
#endif
static unsigned long sysv_process_index = 0;   /* index of the next process to run in the
                                                * list of runnable processes */
static smx_ctx_sysv_t sysv_maestro_context;

static int smx_ctx_sysv_factory_finalize(smx_context_factory_t *factory);
static smx_context_t
smx_ctx_sysv_create_context_sized(size_t structure_size,
                                  xbt_main_func_t code, int argc,
                                  char **argv,
                                  void_pfn_smxprocess_t cleanup_func,
                                  void *data);
static void smx_ctx_sysv_free(smx_context_t context);
static smx_context_t
smx_ctx_sysv_create_context(xbt_main_func_t code, int argc, char **argv,
    void_pfn_smxprocess_t cleanup_func, void* data);

static void smx_ctx_sysv_wrapper(int count, ...);

static void smx_ctx_sysv_stop_serial(smx_context_t context);
static void smx_ctx_sysv_suspend_serial(smx_context_t context);
static void smx_ctx_sysv_resume_serial(smx_process_t first_process);
static void smx_ctx_sysv_runall_serial(void);

static void smx_ctx_sysv_stop_parallel(smx_context_t context);
static void smx_ctx_sysv_suspend_parallel(smx_context_t context);
static void smx_ctx_sysv_resume_parallel(smx_process_t first_process);
static void smx_ctx_sysv_runall_parallel(void);

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
  (*factory)->name = "smx_sysv_context_factory";

  if (SIMIX_context_is_parallel()) {
#ifdef CONTEXT_THREADS	/* To use parallel ucontexts a thread pool is needed */
    int nthreads = SIMIX_context_get_nthreads();
    sysv_parmap = xbt_parmap_new(nthreads, SIMIX_context_get_parallel_mode());
    sysv_workers_stacks = xbt_new(ucontext_t, nthreads);
    xbt_os_thread_key_create(&sysv_worker_id_key);
    (*factory)->stop = smx_ctx_sysv_stop_parallel;
    (*factory)->suspend = smx_ctx_sysv_suspend_parallel;
    (*factory)->runall = smx_ctx_sysv_runall_parallel;
#else
    THROWF(arg_error, 0, "No thread support for parallel context execution");
#endif
  } else {
    (*factory)->stop = smx_ctx_sysv_stop_serial;
    (*factory)->suspend = smx_ctx_sysv_suspend_serial;
    (*factory)->runall = smx_ctx_sysv_runall_serial;
  }    
}

static int smx_ctx_sysv_factory_finalize(smx_context_factory_t *factory)
{ 
#ifdef CONTEXT_THREADS
  if (sysv_parmap)
    xbt_parmap_destroy(sysv_parmap);
  xbt_free(sysv_workers_stacks);
#endif
  return smx_ctx_base_factory_finalize(factory);
}

static smx_context_t
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

  /* if the user provided a function for the process then use it,
     otherwise it is the context for maestro */
  if (code) {

    getcontext(&(context->uc));

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
  } else {
    sysv_maestro_context = context;
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

static void smx_ctx_sysv_free(smx_context_t context)
{

  if (context) {

#ifdef HAVE_VALGRIND_VALGRIND_H
    VALGRIND_STACK_DEREGISTER(((smx_ctx_sysv_t)
                               context)->valgrind_stack_id);
#endif                          /* HAVE_VALGRIND_VALGRIND_H */

  }
  smx_ctx_base_free(context);
}

static void smx_ctx_sysv_wrapper(int first, ...)
{ 
  union u_ctx_addr ctx_addr;
  smx_ctx_sysv_t context;

  ctx_addr.intv[0] = first;
  if (CTX_ADDR_LEN > 1) {
    va_list ap;
    int i;
    va_start(ap, first);
    for (i = 1; i < CTX_ADDR_LEN; i++)
      ctx_addr.intv[i] = va_arg(ap, int);
    va_end(ap);
  }
  context = ctx_addr.addr;
  (context->super.code) (context->super.argc, context->super.argv);

  simix_global->context_factory->stop((smx_context_t) context);
}

static void smx_ctx_sysv_stop_serial(smx_context_t context)
{
  smx_ctx_base_stop(context);
  smx_ctx_sysv_suspend_serial(context);
}

static void smx_ctx_sysv_suspend_serial(smx_context_t context)
{
  /* determine the next context */
  smx_context_t next_context;
  unsigned long int i = sysv_process_index++;

  if (i < xbt_dynar_length(simix_global->process_to_run)) {
    /* execute the next process */
    XBT_DEBUG("Run next process");
    next_context = xbt_dynar_get_as(
        simix_global->process_to_run,i, smx_process_t)->context;
  }
  else {
    /* all processes were run, return to maestro */
    XBT_DEBUG("No more process to run");
    next_context = (smx_context_t) sysv_maestro_context;
  }
  SIMIX_context_set_current(next_context);
  swapcontext(&((smx_ctx_sysv_t) context)->uc,
      &((smx_ctx_sysv_t) next_context)->uc);
}

static void smx_ctx_sysv_resume_serial(smx_process_t first_process)
{
  smx_context_t context = first_process->context;
  SIMIX_context_set_current(context);
  swapcontext(&sysv_maestro_context->uc,
      &((smx_ctx_sysv_t) context)->uc);
}

static void smx_ctx_sysv_runall_serial(void)
{
  if (!xbt_dynar_is_empty(simix_global->process_to_run)) {
    smx_process_t first_process =
        xbt_dynar_get_as(simix_global->process_to_run, 0, smx_process_t);
    sysv_process_index = 1;

    /* execute the first process */
    smx_ctx_sysv_resume_serial(first_process);
  }
}

static void smx_ctx_sysv_stop_parallel(smx_context_t context)
{
  smx_ctx_base_stop(context);
  smx_ctx_sysv_suspend_parallel(context);
}

static void smx_ctx_sysv_suspend_parallel(smx_context_t context)
{
#ifdef CONTEXT_THREADS
  /* determine the next context */
  smx_process_t next_work = xbt_parmap_next(sysv_parmap);
  smx_context_t next_context;
  ucontext_t* next_stack;

  if (next_work != NULL) {
    /* there is a next process to resume */
    XBT_DEBUG("Run next process");
    next_context = next_work->context;
    next_stack = &((smx_ctx_sysv_t) next_context)->uc;
  }
  else {
    /* all processes were run, go to the barrier */
    XBT_DEBUG("No more processes to run");
    next_context = (smx_context_t) sysv_maestro_context;
    unsigned long worker_id =
        (unsigned long) xbt_os_thread_get_specific(sysv_worker_id_key);
    next_stack = &sysv_workers_stacks[worker_id];
  }

  SIMIX_context_set_current(next_context);
  swapcontext(&((smx_ctx_sysv_t) context)->uc, next_stack);
#endif
}

static void smx_ctx_sysv_resume_parallel(smx_process_t first_process)
{
#ifdef CONTEXT_THREADS
  unsigned long worker_id = __sync_fetch_and_add(&sysv_threads_working, 1);
  xbt_os_thread_set_specific(sysv_worker_id_key, (void*) worker_id);
  ucontext_t* worker_stack = &sysv_workers_stacks[worker_id];

  smx_context_t context = first_process->context;
  SIMIX_context_set_current(context);
  swapcontext(worker_stack, &((smx_ctx_sysv_t) context)->uc);
#endif
}

static void smx_ctx_sysv_runall_parallel(void)
{
#ifdef CONTEXT_THREADS
  sysv_threads_working = 0;
  xbt_parmap_apply(sysv_parmap, (void_f_pvoid_t) smx_ctx_sysv_resume_parallel,
      simix_global->process_to_run);
#endif
}
