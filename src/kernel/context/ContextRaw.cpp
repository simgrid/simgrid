/* Copyright (c) 2009-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "ContextRaw.hpp"
#include "simgrid/Exception.hpp"
#include "src/mc/mc.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(ker_context);

// Raw context routines

using rawctx_entry_point_t = void (*)(simgrid::kernel::context::SwappedContext*);

using raw_stack_t = void*;
extern "C" raw_stack_t raw_makecontext(void* malloced_stack, int stack_size, rawctx_entry_point_t entry_point,
                                       simgrid::kernel::context::SwappedContext* arg);
extern "C" void raw_swapcontext(raw_stack_t* old, raw_stack_t new_context);

// TODO, we should handle FP, MMX and the x87 control-word (for x86 and x86_64)

#if HAVE_RAW_CONTEXTS

#if SIMGRID_PROCESSOR_x86_64
__asm__(
#if defined(__APPLE__)
    ".text\n"
    ".globl _raw_makecontext\n"
    "_raw_makecontext:\n"
#else
    ".text\n"
    ".globl raw_makecontext\n"
    ".type raw_makecontext,@function\n"
    "raw_makecontext:\n" /* Calling convention sets the arguments in rdi, rsi, rdx and rcx, respectively */
#endif
    "   mov %rdi,%rax\n"       /* stack */
    "   add %rsi,%rax\n"       /* size  */
    "   andq $-16, %rax\n"     /* align stack */
    "   movq $0,   -8(%rax)\n" /* @return for func */
    "   mov %rdx,-16(%rax)\n"  /* func */
    "   mov %rcx,-24(%rax)\n"  /* arg/rdi */
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
    "   ret\n");

__asm__(
#if defined(__APPLE__)
    ".text\n"
    ".globl _raw_swapcontext\n"
    "_raw_swapcontext:\n"
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
    "   mov %rsi,%rsp\n"   /* new */
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
    "   ret\n");
#elif SIMGRID_PROCESSOR_i686
__asm__(
#if defined(__APPLE__)
    ".text\n"
    ".globl _raw_makecontext\n"
    "_raw_makecontext:\n"
#else
    ".text\n"
    ".globl raw_makecontext\n"
    ".type raw_makecontext,@function\n"
    "raw_makecontext:\n"
#endif
    "   movl 4(%esp),%eax\n"  /* stack */
    "   addl 8(%esp),%eax\n"  /* size  */
    "   andl $-16, %eax\n"    /* align stack */
    "   movl 12(%esp),%ecx\n" /* func  */
    "   movl 16(%esp),%edx\n" /* arg   */
    "   movl %edx, -4(%eax)\n"
    "   movl $0,   -8(%eax)\n" /* @return for func */
    "   movl %ecx,-12(%eax)\n"
    "   movl $0,  -16(%eax)\n" /* ebp */
    "   movl $0,  -20(%eax)\n" /* ebx */
    "   movl $0,  -24(%eax)\n" /* esi */
    "   movl $0,  -28(%eax)\n" /* edi */
    "   subl $28,%eax\n"
    "   retl\n");

__asm__(
#if defined(__APPLE__)
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
    "   retl\n");
#elif SIMGRID_PROCESSOR_arm64
__asm__(
    ".text\n"
    ".globl raw_makecontext\n"
    ".type raw_makecontext,@function\n"
    "raw_makecontext:\n"          /* x0=stack, x1=size, x2=entry, x3=arg */
    "   add x0, x0, x1\n"         /* stack + size */
    "   and x0, x0, #-16\n"       /* align stack */
    "   sub x0, x0, #176\n"       /* frame size: 112 (x regs) + 64 (d regs) */
    "   str x3, [x0, #0]\n"       /* x0 slot = arg */
    "   stp xzr, xzr, [x0, #16]\n"
    "   stp xzr, xzr, [x0, #32]\n"
    "   stp xzr, xzr, [x0, #48]\n"
    "   stp xzr, xzr, [x0, #64]\n"
    "   stp xzr, xzr, [x0, #80]\n"
    "   stp xzr, x2, [x0, #96]\n"  /* x29=0, x30=entry */
    "   stp xzr, xzr, [x0, #112]\n"
    "   stp xzr, xzr, [x0, #128]\n"
    "   stp xzr, xzr, [x0, #144]\n"
    "   stp xzr, xzr, [x0, #160]\n"
    "   ret\n");

__asm__(
    ".text\n"
    ".globl raw_swapcontext\n"
    ".type raw_swapcontext,@function\n"
    "raw_swapcontext:\n"          /* x0=old, x1=new */
    "   mov x9, x1\n"
    "   sub sp, sp, #176\n"
    "   str x0, [sp, #0]\n"
    "   stp x19, x20, [sp, #16]\n"
    "   stp x21, x22, [sp, #32]\n"
    "   stp x23, x24, [sp, #48]\n"
    "   stp x25, x26, [sp, #64]\n"
    "   stp x27, x28, [sp, #80]\n"
    "   stp x29, x30, [sp, #96]\n"
    "   stp d8, d9, [sp, #112]\n"
    "   stp d10, d11, [sp, #128]\n"
    "   stp d12, d13, [sp, #144]\n"
    "   stp d14, d15, [sp, #160]\n"
    "   mov x10, sp\n"
    "   str x10, [x0]\n"
    "   mov sp, x9\n"
    "   ldr x0, [sp, #0]\n"
    "   ldp x19, x20, [sp, #16]\n"
    "   ldp x21, x22, [sp, #32]\n"
    "   ldp x23, x24, [sp, #48]\n"
    "   ldp x25, x26, [sp, #64]\n"
    "   ldp x27, x28, [sp, #80]\n"
    "   ldp x29, x30, [sp, #96]\n"
    "   ldp d8, d9, [sp, #112]\n"
    "   ldp d10, d11, [sp, #128]\n"
    "   ldp d12, d13, [sp, #144]\n"
    "   ldp d14, d15, [sp, #160]\n"
    "   add sp, sp, #176\n"
    "   ret\n");
#else
#error HAVE_RAW_CONTEXTS defined, but neither SIMGRID_PROCESSOR_x86_64, SIMGRID_PROCESSOR_i686 nor SIMGRID_PROCESSOR_arm64. Please update the code.
#endif

#else /* not HAVE_RAW_CONTEXTS */

/* If you implement raw contexts for other processors, don't forget to
   update the definition of HAVE_RAW_CONTEXTS in tools/cmake/CompleteInFiles.cmake */

raw_stack_t raw_makecontext(void* malloced_stack, int stack_size, rawctx_entry_point_t entry_point,
                            simgrid::kernel::context::SwappedContext* arg)
{
  THROW_UNIMPLEMENTED;
}

void raw_swapcontext(raw_stack_t* old, raw_stack_t new_context)
{
  THROW_UNIMPLEMENTED;
}

#endif

// ***** Method definitions

namespace simgrid::kernel::context {

// RawContextFactory

RawContext* RawContextFactory::create_context(std::function<void()>&& code, actor::ActorImpl* actor)
{
  return this->new_context<RawContext>(std::move(code), actor, this);
}

// RawContext

RawContext::RawContext(std::function<void()>&& code, actor::ActorImpl* actor, SwappedContextFactory* factory)
    : SwappedContext(std::move(code), actor, factory)
{
  XBT_VERB("Creating a context of stack %uMb", actor->get_stacksize() / 1024 / 1024);
  if (has_code()) {
    this->stack_top_ = raw_makecontext(get_stack(), actor->get_stacksize(), smx_ctx_wrapper, this);
  }
}

void RawContext::swap_into_for_real(SwappedContext* to_)
{
  const RawContext* to = static_cast<RawContext*>(to_);
  raw_swapcontext(&this->stack_top_, to->stack_top_);
}

ContextFactory* raw_factory()
{
  XBT_VERB("Using raw contexts. Because the glibc is just not good enough for us.");
  return new RawContextFactory();
}
} // namespace simgrid::kernel::context
