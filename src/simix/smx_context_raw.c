/* context_raw - context switching with ucontextes from System V           */

/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simix/private.h"
#include "xbt/parmap.h"


#ifdef HAVE_VALGRIND_VALGRIND_H
#  include <valgrind/valgrind.h>
#endif                          /* HAVE_VALGRIND_VALGRIND_H */

#ifdef _XBT_WIN32
#include "win32_ucontext.h"
#include "win32_ucontext.c"
#else
#include "ucontext.h"
#endif

typedef char * raw_stack_t;
typedef void (*rawctx_entry_point_t)(void *);

typedef struct s_smx_ctx_raw {
  s_smx_ctx_base_t super;       /* Fields of super implementation */
  char *malloced_stack; /* malloced area containing the stack */
  raw_stack_t stack_top; /* pointer to stack top (within previous area) */
  raw_stack_t old_stack_top; /* to whom I should return the control */
#ifdef HAVE_VALGRIND_VALGRIND_H
  unsigned int valgrind_stack_id;       /* the valgrind stack id */
#endif
} s_smx_ctx_raw_t, *smx_ctx_raw_t;

smx_ctx_raw_t maestro_raw_context;

extern raw_stack_t raw_makecontext(char* malloced_stack, int stack_size,
                                   rawctx_entry_point_t entry_point, void* arg);
extern void raw_swapcontext(raw_stack_t* old, raw_stack_t new);

#ifdef PROCESSOR_i686
__asm__ (
   ".text\n"
   ".globl raw_makecontext\n"
   ".type raw_makecontext,@function\n"
   "raw_makecontext:\n"
   "   movl 4(%esp),%eax\n"   /* stack */
   "   addl 8(%esp),%eax\n"   /* size  */
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
   ".text\n"
   ".globl raw_swapcontext\n"
   ".type raw_swapcontext,@function\n"
   "raw_swapcontext:\n"
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
#elif PROCESSOR_x86_64
__asm__ (
   ".text\n"
   ".globl raw_makecontext\n"
   ".type raw_makecontext,@function\n"
   "raw_makecontext:\n" /* Calling convention sets the arguments in rdi, rsi, rdx and rcx, respectively */
   "   movq %rdi,%rax\n"      /* stack */
   "   addq %rsi,%rax\n"      /* size  */
   "   movq $0,   -8(%rax)\n" /* @return for func */
   "   movq %rdx,-16(%rax)\n" /* func */
   "   movq %rcx,-24(%rax)\n" /* arg/rdi */
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
   "   subq $112,%rax\n"
   "   retq\n"
);

__asm__ (
   ".text\n"
   ".globl raw_swapcontext\n"
   ".type raw_swapcontext,@function\n"
   "raw_swapcontext:\n" /* Calling convention sets the arguments in rdi and rsi, respectively */
   "   pushq %rdi\n"
   "   pushq %rsi\n"
   "   pushq %rdx\n"
   "   pushq %rcx\n"
   "   pushq %r8\n"
   "   pushq %r9\n"
   "   pushq %rbp\n"
   "   pushq %rbx\n"
   "   pushq %r12\n"
   "   pushq %r13\n"
   "   pushq %r14\n"
   "   pushq %r15\n"
   "   movq %rsp,(%rdi)\n" /* old */
   "   movq %rsi,%rsp\n" /* new */
   "   popq %r15\n"
   "   popq %r14\n"
   "   popq %r13\n"
   "   popq %r12\n"
   "   popq %rbx\n"
   "   popq %rbp\n"
   "   popq %r9\n"
   "   popq %r8\n"
   "   popq %rcx\n"
   "   popq %rdx\n"
   "   popq %rsi\n"
   "   popq %rdi\n"
   "   retq\n"
);
#else
raw_stack_t raw_makecontext(char* malloced_stack, int stack_size,
                            rawctx_entry_point_t entry_point, void* arg) {
   THROW_UNIMPLEMENTED;
}

void raw_swapcontext(raw_stack_t* old, raw_stack_t new) {
   THROW_UNIMPLEMENTED;
}

#endif

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_context);

static xbt_parmap_t parmap;

#ifdef CONTEXT_THREADS
static __thread smx_context_t current_context;
#endif

static void smx_ctx_raw_wrapper(smx_ctx_raw_t context);

static int smx_ctx_raw_factory_finalize(smx_context_factory_t *factory)
{ 
  if(parmap)
      xbt_parmap_destroy(parmap);
  return smx_ctx_base_factory_finalize(factory);
}


static smx_context_t
smx_ctx_raw_create_context(xbt_main_func_t code, int argc, char **argv,
    void_pfn_smxprocess_t cleanup_func,
    void *data)
{

  smx_ctx_raw_t context =
      (smx_ctx_raw_t) smx_ctx_base_factory_create_context_sized(
          sizeof(s_smx_ctx_raw_t),
          code,
          argc,
          argv,
          cleanup_func,
          data);

  /* If the user provided a function for the process then use it
     otherwise is the context for maestro */
     if (code) {
       context->malloced_stack = xbt_malloc0(smx_context_stack_size);
       context->stack_top =
           raw_makecontext(context->malloced_stack, smx_context_stack_size,
               (void(*)(void*))smx_ctx_raw_wrapper,context);

#ifdef HAVE_VALGRIND_VALGRIND_H
       context->valgrind_stack_id =
           VALGRIND_STACK_REGISTER(context->malloced_stack,
               context->malloced_stack + smx_context_stack_size);
#endif                          /* HAVE_VALGRIND_VALGRIND_H */

     }else{
       maestro_raw_context = context;
       current_context = (smx_context_t) maestro_raw_context;
     }

     return (smx_context_t) context;
}

static void smx_ctx_raw_free(smx_context_t context)
{

  if (context) {

#ifdef HAVE_VALGRIND_VALGRIND_H
    VALGRIND_STACK_DEREGISTER(((smx_ctx_raw_t)
        context)->valgrind_stack_id);
#endif                          /* HAVE_VALGRIND_VALGRIND_H */

    free(((smx_ctx_raw_t)context)->malloced_stack);
  }
  smx_ctx_base_free(context);
}

static void smx_ctx_raw_suspend(smx_context_t context)
{
  smx_current_context = (smx_context_t)maestro_raw_context;
  raw_swapcontext(
      &((smx_ctx_raw_t) context)->stack_top,
      ((smx_ctx_raw_t) context)->old_stack_top);
}

static void smx_ctx_raw_stop(smx_context_t context)
{
  smx_ctx_base_stop(context);
  smx_ctx_raw_suspend(context);
}

static void smx_ctx_raw_wrapper(smx_ctx_raw_t context)
{ 
  (context->super.code) (context->super.argc, context->super.argv);

  smx_ctx_raw_stop((smx_context_t) context);
}

static void smx_ctx_raw_resume(smx_context_t context)
{
  smx_current_context = context; 
  raw_swapcontext(
      &((smx_ctx_raw_t) context)->old_stack_top,
      ((smx_ctx_raw_t) context)->stack_top);
}

static void smx_ctx_raw_runall(xbt_dynar_t processes)
{
  smx_process_t process;
  unsigned int cursor;

  xbt_dynar_foreach(processes, cursor, process) {
    DEBUG2("Schedule item %u of %lu",cursor,xbt_dynar_length(processes));
    smx_ctx_raw_resume(process->context);
  }
  xbt_dynar_reset(processes);
}

static void smx_ctx_raw_resume_parallel(smx_process_t process)
{
  smx_ctx_raw_t context = (smx_ctx_raw_t)process->context;
  /*xbt_os_thread_set_extra_data(context);*/
  current_context = (smx_context_t)context;
  raw_swapcontext(
      &context->old_stack_top,
      context->stack_top);
  current_context = (smx_context_t)maestro_raw_context;
  /*xbt_os_thread_set_extra_data(NULL);*/
}

static void smx_ctx_raw_runall_parallel(xbt_dynar_t processes)
{
  xbt_parmap_apply(parmap, (void_f_pvoid_t)smx_ctx_raw_resume_parallel, processes);
  xbt_dynar_reset(processes);
}

static smx_context_t smx_ctx_raw_self_parallel(void)
{
  /*smx_context_t self_context = (smx_context_t) xbt_os_thread_get_extra_data();
  return self_context ? self_context : (smx_context_t) maestro_raw_context;*/
  return current_context;
}

void SIMIX_ctx_raw_factory_init(smx_context_factory_t *factory)
{
  VERB0("Using raw contexts. Because the glibc is just not good enough for us.");
  smx_ctx_base_factory_init(factory);

  (*factory)->finalize  = smx_ctx_raw_factory_finalize;
  (*factory)->create_context = smx_ctx_raw_create_context;
  /* Do not overload that method (*factory)->finalize */
  (*factory)->free = smx_ctx_raw_free;
  (*factory)->stop = smx_ctx_raw_stop;
  (*factory)->suspend = smx_ctx_raw_suspend;
  (*factory)->name = "smx_raw_context_factory";

  if (SIMIX_context_is_parallel()) {
#ifdef CONTEXT_THREADS  /* To use parallel ucontexts a thread pool is needed */
    parmap = xbt_parmap_new(2);
    (*factory)->runall = smx_ctx_raw_runall_parallel;
    (*factory)->self = smx_ctx_raw_self_parallel;
#else
    THROW0(arg_error, 0, "No thread support for parallel context execution");
#endif
  } else {
    (*factory)->runall = smx_ctx_raw_runall;
  }
}
