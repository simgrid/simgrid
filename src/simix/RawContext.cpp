/* Copyright (c) 2009-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** \file RawContext.cpp 
  * Fast context switching inspired from SystemV ucontexts.
  *
  * In contrast to System V context, it does not touch the signal mask
  * which avoids making a system call (at least on Linux).
  */

#include <math.h>

#include <utility>
#include <functional>

#include "src/internal_config.h" 

#include "xbt/log.h"
#include "xbt/parmap.h"
#include "xbt/dynar.h"

#include "smx_private.h"
#include "smx_private.hpp"
#include "mc/mc.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_context);

// ***** Class definitions

namespace simgrid {
namespace simix {

class RawContext;
class RawContextFactory;

class RawContext : public Context {
protected:
  void* stack_ = nullptr; 
  /** pointer to top the stack stack */
  void* stack_top_ = nullptr;
public:
  friend class RawContextFactory;
  RawContext(std::function<void()> code,
          void_pfn_smxprocess_t cleanup_func,
          smx_process_t process);
  ~RawContext();
public:
  static void wrapper(void* arg);
  void stop() override;
  void suspend() override;
  void resume();
private:
  void suspend_serial();
  void suspend_parallel();
  void resume_serial();
  void resume_parallel();
};

class RawContextFactory : public ContextFactory {
public:
  RawContextFactory();
  ~RawContextFactory();
  RawContext* create_context(std::function<void()> code,
    void_pfn_smxprocess_t, smx_process_t process) override;
  void run_all() override;
private:
  void run_all_adaptative();
  void run_all_serial();
  void run_all_parallel();
};

ContextFactory* raw_factory()
{
  XBT_VERB("Using raw contexts. Because the glibc is just not good enough for us.");
  return new RawContextFactory();
}

}
}

// ***** Loads of static stuff

#if HAVE_THREAD_CONTEXTS
static xbt_parmap_t raw_parmap;
static simgrid::simix::RawContext** raw_workers_context;    /* space to save the worker context in each thread */
static uintptr_t raw_threads_working;     /* number of threads that have started their work */
static xbt_os_thread_key_t raw_worker_id_key; /* thread-specific storage for the thread id */
#endif
#ifdef ADAPTIVE_THRESHOLD
#define SCHED_ROUND_LIMIT 5
static xbt_os_timer_t round_time;
static double par_time,seq_time;
static double par_ratio,seq_ratio;
static int reached_seq_limit, reached_par_limit;
static unsigned int par_proc_that_ran = 0,seq_proc_that_ran = 0;  /* Counters of processes that have run in SCHED_ROUND_LIMIT scheduling rounds */
static unsigned int seq_sched_round=0, par_sched_round=0; /* Amount of SR that ran serial/parallel*/
/*Varables used to calculate running variance and mean*/
static double prev_avg_par_proc=0,prev_avg_seq_proc=0;
static double delta=0;
static double s_par_proc=0,s_seq_proc=0; /*Standard deviation of number of processes computed in par/seq during the current simulation*/
static double avg_par_proc=0,sd_par_proc=0;
static double avg_seq_proc=0,sd_seq_proc=0;
static long long par_window=(long long)HUGE_VAL,seq_window=0;
#endif
static unsigned long raw_process_index = 0;   /* index of the next process to run in the
                                               * list of runnable processes */
static simgrid::simix::RawContext* raw_maestro_context;

static bool raw_context_parallel = false;
#ifdef ADAPTIVE_THRESHOLD
static bool raw_context_adaptative = false;
#endif

// ***** Raw context routines

typedef void (*rawctx_entry_point_t)(void *);

typedef void* raw_stack_t;
extern "C" raw_stack_t raw_makecontext(void* malloced_stack, int stack_size,
                                   rawctx_entry_point_t entry_point, void* arg);
extern "C" void raw_swapcontext(raw_stack_t* old, raw_stack_t new_context);

#if SIMGRID_PROCESSOR_x86_64
__asm__ (
#if defined(__APPLE__)
   ".text\n"
   ".globl _raw_makecontext\n"
   "_raw_makecontext:\n"
#elif defined(_WIN32)
   ".text\n"
   ".globl raw_makecontext\n"
   "raw_makecontext:\n"
#else
   ".text\n"
   ".globl raw_makecontext\n"
   ".type raw_makecontext,@function\n"
   "raw_makecontext:\n"/* Calling convention sets the arguments in rdi, rsi, rdx and rcx, respectively */
#endif
   "   mov %rdi,%rax\n"      /* stack */
   "   add %rsi,%rax\n"      /* size  */
   "   andq $-16, %rax\n"    /* align stack */
   "   movq $0,   -8(%rax)\n" /* @return for func */
   "   mov %rdx,-16(%rax)\n" /* func */
   "   mov %rcx,-24(%rax)\n" /* arg/rdi */
   "   movq $0,  -32(%rax)\n" /* rsi */
   "   movq $0,  -40(%rax)\n" /* rdx */
   "   movq $0,  -48(%rax)\n" /* rcx */
   "   movq $0,  -56(%rax)\n" /* r8  */
   "   movq $0,  -64(%rax)\n" /* r9  */
   "   movq $0,  -72(%rax)\n" /* rbp */
   "   movq $0,  -80(%rax)\n" /* rbx */
   "   movq $0,  -88(%rax)\n" /* r12 */
   "   movq $0,  -96(%rax)\n" /* r13 */
   "   movq $0, -104(%rax)\n" /* r14 */
   "   movq $0, -112(%rax)\n" /* r15 */
   "   sub $112,%rax\n"
   "   ret\n"
);

__asm__ (
#if defined(__APPLE__)
   ".text\n"
   ".globl _raw_swapcontext\n"
   "_raw_swapcontext:\n"
#elif defined(_WIN32)
   ".text\n"
   ".globl raw_swapcontext\n"
   "raw_swapcontext:\n"
#else
   ".text\n"
   ".globl raw_swapcontext\n"
   ".type raw_swapcontext,@function\n"
   "raw_swapcontext:\n" /* Calling convention sets the arguments in rdi and rsi, respectively */
#endif
   "   push %rdi\n"
   "   push %rsi\n"
   "   push %rdx\n"
   "   push %rcx\n"
   "   push %r8\n"
   "   push %r9\n"
   "   push %rbp\n"
   "   push %rbx\n"
   "   push %r12\n"
   "   push %r13\n"
   "   push %r14\n"
   "   push %r15\n"
   "   mov %rsp,(%rdi)\n" /* old */
   "   mov %rsi,%rsp\n" /* new */
   "   pop %r15\n"
   "   pop %r14\n"
   "   pop %r13\n"
   "   pop %r12\n"
   "   pop %rbx\n"
   "   pop %rbp\n"
   "   pop %r9\n"
   "   pop %r8\n"
   "   pop %rcx\n"
   "   pop %rdx\n"
   "   pop %rsi\n"
   "   pop %rdi\n"
   "   ret\n"
);
#elif SIMGRID_PROCESSOR_i686
__asm__ (
#if defined(__APPLE__) || defined(_WIN32)
   ".text\n"
   ".globl _raw_makecontext\n"
   "_raw_makecontext:\n"
#else
   ".text\n"
   ".globl raw_makecontext\n"
   ".type raw_makecontext,@function\n"
   "raw_makecontext:\n"
#endif
   "   movl 4(%esp),%eax\n"   /* stack */
   "   addl 8(%esp),%eax\n"   /* size  */
   "   andl $-16, %eax\n"     /* align stack */
   "   movl 12(%esp),%ecx\n"  /* func  */
   "   movl 16(%esp),%edx\n"  /* arg   */
   "   movl %edx, -4(%eax)\n"
   "   movl $0,   -8(%eax)\n" /* @return for func */
   "   movl %ecx,-12(%eax)\n"
   "   movl $0,  -16(%eax)\n" /* ebp */
   "   movl $0,  -20(%eax)\n" /* ebx */
   "   movl $0,  -24(%eax)\n" /* esi */
   "   movl $0,  -28(%eax)\n" /* edi */
   "   subl $28,%eax\n"
   "   retl\n"
);

__asm__ (
#if defined(__APPLE__) || defined(_WIN32)
   ".text\n"
   ".globl _raw_swapcontext\n"
   "_raw_swapcontext:\n"
#else
   ".text\n"
   ".globl raw_swapcontext\n"
   ".type raw_swapcontext,@function\n"
   "raw_swapcontext:\n"
#endif
   "   movl 4(%esp),%eax\n" /* old */
   "   movl 8(%esp),%edx\n" /* new */
   "   pushl %ebp\n"
   "   pushl %ebx\n"
   "   pushl %esi\n"
   "   pushl %edi\n"
   "   movl %esp,(%eax)\n"
   "   movl %edx,%esp\n"
   "   popl %edi\n"
   "   popl %esi\n"
   "   popl %ebx\n"
   "   popl %ebp\n"
   "   retl\n"
);
#else


/* If you implement raw contexts for other processors, don't forget to
   update the definition of HAVE_RAW_CONTEXTS in tools/cmake/CompleteInFiles.cmake */

raw_stack_t raw_makecontext(void* malloced_stack, int stack_size,
                            rawctx_entry_point_t entry_point, void* arg) {
   THROW_UNIMPLEMENTED;
}

void raw_swapcontext(raw_stack_t* old, raw_stack_t new_context) {
   THROW_UNIMPLEMENTED;
}

#endif

// ***** Method definitions

namespace simgrid {
namespace simix {

RawContextFactory::RawContextFactory()
  : ContextFactory("RawContextFactory")
{
#ifdef ADAPTIVE_THRESHOLD
  raw_context_adaptative = (SIMIX_context_get_parallel_threshold() > 1);
#endif
  raw_context_parallel = SIMIX_context_is_parallel();
  if (raw_context_parallel) {
#if HAVE_THREAD_CONTEXTS
    int nthreads = SIMIX_context_get_nthreads();
    xbt_os_thread_key_create(&raw_worker_id_key);
    // TODO, lazily init
    raw_parmap = nullptr;
    raw_workers_context = xbt_new(RawContext*, nthreads);
    raw_maestro_context = nullptr;
#endif
    // TODO, if(SIMIX_context_get_parallel_threshold() > 1) => choose dynamically
  }
#ifdef ADAPTIVE_THRESHOLD
  round_time = xbt_os_timer_new();
  reached_seq_limit = 0;
  reached_par_limit = 0;
#endif
}

RawContextFactory::~RawContextFactory()
{
#if HAVE_THREAD_CONTEXTS
  if (raw_parmap)
    xbt_parmap_destroy(raw_parmap);
  xbt_free(raw_workers_context);
#endif
}

RawContext* RawContextFactory::create_context(std::function<void()> code,
    void_pfn_smxprocess_t cleanup, smx_process_t process)
{
  return this->new_context<RawContext>(std::move(code),
    cleanup, process);
}

void RawContext::wrapper(void* arg)
{
  RawContext* context = (RawContext*) arg;
  (*context)();
  context->stop();
}

RawContext::RawContext(std::function<void()> code,
    void_pfn_smxprocess_t cleanup, smx_process_t process)
  : Context(std::move(code), cleanup, process)
{
   if (has_code()) {
     this->stack_ = SIMIX_context_stack_new();
     this->stack_top_ = raw_makecontext(this->stack_,
                         smx_context_usable_stack_size,
                         RawContext::wrapper,
                         this);
   } else {
     if(process != nullptr && raw_maestro_context == nullptr)
       raw_maestro_context = this;
     if (MC_is_active())
       MC_ignore_heap(
         &raw_maestro_context->stack_top_,
         sizeof(raw_maestro_context->stack_top_));
   }
}

RawContext::~RawContext()
{
  SIMIX_context_stack_delete(this->stack_);
}

void RawContext::stop()
{
  Context::stop();
  this->suspend();
}

void RawContextFactory::run_all()
{
#ifdef ADAPTIVE_THRESHOLD
  if (raw_context_adaptative)
    run_all_adaptative();
  else
#endif
  if (raw_context_parallel)
    run_all_parallel();
  else
    run_all_serial();
}

void RawContextFactory::run_all_serial()
{
  smx_process_t first_process =
      xbt_dynar_get_as(simix_global->process_to_run, 0, smx_process_t);
  raw_process_index = 1;
  static_cast<RawContext*>(first_process->context)->resume_serial();
}

void RawContextFactory::run_all_parallel()
{
#if HAVE_THREAD_CONTEXTS
  raw_threads_working = 0;
  if (raw_parmap == nullptr)
    raw_parmap = xbt_parmap_new(
      SIMIX_context_get_nthreads(), SIMIX_context_get_parallel_mode());
  xbt_parmap_apply(raw_parmap,
      [](void* arg) {
        smx_process_t process = static_cast<smx_process_t>(arg);
        RawContext* context = static_cast<RawContext*>(process->context);
        context->resume_parallel();
      },
      simix_global->process_to_run);
#else
  xbt_die("You asked for a parallel execution, but you don't have any threads.");
#endif
}

void RawContext::suspend()
{
  if (raw_context_parallel)
    RawContext::suspend_parallel();
  else
    RawContext::suspend_serial();
}

void RawContext::suspend_serial()
{
  /* determine the next context */
  RawContext* next_context = nullptr;
  unsigned long int i;
  i = raw_process_index++;
  if (i < xbt_dynar_length(simix_global->process_to_run)) {
    /* execute the next process */
    XBT_DEBUG("Run next process");
    next_context = (RawContext*) xbt_dynar_get_as(
        simix_global->process_to_run, i, smx_process_t)->context;
  }
  else {
    /* all processes were run, return to maestro */
    XBT_DEBUG("No more process to run");
    next_context = (RawContext*) raw_maestro_context;
  }
  SIMIX_context_set_current(next_context);
  raw_swapcontext(&this->stack_top_, next_context->stack_top_);
}

void RawContext::suspend_parallel()
{
#if HAVE_THREAD_CONTEXTS
  /* determine the next context */
  smx_process_t next_work = (smx_process_t) xbt_parmap_next(raw_parmap);
  RawContext* next_context = nullptr;

  if (next_work != NULL) {
    /* there is a next process to resume */
    XBT_DEBUG("Run next process");
    next_context = (RawContext*) next_work->context;
  }
  else {
    /* all processes were run, go to the barrier */
    XBT_DEBUG("No more processes to run");
    uintptr_t worker_id = (uintptr_t)
      xbt_os_thread_get_specific(raw_worker_id_key);
    next_context = (RawContext*) raw_workers_context[worker_id];
    XBT_DEBUG("Restoring worker stack %zu (working threads = %zu)",
        worker_id, raw_threads_working);
  }

  SIMIX_context_set_current(next_context);
  raw_swapcontext(&this->stack_top_, next_context->stack_top_);
#endif
}

void RawContext::resume()
{
  if (raw_context_parallel)
    resume_parallel();
  else
    resume_serial();
}

void RawContext::resume_serial()
{
  SIMIX_context_set_current(this);
  raw_swapcontext(&raw_maestro_context->stack_top_, this->stack_top_);
}

void RawContext::resume_parallel()
{
#if HAVE_THREAD_CONTEXTS
  uintptr_t worker_id = __sync_fetch_and_add(&raw_threads_working, 1);
  xbt_os_thread_set_specific(raw_worker_id_key, (void*) worker_id);
  RawContext* worker_context = (RawContext*) SIMIX_context_self();
  raw_workers_context[worker_id] = worker_context;
  XBT_DEBUG("Saving worker stack %zu", worker_id);
  SIMIX_context_set_current(this);
  raw_swapcontext(&worker_context->stack_top_, this->stack_top_);
#else
  xbt_die("Parallel execution disabled");
#endif
}

/**
 * \brief Resumes all processes ready to run.
 */
#ifdef ADAPTIVE_THRESHOLD
void RawContectFactory::run_all_adaptative()
{
  unsigned long nb_processes = xbt_dynar_length(simix_global->process_to_run);
  unsigned long threshold = SIMIX_context_get_parallel_threshold();
  reached_seq_limit = (seq_sched_round % SCHED_ROUND_LIMIT == 0);
  reached_par_limit = (par_sched_round % SCHED_ROUND_LIMIT == 0);

  if(reached_seq_limit && reached_par_limit){
    par_ratio = (par_proc_that_ran != 0) ? (par_time / (double)par_proc_that_ran) : 0;
    seq_ratio = (seq_proc_that_ran != 0) ? (seq_time / (double)seq_proc_that_ran) : 0;
    if(seq_ratio > par_ratio){
       if(nb_processes < avg_par_proc) {
          threshold = (threshold>2) ? threshold - 1 : threshold ;
          SIMIX_context_set_parallel_threshold(threshold);
        }
    } else {
        if(nb_processes > avg_seq_proc){
          SIMIX_context_set_parallel_threshold(threshold+1);
        }
    }
  }

  if (nb_processes >= SIMIX_context_get_parallel_threshold()) {
    simix_global->context_factory->suspend = smx_ctx_raw_suspend_parallel;
    if (nb_processes < par_window){
      par_sched_round++;
      xbt_os_walltimer_start(round_time);
      smx_ctx_raw_runall_parallel();
      xbt_os_walltimer_stop(round_time);
      par_time += xbt_os_timer_elapsed(round_time);

      prev_avg_par_proc = avg_par_proc;
      delta = nb_processes - avg_par_proc;
      avg_par_proc = (par_sched_round==1) ? nb_processes : avg_par_proc + delta / (double) par_sched_round;

      if(par_sched_round>=2){
        s_par_proc = s_par_proc + (nb_processes - prev_avg_par_proc) * delta;
        sd_par_proc = sqrt(s_par_proc / (par_sched_round-1));
        par_window = (int) (avg_par_proc + sd_par_proc);
      }else{
        sd_par_proc = 0;
      }

      par_proc_that_ran += nb_processes;
    } else{
      smx_ctx_raw_runall_parallel();
    }
  } else {
    simix_global->context_factory->suspend = smx_ctx_raw_suspend_serial;
    if(nb_processes > seq_window){
      seq_sched_round++;
      xbt_os_walltimer_start(round_time);
      smx_ctx_raw_runall_serial();
      xbt_os_walltimer_stop(round_time);
      seq_time += xbt_os_timer_elapsed(round_time);

      prev_avg_seq_proc = avg_seq_proc;
      delta = (nb_processes-avg_seq_proc);
      avg_seq_proc = (seq_sched_round==1) ? nb_processes : avg_seq_proc + delta / (double) seq_sched_round;

      if(seq_sched_round>=2){
        s_seq_proc = s_seq_proc + (nb_processes - prev_avg_seq_proc)*delta;
        sd_seq_proc = sqrt(s_seq_proc / (seq_sched_round-1));
        seq_window = (int) (avg_seq_proc - sd_seq_proc);
      } else {
        sd_seq_proc = 0;
      }

      seq_proc_that_ran += nb_processes;
    } else {
      smx_ctx_raw_runall_serial();
    }
  }
}

#else

// TODO
void RawContextFactory::run_all_adaptative()
{
  unsigned long nb_processes = xbt_dynar_length(simix_global->process_to_run);
  if (SIMIX_context_is_parallel()
    && (unsigned long) SIMIX_context_get_parallel_threshold() < nb_processes) {
        raw_context_parallel = true;
        XBT_DEBUG("Runall // %lu", nb_processes);
        this->run_all_parallel();
    } else {
        XBT_DEBUG("Runall serial %lu", nb_processes);
        raw_context_parallel = false;
        this->run_all_serial();
    }
}
#endif

}
}
