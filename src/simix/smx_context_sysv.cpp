/* context_sysv - context switching with ucontexts from System V           */

/* Copyright (c) 2009-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdarg.h>

#include "xbt/parmap.h"
#include "smx_private.h"
#include "src/internal_config.h"
#include "src/context_sysv_config.h"        /* loads context system definitions */
#include "mc/mc.h"

#ifdef _XBT_WIN32
#  include <xbt/win32_ucontext.h>     /* context relative declarations */
#else
#  include <ucontext.h>           /* context relative declarations */
#endif

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_context);

typedef struct s_smx_ctx_sysv {
  s_smx_ctx_base_t super;       /* Fields of super implementation */
  ucontext_t uc;                /* the ucontext that executes the code */
  char *stack;                  /* the thread stack */
} s_smx_ctx_sysv_t, *smx_ctx_sysv_t;

#ifdef CONTEXT_THREADS
static xbt_parmap_t sysv_parmap;
static smx_ctx_sysv_t* sysv_workers_context;   /* space to save the worker's context in each thread */
static unsigned long sysv_threads_working;     /* number of threads that have started their work */
static xbt_os_thread_key_t sysv_worker_id_key; /* thread-specific storage for the thread id */
#endif
static unsigned long sysv_process_index = 0;   /* index of the next process to run in the
                                                * list of runnable processes */
static smx_ctx_sysv_t sysv_maestro_context;

static int smx_ctx_sysv_factory_finalize(smx_context_factory_t *factory);
static void smx_ctx_sysv_free(smx_context_t context);
static smx_context_t
smx_ctx_sysv_create_context(xbt_main_func_t code, int argc, char **argv,
    void_pfn_smxprocess_t cleanup_func, smx_process_t process);

static void smx_ctx_sysv_wrapper(int count, ...);

static void smx_ctx_sysv_stop_serial(smx_context_t context);
static void smx_ctx_sysv_suspend_serial(smx_context_t context);
static void smx_ctx_sysv_resume_serial(smx_process_t first_process);
static void smx_ctx_sysv_runall_serial(void);

static void smx_ctx_sysv_stop_parallel(smx_context_t context);
static void smx_ctx_sysv_suspend_parallel(smx_context_t context);
static void smx_ctx_sysv_resume_parallel(smx_process_t first_process);
static void smx_ctx_sysv_runall_parallel(void);

/* This is a bit paranoid about sizeof(smx_ctx_sysv_t) not being a multiple of
 * sizeof(int), but it doesn't harm. */
#define CTX_ADDR_LEN                            \
  (sizeof(smx_ctx_sysv_t) / sizeof(int) +       \
   !!(sizeof(smx_ctx_sysv_t) % sizeof(int)))

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
#ifdef CONTEXT_THREADS  /* To use parallel ucontexts a thread pool is needed */
    int nthreads = SIMIX_context_get_nthreads();
    sysv_parmap = xbt_parmap_new(nthreads, SIMIX_context_get_parallel_mode());
    sysv_workers_context = xbt_new(smx_ctx_sysv_t, nthreads);
    sysv_maestro_context = NULL;
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
  xbt_free(sysv_workers_context);
#endif
  return smx_ctx_base_factory_finalize(factory);
}

static smx_context_t
smx_ctx_sysv_create_context(xbt_main_func_t code, int argc, char **argv,
                            void_pfn_smxprocess_t cleanup_func,
                            smx_process_t process)
{
  int ctx_addr[CTX_ADDR_LEN];
  smx_ctx_sysv_t context =
    (smx_ctx_sysv_t) smx_ctx_base_factory_create_context_sized(
      sizeof(s_smx_ctx_sysv_t),
      code,
      argc,
      argv,
      cleanup_func,
      process);

  /* if the user provided a function for the process then use it,
     otherwise it is the context for maestro */
  if (code) {

    context->stack = (char*) SIMIX_context_stack_new();
    getcontext(&(context->uc));

    context->uc.uc_link = NULL;

    context->uc.uc_stack.ss_sp =
        pth_skaddr_makecontext(context->stack, smx_context_usable_stack_size);

    context->uc.uc_stack.ss_size =
        pth_sksize_makecontext(context->stack, smx_context_usable_stack_size);

    memcpy(ctx_addr, &context, sizeof(smx_ctx_sysv_t));
    switch (CTX_ADDR_LEN) {
    case 1:
      makecontext(&context->uc, (void (*)())smx_ctx_sysv_wrapper,
                  1, ctx_addr[0]);
      break;
    case 2:
      makecontext(&context->uc, (void (*)())smx_ctx_sysv_wrapper,
                  2, ctx_addr[0], ctx_addr[1]);
      break;
    default:
      xbt_die("Ucontexts are not supported on this arch yet (addr len = %zu/%zu = %zu)",
              sizeof(smx_ctx_sysv_t), sizeof(int), CTX_ADDR_LEN);
    }
  } else {
    if (process != NULL && sysv_maestro_context == NULL)
      sysv_maestro_context = context;
  }

#ifdef HAVE_MC
  if (MC_is_active() && code) {
    MC_register_stack_area(context->stack, ((smx_context_t)context)->process,
                      &(context->uc), smx_context_usable_stack_size);
  }
#endif

  return (smx_context_t) context;
}

static void smx_ctx_sysv_free(smx_context_t context)
{

  if (context) {
    SIMIX_context_stack_delete(((smx_ctx_sysv_t)context)->stack);
  }
  smx_ctx_base_free(context);
}

static void smx_ctx_sysv_wrapper(int first, ...)
{ 
  int ctx_addr[CTX_ADDR_LEN];
  smx_ctx_sysv_t context;

  ctx_addr[0] = first;
  if (CTX_ADDR_LEN > 1) {
    va_list ap;
    va_start(ap, first);
    for (unsigned i = 1; i < CTX_ADDR_LEN; i++)
      ctx_addr[i] = va_arg(ap, int);
    va_end(ap);
  }
  memcpy(&context, ctx_addr, sizeof(smx_ctx_sysv_t));
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
  smx_process_t first_process =
      xbt_dynar_get_as(simix_global->process_to_run, 0, smx_process_t);
  sysv_process_index = 1;

  /* execute the first process */
  smx_ctx_sysv_resume_serial(first_process);
}

static void smx_ctx_sysv_stop_parallel(smx_context_t context)
{
  smx_ctx_base_stop(context);
  smx_ctx_sysv_suspend_parallel(context);
}

/* This function is called by maestro at the beginning of a scheduling round to get all working threads executing some stuff
 * It is much easier to understand what happens if you see the working threads as bodies that swap their soul for the
 *    ones of the simulated processes that must run.
 */
static void smx_ctx_sysv_runall_parallel(void) {
#ifdef CONTEXT_THREADS
  sysv_threads_working = 0;
  // parmap_apply ensures that every working thread get an index in the process_to_run array (through an atomic fetch_and_add),
  //  and runs the smx_ctx_sysv_resume_parallel function on that index
  xbt_parmap_apply(sysv_parmap, (void_f_pvoid_t) smx_ctx_sysv_resume_parallel,
      simix_global->process_to_run);
#else
  xbt_die("You asked for a parallel execution, but you don't have any threads.");
#endif
}

/* This function is in charge of running one particular simulated process on the current thread */
static void smx_ctx_sysv_resume_parallel(smx_process_t simulated_process_to_run) {
#ifdef CONTEXT_THREADS
  unsigned long worker_id = __sync_fetch_and_add(&sysv_threads_working, 1); // what is my containing body?
  xbt_os_thread_set_specific(sysv_worker_id_key, (void*) worker_id);        // Store the number of my containing body in os-thread-specific area
  smx_ctx_sysv_t worker_context = (smx_ctx_sysv_t)SIMIX_context_self();     // get my current soul
  sysv_workers_context[worker_id] = worker_context;                         // write down that this soul is hosted in that body (for now)
  ucontext_t* worker_stack = &worker_context->uc;                           // retrieves the system-level info that fuels this soul

  smx_context_t context = simulated_process_to_run->context;                // That's the first soul that I should become
  SIMIX_context_set_current(context);                                       // write in simix that I switched my soul
  swapcontext(worker_stack, &((smx_ctx_sysv_t) context)->uc);               // actually do that using the relevant syscall
  // No body runs that soul anymore at this point. Instead the current body took the soul of simulated process
  // The simulated process wakes back after the call to "SIMIX_context_suspend(self->context);" within smx_process.c::SIMIX_process_yield()

  // From now on, the simulated processes will change their soul with the next soul to execute (in suspend_parallel, below).
  // When nobody is to be executed in this scheduling round, the last simulated process will take back the initial soul of the current working thread
#endif
}

/* This function is called when a simulated process wants to yield back to the maestro in a blocking simcall.
 *    This naturally occurs within SIMIX_context_suspend(self->context), called from SIMIX_process_yield()
 * Actually, it does not really yield back to maestro, but into the next process that must be executed.
 * If no one is to be executed, then it yields to the initial soul that was in this working thread (that was saved in resume_parallel).
 */
static void smx_ctx_sysv_suspend_parallel(smx_context_t context) {
#ifdef CONTEXT_THREADS
  /* determine the next context */
  smx_process_t next_work = (smx_process_t) xbt_parmap_next(sysv_parmap);  // get the next soul to embody now
  smx_context_t next_context;
  ucontext_t* next_stack;                                  // will contain the next soul to run, either simulated or initial minion's one

  if (next_work != NULL) {                                 // there is a next soul to embody (ie, a next process to resume)
    XBT_DEBUG("Run next process");
    next_context = next_work->context;
  }
  else {
    /* all processes were run, go to the barrier */
    XBT_DEBUG("No more processes to run");
    unsigned long worker_id =                             // Get back the identity of my body that was stored when starting the scheduling round
        (unsigned long) xbt_os_thread_get_specific(sysv_worker_id_key);
    next_context = (smx_context_t)sysv_workers_context[worker_id];  // deduce the initial soul of that body
                                                                    // When given that soul, the body will wait for the next scheduling round
  }

  next_stack = &((smx_ctx_sysv_t)next_context)->uc;

  SIMIX_context_set_current(next_context);
  swapcontext(&((smx_ctx_sysv_t) context)->uc, next_stack);  // get that next soul
#endif
}
