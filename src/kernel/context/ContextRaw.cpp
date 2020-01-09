/* Copyright (c) 2009-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "ContextRaw.hpp"
#include "context_private.hpp"
#include "mc/mc.h"
#include "simgrid/Exception.hpp"
#include "src/simix/smx_private.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_context);

// Raw context routines

typedef void (*rawctx_entry_point_t)(simgrid::kernel::context::RawContext*);

typedef void* raw_stack_t;
extern "C" raw_stack_t raw_makecontext(void* malloced_stack, int stack_size, rawctx_entry_point_t entry_point,
                                       simgrid::kernel::context::RawContext* arg);
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

raw_stack_t raw_makecontext(void* malloced_stack, int stack_size, rawctx_entry_point_t entry_point,
                            simgrid::kernel::context::RawContext* arg)
{
  THROW_UNIMPLEMENTED;
}

void raw_swapcontext(raw_stack_t* old, raw_stack_t new_context)
{
  THROW_UNIMPLEMENTED;
}

#endif

// ***** Method definitions

namespace simgrid {
namespace kernel {
namespace context {

// RawContextFactory

RawContext* RawContextFactory::create_context(std::function<void()>&& code, actor::ActorImpl* actor)
{
  return this->new_context<RawContext>(std::move(code), actor, this);
}

// RawContext

RawContext::RawContext(std::function<void()>&& code, actor::ActorImpl* actor, SwappedContextFactory* factory)
    : SwappedContext(std::move(code), actor, factory)
{
   if (has_code()) {
     this->stack_top_ = raw_makecontext(get_stack(), smx_context_stack_size, RawContext::wrapper, this);
   } else {
     if (MC_is_active())
       MC_ignore_heap(&stack_top_, sizeof(stack_top_));
   }
}

void RawContext::wrapper(RawContext* context)
{
  ASAN_FINISH_SWITCH(nullptr, &context->asan_ctx_->asan_stack_, &context->asan_ctx_->asan_stack_size_);
  try {
    (*context)();
    context->Context::stop();
  } catch (ForcefulKillException const&) {
    XBT_DEBUG("Caught a ForcefulKillException");
  } catch (simgrid::Exception const& e) {
    XBT_INFO("Actor killed by an uncaught exception %s", simgrid::xbt::demangle(typeid(e).name()).get());
    throw;
  }
  ASAN_ONLY(context->asan_stop_ = true);
  context->suspend();
  THROW_IMPOSSIBLE;
}

void RawContext::swap_into(SwappedContext* to_)
{
  const RawContext* to = static_cast<RawContext*>(to_);
  ASAN_ONLY(void* fake_stack = nullptr);
  ASAN_ONLY(to_->asan_ctx_ = this);
  ASAN_START_SWITCH(this->asan_stop_ ? nullptr : &fake_stack, to_->asan_stack_, to_->asan_stack_size_);
  raw_swapcontext(&this->stack_top_, to->stack_top_);
  ASAN_FINISH_SWITCH(fake_stack, &this->asan_ctx_->asan_stack_, &this->asan_ctx_->asan_stack_size_);
}

ContextFactory* raw_factory()
{
  XBT_VERB("Using raw contexts. Because the glibc is just not good enough for us.");
  return new RawContextFactory();
}
} // namespace context
} // namespace kernel
} // namespace simgrid
