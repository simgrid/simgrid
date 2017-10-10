/* Copyright (c) 2009-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/internal_config.h"

#include "xbt/parmap.hpp"

#include "mc/mc.h"
#include "src/simix/smx_private.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_context);

// ***** Class definitions

namespace simgrid {
namespace kernel {
namespace context {

class RawContext;
class RawContextFactory;

/** @brief Fast context switching inspired from SystemV ucontexts.
  *
  * The main difference to the System V context is that Raw Contexts are much faster because they don't
  * preserve the signal mask when switching. This saves a system call (at least on Linux) on each context switch.
  */
class RawContext : public Context {
protected:
  void* stack_ = nullptr;
  /** pointer to top the stack stack */
  void* stack_top_ = nullptr;
public:
  friend class RawContextFactory;
  RawContext(std::function<void()> code,
          void_pfn_smxprocess_t cleanup_func,
          smx_actor_t process);
  ~RawContext() override;

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
  ~RawContextFactory() override;
  RawContext* create_context(std::function<void()> code,
    void_pfn_smxprocess_t cleanup, smx_actor_t process) override;
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

}}} // namespace

// ***** Loads of static stuff

#if HAVE_THREAD_CONTEXTS
static simgrid::xbt::Parmap<smx_actor_t>* raw_parmap;
static simgrid::kernel::context::RawContext** raw_workers_context;    /* space to save the worker context in each thread */
static uintptr_t raw_threads_working;     /* number of threads that have started their work */
static xbt_os_thread_key_t raw_worker_id_key; /* thread-specific storage for the thread id */
#endif
static unsigned long raw_process_index = 0;   /* index of the next process to run in the
                                               * list of runnable processes */
static simgrid::kernel::context::RawContext* raw_maestro_context;

static bool raw_context_parallel = false;

// ***** Raw context routines

typedef void (*rawctx_entry_point_t)(void *);

typedef void* raw_stack_t;
extern "C" raw_stack_t raw_makecontext(void* malloced_stack, int stack_size,
                                   rawctx_entry_point_t entry_point, void* arg);
extern "C" void raw_swapcontext(raw_stack_t* old, raw_stack_t new_context);

// TODO, we should handle FP, MMX and the x87 control-word (for x86 and x86_64)

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
   // Fetch the parameters:
   "   movl 4(%esp),%eax\n" /* old (raw_stack_t*) */
   "   movl 8(%esp),%edx\n" /* new (raw_stack_t)  */
   // Save registers of the current context on the stack:
   "   pushl %ebp\n"
   "   pushl %ebx\n"
   "   pushl %esi\n"
   "   pushl %edi\n"
   // Save the current context (stack pointer) in *old:
   "   movl %esp,(%eax)\n"
   // Switch to the stack of the new context:
   "   movl %edx,%esp\n"
   // Pop the values of the new context:
   "   popl %edi\n"
   "   popl %esi\n"
   "   popl %ebx\n"
   "   popl %ebp\n"
   // Return using the return address of the new context:
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
namespace kernel {
namespace context {

RawContextFactory::RawContextFactory()
  : ContextFactory("RawContextFactory")
{
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
    // TODO: choose dynamically when SIMIX_context_get_parallel_threshold() > 1
  }
}

RawContextFactory::~RawContextFactory()
{
#if HAVE_THREAD_CONTEXTS
  delete raw_parmap;
  xbt_free(raw_workers_context);
#endif
}

RawContext* RawContextFactory::create_context(std::function<void()> code,
    void_pfn_smxprocess_t cleanup, smx_actor_t process)
{
  return this->new_context<RawContext>(std::move(code), cleanup, process);
}

void RawContext::wrapper(void* arg)
{
  RawContext* context = static_cast<RawContext*>(arg);
  try {
    (*context)();
    context->Context::stop();
  } catch (StopRequest const&) {
    XBT_DEBUG("Caught a StopRequest");
  }
  context->suspend();
}

RawContext::RawContext(std::function<void()> code,
    void_pfn_smxprocess_t cleanup, smx_actor_t process)
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
  throw StopRequest();
}

void RawContextFactory::run_all()
{
  if (raw_context_parallel)
    run_all_parallel();
  else
    run_all_serial();
}

void RawContextFactory::run_all_serial()
{
  if (simix_global->process_to_run.empty())
    return;

  smx_actor_t first_process = simix_global->process_to_run.front();
  raw_process_index = 1;
  static_cast<RawContext*>(first_process->context)->resume_serial();
}

void RawContextFactory::run_all_parallel()
{
#if HAVE_THREAD_CONTEXTS
  raw_threads_working = 0;
  if (raw_parmap == nullptr)
    raw_parmap = new simgrid::xbt::Parmap<smx_actor_t>(SIMIX_context_get_nthreads(), SIMIX_context_get_parallel_mode());
  raw_parmap->apply(
      [](smx_actor_t process) {
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
  unsigned long int i      = raw_process_index;
  raw_process_index++;
  if (i < simix_global->process_to_run.size()) {
    /* execute the next process */
    XBT_DEBUG("Run next process");
    next_context = static_cast<RawContext*>(simix_global->process_to_run[i]->context);
  } else {
    /* all processes were run, return to maestro */
    XBT_DEBUG("No more process to run");
    next_context = static_cast<RawContext*>(raw_maestro_context);
  }
  SIMIX_context_set_current(next_context);
  raw_swapcontext(&this->stack_top_, next_context->stack_top_);
}

void RawContext::suspend_parallel()
{
#if HAVE_THREAD_CONTEXTS
  /* determine the next context */
  boost::optional<smx_actor_t> next_work = raw_parmap->next();
  RawContext* next_context;
  if (next_work) {
    /* there is a next process to resume */
    XBT_DEBUG("Run next process");
    next_context = static_cast<RawContext*>(next_work.get()->context);
  } else {
    /* all processes were run, go to the barrier */
    XBT_DEBUG("No more processes to run");
    uintptr_t worker_id = (uintptr_t)
      xbt_os_thread_get_specific(raw_worker_id_key);
    next_context = static_cast<RawContext*>(raw_workers_context[worker_id]);
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
  RawContext* worker_context     = static_cast<RawContext*>(SIMIX_context_self());
  raw_workers_context[worker_id] = worker_context;
  XBT_DEBUG("Saving worker stack %zu", worker_id);
  SIMIX_context_set_current(this);
  raw_swapcontext(&worker_context->stack_top_, this->stack_top_);
#else
  xbt_die("Parallel execution disabled");
#endif
}

/** @brief Resumes all processes ready to run. */
void RawContextFactory::run_all_adaptative()
{
  unsigned long nb_processes = simix_global->process_to_run.size();
  if (SIMIX_context_is_parallel() &&
      static_cast<unsigned long>(SIMIX_context_get_parallel_threshold()) < nb_processes) {
    raw_context_parallel = true;
    XBT_DEBUG("Runall // %lu", nb_processes);
    this->run_all_parallel();
  } else {
    XBT_DEBUG("Runall serial %lu", nb_processes);
    raw_context_parallel = false;
    this->run_all_serial();
  }
}

}}}
