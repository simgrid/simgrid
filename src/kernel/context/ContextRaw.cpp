/* Copyright (c) 2009-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "ContextRaw.hpp"

#include "mc/mc.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_context);

// Raw context routines

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

// RawContextFactory

RawContextFactory::RawContextFactory() : ContextFactory("RawContextFactory"), parallel_(SIMIX_context_is_parallel())
{
  RawContext::setMaestro(nullptr);
  if (parallel_) {
#if HAVE_THREAD_CONTEXTS
    // TODO: choose dynamically when SIMIX_context_get_parallel_threshold() > 1
    ParallelRawContext::initialize();
#else
    xbt_die("You asked for a parallel execution, but you don't have any threads.");
#endif
  }
}

RawContextFactory::~RawContextFactory()
{
#if HAVE_THREAD_CONTEXTS
  if (parallel_)
    ParallelRawContext::finalize();
#endif
}

Context* RawContextFactory::create_context(std::function<void()> code, void_pfn_smxprocess_t cleanup_func,
                                           smx_actor_t process)
{
#if HAVE_THREAD_CONTEXTS
  if (parallel_)
    return this->new_context<ParallelRawContext>(std::move(code), cleanup_func, process);
#endif

  return this->new_context<SerialRawContext>(std::move(code), cleanup_func, process);
}

void RawContextFactory::run_all()
{
#if HAVE_THREAD_CONTEXTS
  if (parallel_)
    ParallelRawContext::run_all();
  else
#endif
    SerialRawContext::run_all();
}

// RawContext

RawContext* RawContext::maestro_context_ = nullptr;

RawContext::RawContext(std::function<void()> code, void_pfn_smxprocess_t cleanup, smx_actor_t process)
    : Context(std::move(code), cleanup, process)
{
   if (has_code()) {
     this->stack_ = SIMIX_context_stack_new();
     this->stack_top_ = raw_makecontext(this->stack_, smx_context_usable_stack_size, RawContext::wrapper, this);
   } else {
     if (process != nullptr && maestro_context_ == nullptr)
       maestro_context_ = this;
     if (MC_is_active())
       MC_ignore_heap(&maestro_context_->stack_top_, sizeof(maestro_context_->stack_top_));
   }
}

RawContext::~RawContext()
{
  SIMIX_context_stack_delete(this->stack_);
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

inline void RawContext::swap(RawContext* from, RawContext* to)
{
  raw_swapcontext(&from->stack_top_, to->stack_top_);
}

void RawContext::stop()
{
  Context::stop();
  throw StopRequest();
}

// SerialRawContext

unsigned long SerialRawContext::process_index_; /* index of the next process to run in the list of runnable processes */

void SerialRawContext::suspend()
{
  /* determine the next context */
  SerialRawContext* next_context;
  unsigned long int i = process_index_;
  process_index_++;
  if (i < simix_global->process_to_run.size()) {
    /* execute the next process */
    XBT_DEBUG("Run next process");
    next_context = static_cast<SerialRawContext*>(simix_global->process_to_run[i]->context);
  } else {
    /* all processes were run, return to maestro */
    XBT_DEBUG("No more process to run");
    next_context = static_cast<SerialRawContext*>(RawContext::getMaestro());
  }
  SIMIX_context_set_current(next_context);
  RawContext::swap(this, next_context);
}

void SerialRawContext::resume()
{
  SIMIX_context_set_current(this);
  RawContext::swap(RawContext::getMaestro(), this);
}

void SerialRawContext::run_all()
{
  if (simix_global->process_to_run.empty())
    return;
  smx_actor_t first_process = simix_global->process_to_run.front();
  process_index_            = 1;
  static_cast<SerialRawContext*>(first_process->context)->resume();
}

// ParallelRawContext

#if HAVE_THREAD_CONTEXTS

simgrid::xbt::Parmap<smx_actor_t>* ParallelRawContext::parmap_;
std::atomic<uintptr_t> ParallelRawContext::threads_working_; /* number of threads that have started their work */
xbt_os_thread_key_t ParallelRawContext::worker_id_key_; /* thread-specific storage for the thread id */
std::vector<ParallelRawContext*> ParallelRawContext::workers_context_; /* space to save the worker context
                                                                          in each thread */

void ParallelRawContext::initialize()
{
  parmap_ = nullptr;
  workers_context_.clear();
  workers_context_.resize(SIMIX_context_get_nthreads(), nullptr);
  xbt_os_thread_key_create(&worker_id_key_);
}

void ParallelRawContext::finalize()
{
  delete parmap_;
  parmap_ = nullptr;
  workers_context_.clear();
  xbt_os_thread_key_destroy(worker_id_key_);
}

void ParallelRawContext::run_all()
{
  threads_working_ = 0;
  if (parmap_ == nullptr)
    parmap_ = new simgrid::xbt::Parmap<smx_actor_t>(SIMIX_context_get_nthreads(), SIMIX_context_get_parallel_mode());
  parmap_->apply(
      [](smx_actor_t process) {
        ParallelRawContext* context = static_cast<ParallelRawContext*>(process->context);
        context->resume();
      },
      simix_global->process_to_run);
}

void ParallelRawContext::suspend()
{
  /* determine the next context */
  boost::optional<smx_actor_t> next_work = parmap_->next();
  ParallelRawContext* next_context;
  if (next_work) {
    /* there is a next process to resume */
    XBT_DEBUG("Run next process");
    next_context = static_cast<ParallelRawContext*>(next_work.get()->context);
  } else {
    /* all processes were run, go to the barrier */
    XBT_DEBUG("No more processes to run");
    uintptr_t worker_id = reinterpret_cast<uintptr_t>(xbt_os_thread_get_specific(worker_id_key_));
    next_context        = workers_context_[worker_id];
    XBT_DEBUG("Restoring worker stack %zu (working threads = %zu)", worker_id, threads_working_.load());
  }

  SIMIX_context_set_current(next_context);
  RawContext::swap(this, next_context);
}

void ParallelRawContext::resume()
{
  uintptr_t worker_id = threads_working_.fetch_add(1, std::memory_order_relaxed);
  xbt_os_thread_set_specific(worker_id_key_, reinterpret_cast<void*>(worker_id));
  ParallelRawContext* worker_context = static_cast<ParallelRawContext*>(SIMIX_context_self());
  workers_context_[worker_id]        = worker_context;
  XBT_DEBUG("Saving worker stack %zu", worker_id);
  SIMIX_context_set_current(this);
  RawContext::swap(worker_context, this);
}

#endif

ContextFactory* raw_factory()
{
  XBT_VERB("Using raw contexts. Because the glibc is just not good enough for us.");
  return new RawContextFactory();
}
}}}
