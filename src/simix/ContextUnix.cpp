/* Copyright (c) 2009-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* \file UContext.cpp Context switching with ucontexts from System V         */

#include <stdarg.h>

#include <functional>
#include <ucontext.h>           /* context relative declarations */

#include "xbt/parmap.h"
#include "src/simix/smx_private.h"
#include "src/simix/smx_private.hpp"
#include "src/internal_config.h"
#include "mc/mc.h"

/** Many integers are needed to store a pointer
 *
 * This is a bit paranoid about sizeof(smx_ctx_sysv_t) not being a multiple
 * of sizeof(int), but it doesn't harm. */
#define CTX_ADDR_LEN                            \
  (sizeof(void*) / sizeof(int) +       \
   !!(sizeof(void*) % sizeof(int)))

/** A better makecontext
 *
 * Makecontext expects integer arguments, we the context
 * variable is decomposed into a serie of integers and
 * each integer is passed as argument to makecontext. */
static void simgrid_makecontext(ucontext_t* ucp, void (*func)(int first, ...), void* arg)
{
  int ctx_addr[CTX_ADDR_LEN];
  memcpy(ctx_addr, &arg, sizeof(void*));
  switch (CTX_ADDR_LEN) {
  case 1:
    makecontext(ucp, (void (*)())func, 1, ctx_addr[0]);
    break;
  case 2:
    makecontext(ucp, (void (*)()) func, 2, ctx_addr[0], ctx_addr[1]);
    break;
  default:
    xbt_die("Ucontexts are not supported on this arch yet (addr len = %zu/%zu = %zu)", sizeof(void*), sizeof(int), CTX_ADDR_LEN);
  }
}

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_context);

namespace simgrid {
namespace simix {
  class UContext;
  class SerialUContext;
  class ParallelUContext;
  class UContextFactory;
}
}

#if HAVE_THREAD_CONTEXTS
static xbt_parmap_t sysv_parmap;
static simgrid::simix::ParallelUContext** sysv_workers_context;   /* space to save the worker's context in each thread */
static uintptr_t sysv_threads_working;     /* number of threads that have started their work */
static xbt_os_thread_key_t sysv_worker_id_key; /* thread-specific storage for the thread id */
#endif
static unsigned long sysv_process_index = 0;   /* index of the next process to run in the
                                                * list of runnable processes */
static simgrid::simix::UContext* sysv_maestro_context;
static bool sysv_parallel;

// The name of this function is currently hardcoded in the code (as string).
// Do not change it without fixing those references as well.
static void smx_ctx_sysv_wrapper(int first, ...);

namespace simgrid {
namespace simix {

class UContext : public Context {
protected:
  ucontext_t uc_;         /* the ucontext that executes the code */
  char *stack_ = nullptr; /* the thread stack */
public:
  friend UContextFactory;
  UContext(std::function<void()>  code,
    void_pfn_smxprocess_t cleanup_func, smx_process_t process);
  ~UContext();
};

class SerialUContext : public UContext {
public:
  SerialUContext(std::function<void()>  code,
      void_pfn_smxprocess_t cleanup_func, smx_process_t process)
    : UContext(std::move(code), cleanup_func, process)
  {}
  void stop() override;
  void suspend() override;
  void resume();
};

class ParallelUContext : public UContext {
public:
  ParallelUContext(std::function<void()>  code,
      void_pfn_smxprocess_t cleanup_func, smx_process_t process)
    : UContext(std::move(code), cleanup_func, process)
  {}
  void stop() override;
  void suspend() override;
  void resume();
};

class UContextFactory : public ContextFactory {
public:
  friend UContext;
  friend SerialUContext;
  friend ParallelUContext;

  UContextFactory();
  virtual ~UContextFactory();
  virtual Context* create_context(std::function<void()> code,
    void_pfn_smxprocess_t, smx_process_t process) override;
  void run_all() override;
};

XBT_PRIVATE ContextFactory* sysv_factory()
{
  XBT_VERB("Activating SYSV context factory");
  return new UContextFactory();
}

UContextFactory::UContextFactory() : ContextFactory("UContextFactory")
{
  if (SIMIX_context_is_parallel()) {
    sysv_parallel = true;
#if HAVE_THREAD_CONTEXTS  /* To use parallel ucontexts a thread pool is needed */
    int nthreads = SIMIX_context_get_nthreads();
    sysv_parmap = nullptr;
    sysv_workers_context = xbt_new(ParallelUContext*, nthreads);
    sysv_maestro_context = nullptr;
    xbt_os_thread_key_create(&sysv_worker_id_key);
#else
    THROWF(arg_error, 0, "No thread support for parallel context execution");
#endif
  } else {
    sysv_parallel = false;
  }
}

UContextFactory::~UContextFactory()
{
#if HAVE_THREAD_CONTEXTS
  if (sysv_parmap)
    xbt_parmap_destroy(sysv_parmap);
  xbt_free(sysv_workers_context);
#endif
}

/* This function is called by maestro at the beginning of a scheduling round to get all working threads executing some stuff
 * It is much easier to understand what happens if you see the working threads as bodies that swap their soul for the
 *    ones of the simulated processes that must run.
 */
void UContextFactory::run_all()
{
  if (sysv_parallel) {
    #if HAVE_THREAD_CONTEXTS
      sysv_threads_working = 0;
      // Parmap_apply ensures that every working thread get an index in the
      // process_to_run array (through an atomic fetch_and_add),
      //  and runs the smx_ctx_sysv_resume_parallel function on that index

      // We lazily create the parmap because the parmap creates context
      // with simix_global->context_factory (which might not be initialized
      // when bootstrapping):
      if (sysv_parmap == nullptr)
        sysv_parmap = xbt_parmap_new(
          SIMIX_context_get_nthreads(), SIMIX_context_get_parallel_mode());

      xbt_parmap_apply(sysv_parmap,
        [](void* arg) {
          smx_process_t process = (smx_process_t) arg;
          ParallelUContext* context = static_cast<ParallelUContext*>(process->context);
          context->resume();
        },
        simix_global->process_to_run);
    #else
      xbt_die("You asked for a parallel execution, but you don't have any threads.");
    #endif
  } else {
    // Serial:
      smx_process_t first_process =
          xbt_dynar_get_as(simix_global->process_to_run, 0, smx_process_t);
      sysv_process_index = 1;
      SerialUContext* context =
        static_cast<SerialUContext*>(first_process->context);
      context->resume();
  }
}

Context* UContextFactory::create_context(std::function<void()> code,
  void_pfn_smxprocess_t cleanup, smx_process_t process)
{
  if (sysv_parallel)
    return new_context<ParallelUContext>(std::move(code), cleanup, process);
  else
    return new_context<SerialUContext>(std::move(code), cleanup, process);
}

UContext::UContext(std::function<void()> code,
    void_pfn_smxprocess_t cleanup_func, smx_process_t process)
  : Context(std::move(code), cleanup_func, process)
{
  /* if the user provided a function for the process then use it, otherwise it is the context for maestro */
  if (has_code()) {
    this->stack_ = (char*) SIMIX_context_stack_new();
    getcontext(&this->uc_);
    this->uc_.uc_link = nullptr;
    this->uc_.uc_stack.ss_sp   = sg_makecontext_stack_addr(this->stack_);
    this->uc_.uc_stack.ss_size = sg_makecontext_stack_size(smx_context_usable_stack_size);
    simgrid_makecontext(&this->uc_, smx_ctx_sysv_wrapper, this);
  } else {
    if (process != NULL && sysv_maestro_context == NULL)
      sysv_maestro_context = this;
  }

#if HAVE_MC
  if (MC_is_active() && has_code()) {
    MC_register_stack_area(this->stack_, process,
                      &(this->uc_), smx_context_usable_stack_size);
  }
#endif
}

UContext::~UContext()
{
  SIMIX_context_stack_delete(this->stack_);
}

}
}

static void smx_ctx_sysv_wrapper(int first, ...)
{
  // Rebuild the Context* pointer from the integers:
  int ctx_addr[CTX_ADDR_LEN];
  simgrid::simix::UContext* context;
  ctx_addr[0] = first;
  if (CTX_ADDR_LEN > 1) {
    va_list ap;
    va_start(ap, first);
    for (unsigned i = 1; i < CTX_ADDR_LEN; i++)
      ctx_addr[i] = va_arg(ap, int);
    va_end(ap);
  }
  memcpy(&context, ctx_addr, sizeof(simgrid::simix::UContext*));

  (*context)();
  context->stop();
}

namespace simgrid {
namespace simix {

void SerialUContext::stop()
{
  Context::stop();
  this->suspend();
}

void SerialUContext::suspend()
{
  /* determine the next context */
  SerialUContext* next_context = nullptr;
  unsigned long int i = sysv_process_index++;

  if (i < xbt_dynar_length(simix_global->process_to_run)) {
    /* execute the next process */
    XBT_DEBUG("Run next process");
    next_context = (SerialUContext*) xbt_dynar_get_as(
        simix_global->process_to_run,i, smx_process_t)->context;
  }
  else {
    /* all processes were run, return to maestro */
    XBT_DEBUG("No more process to run");
    next_context = (SerialUContext*) sysv_maestro_context;
  }
  SIMIX_context_set_current(next_context);
  swapcontext(&this->uc_, &next_context->uc_);
}

// UContextSerial

void SerialUContext::resume()
{
  SIMIX_context_set_current(this);
  swapcontext(&((SerialUContext*)sysv_maestro_context)->uc_, &this->uc_);
}

void ParallelUContext::stop()
{
  UContext::stop();
  this->suspend();
}

/** Run one particular simulated process on the current thread. */
void ParallelUContext::resume()
{
#if HAVE_THREAD_CONTEXTS
  // What is my containing body?
  uintptr_t worker_id = __sync_fetch_and_add(&sysv_threads_working, 1);
  // Store the number of my containing body in os-thread-specific area :
  xbt_os_thread_set_specific(sysv_worker_id_key, (void*) worker_id);
  // Get my current soul:
  ParallelUContext* worker_context = (ParallelUContext*) SIMIX_context_self();
  // Write down that this soul is hosted in that body (for now)
  sysv_workers_context[worker_id] = worker_context;
  // Retrieve the system-level info that fuels this soul:
  ucontext_t* worker_stack = &((ParallelUContext*) worker_context)->uc_;
  // Write in simix that I switched my soul
  SIMIX_context_set_current(this);
   // Actually do that using the relevant library call:
  swapcontext(worker_stack, &this->uc_);
  // No body runs that soul anymore at this point.
  // Instead the current body took the soul of simulated process
  // The simulated process wakes back after the call to
  // "SIMIX_context_suspend(self->context);" within
  // smx_process.c::SIMIX_process_yield()

  // From now on, the simulated processes will change their
  // soul with the next soul to execute (in suspend_parallel, below).
  // When nobody is to be executed in this scheduling round,
  // the last simulated process will take back the initial
  // soul of the current working thread
#endif
}

/** Yield
 *
 * This function is called when a simulated process wants to yield back
 * to the maestro in a blocking simcall. This naturally occurs within
 * SIMIX_context_suspend(self->context), called from SIMIX_process_yield()
 * Actually, it does not really yield back to maestro, but into the next
 * process that must be executed. If no one is to be executed, then it
 * yields to the initial soul that was in this working thread (that was
 * saved in resume_parallel).
 */
void ParallelUContext::suspend()
{
#if HAVE_THREAD_CONTEXTS
  /* determine the next context */
  // Get the next soul to embody now:
  smx_process_t next_work = (smx_process_t) xbt_parmap_next(sysv_parmap);
  ParallelUContext* next_context = nullptr;
  // Will contain the next soul to run, either simulated or initial minion's one
  ucontext_t* next_stack;

  if (next_work != NULL) {
    // There is a next soul to embody (ie, a next process to resume)
    XBT_DEBUG("Run next process");
    next_context = (ParallelUContext*) next_work->context;
  }
  else {
    // All processes were run, go to the barrier
    XBT_DEBUG("No more processes to run");
    // Get back the identity of my body that was stored when starting
    // the scheduling round
    uintptr_t worker_id =
        (uintptr_t) xbt_os_thread_get_specific(sysv_worker_id_key);
    // Deduce the initial soul of that body
    next_context = (ParallelUContext*) sysv_workers_context[worker_id];
    // When given that soul, the body will wait for the next scheduling round
  }

  next_stack = &next_context->uc_;

  SIMIX_context_set_current(next_context);
  // Get that next soul:
  swapcontext(&this->uc_, next_stack);
#endif
}

}
}

