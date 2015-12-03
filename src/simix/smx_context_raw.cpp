/* context_raw - fast context switching inspired from System V ucontexts   */

/* Copyright (c) 2009-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <math.h>
#include "smx_private.h"
#include "xbt/parmap.h"
#include "xbt/dynar.h"
#include "mc/mc.h"

typedef char * raw_stack_t;
typedef void (*rawctx_entry_point_t)(void *);

typedef struct s_smx_ctx_raw {
  s_smx_ctx_base_t super;         /* Fields of super implementation */
  char *malloced_stack;           /* malloced area containing the stack */
  raw_stack_t stack_top;          /* pointer to stack top (within previous area) */
#ifdef TIME_BENCH_PER_SR
  unsigned int thread;            /* Just for measuring purposes */
#endif
} s_smx_ctx_raw_t, *smx_ctx_raw_t;

#ifdef CONTEXT_THREADS
static xbt_parmap_t raw_parmap;
static smx_ctx_raw_t* raw_workers_context;    /* space to save the worker context in each thread */
static unsigned long raw_threads_working;     /* number of threads that have started their work */
static xbt_os_thread_key_t raw_worker_id_key; /* thread-specific storage for the thread id */
#endif 
#ifdef ADAPTIVE_THRESHOLD
#define SCHED_ROUND_LIMIT 5
xbt_os_timer_t round_time;
double par_time,seq_time;
double par_ratio,seq_ratio;
int reached_seq_limit, reached_par_limit;
static unsigned int par_proc_that_ran = 0,seq_proc_that_ran = 0;  /* Counters of processes that have run in SCHED_ROUND_LIMIT scheduling rounds */
static unsigned int seq_sched_round=0, par_sched_round=0; /* Amount of SR that ran serial/parallel*/
/*Varables used to calculate running variance and mean*/
double prev_avg_par_proc=0,prev_avg_seq_proc=0;
double delta=0;
double s_par_proc=0,s_seq_proc=0; /*Standard deviation of number of processes computed in par/seq during the current simulation*/
double avg_par_proc=0,sd_par_proc=0;
double avg_seq_proc=0,sd_seq_proc=0;
long long par_window=(long long)HUGE_VAL,seq_window=0;
#endif

static unsigned long raw_process_index = 0;   /* index of the next process to run in the
                                               * list of runnable processes */
static smx_ctx_raw_t raw_maestro_context;
extern "C" raw_stack_t raw_makecontext(char* malloced_stack, int stack_size,
                                   rawctx_entry_point_t entry_point, void* arg);
extern "C" void raw_swapcontext(raw_stack_t* old, raw_stack_t new_context);

#if PROCESSOR_x86_64
__asm__ (
#if defined(APPLE)
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
#if defined(APPLE)
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
#elif PROCESSOR_i686
__asm__ (
#if defined(APPLE) || defined(_WIN32)
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
#if defined(APPLE) || defined(_WIN32)
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
   update the definition of HAVE_RAWCTX in tools/cmake/CompleteInFiles.cmake */

raw_stack_t raw_makecontext(char* malloced_stack, int stack_size,
                            rawctx_entry_point_t entry_point, void* arg) {
   THROW_UNIMPLEMENTED;
}

void raw_swapcontext(raw_stack_t* old, raw_stack_t new_context) {
   THROW_UNIMPLEMENTED;
}

#endif

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_context);

#ifdef TIME_BENCH_PER_SR
#include "xbt/xbt_os_time.h"
#define NUM_THREADS 4
static xbt_os_timer_t timer;
static double time_thread_sr[NUM_THREADS];
static double time_thread_ssr[NUM_THREADS];
static double time_wasted_sr = 0;
static double time_wasted_ssr = 0;
static unsigned int sr_count = 0;
static unsigned int ssr_count = 0;
static char new_sr = 0;
#endif

#ifdef TIME_BENCH_ENTIRE_SRS
static unsigned int sr_count = 0;
static xbt_os_timer_t timer;
#endif

static void smx_ctx_raw_wrapper(smx_ctx_raw_t context);
static int smx_ctx_raw_factory_finalize(smx_context_factory_t *factory);
static smx_context_t smx_ctx_raw_create_context(xbt_main_func_t code, int argc,
    char **argv, void_pfn_smxprocess_t cleanup_func, smx_process_t process);
static void smx_ctx_raw_free(smx_context_t context);
static void smx_ctx_raw_wrapper(smx_ctx_raw_t context);
static void smx_ctx_raw_stop(smx_context_t context);
static void smx_ctx_raw_suspend_serial(smx_context_t context);
static void smx_ctx_raw_resume_serial(smx_process_t first_process);
#ifdef TIME_BENCH_PER_SR
static void smx_ctx_raw_runall_serial(xbt_dynar_t processes);
void smx_ctx_raw_new_sr(void);
#else
static void smx_ctx_raw_runall_serial(void);
#endif
static void smx_ctx_raw_suspend_parallel(smx_context_t context);
static void smx_ctx_raw_resume_parallel(smx_process_t first_process);
static void smx_ctx_raw_runall_parallel(void);
static void smx_ctx_raw_runall(void);

/**
 * \brief Initializes the raw context factory.
 * \param factory where to initialize the factory
 */
void SIMIX_ctx_raw_factory_init(smx_context_factory_t *factory)
{

  XBT_VERB("Using raw contexts. Because the glibc is just not good enough for us.");
  smx_ctx_base_factory_init(factory);

  (*factory)->finalize  = smx_ctx_raw_factory_finalize;
  (*factory)->create_context = smx_ctx_raw_create_context;
  /* Do not overload that method (*factory)->finalize */
  (*factory)->free = smx_ctx_raw_free;
  (*factory)->stop = smx_ctx_raw_stop;
  (*factory)->name = "smx_raw_context_factory";

  if (SIMIX_context_is_parallel()) {
#ifdef CONTEXT_THREADS
    int nthreads = SIMIX_context_get_nthreads();
    xbt_os_thread_key_create(&raw_worker_id_key);
    raw_parmap = xbt_parmap_new(nthreads, SIMIX_context_get_parallel_mode());
    raw_workers_context = xbt_new(smx_ctx_raw_t, nthreads);
    raw_maestro_context=NULL;

#endif
    if (SIMIX_context_get_parallel_threshold() > 1) {
      /* choose dynamically */
      (*factory)->runall = smx_ctx_raw_runall;
      (*factory)->suspend = NULL;
    }
    else {
      /* always parallel */
      (*factory)->runall = smx_ctx_raw_runall_parallel;
      (*factory)->suspend = smx_ctx_raw_suspend_parallel;
    }
  }
  else {
    /* always serial */
    (*factory)->runall = smx_ctx_raw_runall_serial;
    (*factory)->suspend = smx_ctx_raw_suspend_serial;
  }
#ifdef TIME_BENCH_ENTIRE_SRS
  (*factory)->runall = smx_ctx_raw_runall;
  (*factory)->suspend = NULL;
  timer = xbt_os_timer_new();
#endif  

#ifdef ADAPTIVE_THRESHOLD
  round_time = xbt_os_timer_new(); 
  reached_seq_limit = 0;
  reached_par_limit = 0;
#endif

#ifdef TIME_BENCH_PER_SR
  timer = xbt_os_timer_new();
#endif
}

/**
 * \brief Finalizes the raw context factory.
 * \param factory the raw context factory
 */
static int smx_ctx_raw_factory_finalize(smx_context_factory_t *factory)
{
#ifdef TIME_BENCH_PER_SR
  XBT_VERB("Total wasted time in %u SR: %f", sr_count, time_wasted_sr);
  XBT_VERB("Total wasted time in %u SSR: %f", ssr_count, time_wasted_ssr);
#endif

#ifdef CONTEXT_THREADS
  if (raw_parmap)
    xbt_parmap_destroy(raw_parmap);
  xbt_free(raw_workers_context);
#endif
  return smx_ctx_base_factory_finalize(factory);
}

/**
 * \brief Creates a new raw context.
 * \param code main function of this context or NULL to create the maestro
 * context
 * \param argc argument number
 * \param argv arguments to pass to the main function
 * \param cleanup_func a function to call to free the user data when the
 * context finished
 * \param process SIMIX process
 */
static smx_context_t
smx_ctx_raw_create_context(xbt_main_func_t code, int argc, char **argv,
                           void_pfn_smxprocess_t cleanup_func,
                           smx_process_t process)
{

  smx_ctx_raw_t context =
      (smx_ctx_raw_t) smx_ctx_base_factory_create_context_sized(
          sizeof(s_smx_ctx_raw_t),
          code,
          argc,
          argv,
          cleanup_func,
          process);

  /* if the user provided a function for the process then use it,
     otherwise it is the context for maestro */
     if (code) {
       context->malloced_stack = (char*) SIMIX_context_stack_new();
       context->stack_top =
           raw_makecontext(context->malloced_stack,
                           smx_context_usable_stack_size,
                           (void_f_pvoid_t)smx_ctx_raw_wrapper, context);

     } else {
       if(process != NULL && raw_maestro_context==NULL)
         raw_maestro_context = context;

       if(MC_is_active())
         MC_ignore_heap(&(raw_maestro_context->stack_top), sizeof(raw_maestro_context->stack_top));

     }

     return (smx_context_t) context;
}

/**
 * \brief Destroys a raw context.
 * \param context a raw context
 */
static void smx_ctx_raw_free(smx_context_t context)
{
  if (context) {
    SIMIX_context_stack_delete(((smx_ctx_raw_t) context)->malloced_stack);
  }
  smx_ctx_base_free(context);
}

/**
 * \brief Wrapper for the main function of a context.
 * \param context a raw context
 */
static void smx_ctx_raw_wrapper(smx_ctx_raw_t context)
{
  (context->super.code) (context->super.argc, context->super.argv);

  smx_ctx_raw_stop((smx_context_t) context);
}

/**
 * \brief Stops a raw context.
 *
 * This function is called when the main function of the context if finished.
 *
 * \param context the current context
 */
static void smx_ctx_raw_stop(smx_context_t context)
{
  smx_ctx_base_stop(context);
  simix_global->context_factory->suspend(context);
}

/**
 * \brief Suspends a running context and resumes another one or returns to
 * maestro.
 * \param context the current context
 */
static void smx_ctx_raw_suspend_serial(smx_context_t context)
{
  /* determine the next context */
  smx_context_t next_context;
  unsigned long int i; 
#ifdef TIME_BENCH_PER_SR
  i = ++raw_process_index;
#else
  i = raw_process_index++;
#endif
  if (i < xbt_dynar_length(simix_global->process_to_run)) {
    /* execute the next process */
    XBT_DEBUG("Run next process");
    next_context = xbt_dynar_get_as(
        simix_global->process_to_run, i, smx_process_t)->context;
  }
  else {
    /* all processes were run, return to maestro */
    XBT_DEBUG("No more process to run");
    next_context = (smx_context_t) raw_maestro_context;
  }
  SIMIX_context_set_current(next_context);
  raw_swapcontext(&((smx_ctx_raw_t) context)->stack_top,
      ((smx_ctx_raw_t) next_context)->stack_top);
}

/**
 * \brief Resumes sequentially all processes ready to run.
 * \param first_process the first process to resume
 */
static void smx_ctx_raw_resume_serial(smx_process_t first_process)
{
  smx_ctx_raw_t context = (smx_ctx_raw_t) first_process->context;
  SIMIX_context_set_current((smx_context_t) context);
  raw_swapcontext(&raw_maestro_context->stack_top,
      ((smx_ctx_raw_t) context)->stack_top);
}

#ifdef TIME_BENCH_PER_SR
static void smx_ctx_raw_runall_serial(xbt_dynar_t processes)
{
  smx_process_t process;
  unsigned int cursor;
  double elapsed = 0;
  double tmax = 0;
  unsigned long num_proc = xbt_dynar_length(simix_global->process_to_run);
  unsigned int t=0;
  unsigned int data_size = (num_proc / NUM_THREADS) + ((num_proc % NUM_THREADS) ? 1 : 0);

  ssr_count++;
  time_thread_ssr[0] = 0;
  xbt_dynar_foreach(processes, cursor, process){ 
        XBT_VERB("Schedule item %u of %lu",cursor,num_proc);
        if(cursor >= t * data_size + data_size){
          if(time_thread_ssr[t] > tmax)
            tmax = time_thread_ssr[t];
          t++;
          time_thread_ssr[t] = 0;
        }

        if(new_sr){
          ((smx_ctx_raw_t)process->context)->thread = t;
          time_thread_sr[t] = 0;
        }

        xbt_os_cputimer_start(timer);
        smx_ctx_raw_resume_serial(process);
        xbt_os_cputimer_stop(timer);
        elapsed = xbt_os_timer_elapsed(timer);
        time_thread_ssr[t] += elapsed;
        time_thread_sr[((smx_ctx_raw_t)process->context)->thread] += elapsed;
  }

  if(new_sr)
    new_sr = FALSE;

  if(time_thread_ssr[t] > tmax)
    tmax = time_thread_ssr[t];

  for(cursor=0; cursor <= t; cursor++){
    XBT_VERB("Time SSR thread %u = %f (max %f)", cursor, time_thread_ssr[cursor], tmax);
    time_wasted_ssr += tmax - time_thread_ssr[cursor];
  }
}

void smx_ctx_raw_new_sr(void)
{
  int i;
  double tmax = 0;
  new_sr = TRUE;
  sr_count++;
  for(i=0; i < NUM_THREADS; i++){
    if(time_thread_sr[i] > tmax)
      tmax = time_thread_sr[i];
  }

  for(i=0; i < NUM_THREADS; i++){
    XBT_CRITICAL("Time SR thread %u = %f (max %f)", i, time_thread_sr[i], tmax);
    time_wasted_sr += tmax - time_thread_sr[i];
  }

  XBT_CRITICAL("Total time SR %u = %f, %d", sr_count, tmax, xbt_dynar_length(simix_global->process_that_ran));
  XBT_CRITICAL("New scheduling round");
}
#else
/**
 * \brief Resumes sequentially all processes ready to run.
 */
static void smx_ctx_raw_runall_serial(void)
{
  smx_process_t first_process =
      xbt_dynar_get_as(simix_global->process_to_run, 0, smx_process_t);
  raw_process_index = 1;

  /* execute the first process */
  smx_ctx_raw_resume_serial(first_process);
}
#endif

/**
 * \brief Suspends a running context and resumes another one or returns to
 * the main function of the current worker thread.
 * \param context the context of the current worker thread
 */
static void smx_ctx_raw_suspend_parallel(smx_context_t context)
{
#ifdef CONTEXT_THREADS
  /* determine the next context */
  smx_process_t next_work = (smx_process_t) xbt_parmap_next(raw_parmap);
  smx_context_t next_context;
  raw_stack_t next_stack;

  if (next_work != NULL) {
    /* there is a next process to resume */
    XBT_DEBUG("Run next process");
    next_context = next_work->context;
    next_stack = ((smx_ctx_raw_t) next_context)->stack_top;
  }
  else {
    /* all processes were run, go to the barrier */
    XBT_DEBUG("No more processes to run");

    unsigned long worker_id =
        (unsigned long)(uintptr_t) xbt_os_thread_get_specific(raw_worker_id_key);

    next_context = (smx_context_t)raw_workers_context[worker_id];
    XBT_DEBUG("Restoring worker stack %lu (working threads = %lu)",
        worker_id, raw_threads_working);
    next_stack = ((smx_ctx_raw_t)next_context)->stack_top;
  }

  SIMIX_context_set_current(next_context);
  raw_swapcontext(&((smx_ctx_raw_t) context)->stack_top, next_stack);
#endif
}

/**
 * \brief Resumes sequentially in the current worker thread the processes ready
 * to run.
 * \param first_process the first process to resume
 */
static void smx_ctx_raw_resume_parallel(smx_process_t first_process)
{
#ifdef CONTEXT_THREADS
  unsigned long worker_id = __sync_fetch_and_add(&raw_threads_working, 1);
  xbt_os_thread_set_specific(raw_worker_id_key, (void*)(uintptr_t) worker_id);
  smx_ctx_raw_t worker_context = (smx_ctx_raw_t)SIMIX_context_self();
  raw_workers_context[worker_id] = worker_context;
  XBT_DEBUG("Saving worker stack %lu", worker_id);
  raw_stack_t* worker_stack = &(worker_context)->stack_top;


  smx_context_t context = first_process->context;
  SIMIX_context_set_current(context);
  raw_swapcontext(worker_stack, ((smx_ctx_raw_t) context)->stack_top);
#endif
}

/**
 * \brief Resumes in parallel all processes ready to run.
 */
static void smx_ctx_raw_runall_parallel(void)
{
#ifdef CONTEXT_THREADS
  raw_threads_working = 0;
  xbt_parmap_apply(raw_parmap, (void_f_pvoid_t) smx_ctx_raw_resume_parallel,
      simix_global->process_to_run);
#else
  xbt_die("You asked for a parallel execution, but you don't have any threads.");
#endif
}

/**
 * \brief Resumes all processes ready to run.
 */
#ifdef ADAPTIVE_THRESHOLD
static void smx_ctx_raw_runall(void)
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

  //XBT_CRITICAL("Thresh: %d", SIMIX_context_get_parallel_threshold());
  if (nb_processes >= SIMIX_context_get_parallel_threshold()) {
    simix_global->context_factory->suspend = smx_ctx_raw_suspend_parallel;
    if(nb_processes < par_window){ 
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

static void smx_ctx_raw_runall(void)
{
#ifdef TIME_BENCH_ENTIRE_SRS
  sr_count++;
  timer = xbt_os_timer_new();
  double elapsed = 0;
#endif
  unsigned long nb_processes = xbt_dynar_length(simix_global->process_to_run);
  if (SIMIX_context_is_parallel()
    && (unsigned long) SIMIX_context_get_parallel_threshold() < nb_processes) {
        XBT_DEBUG("Runall // %lu", nb_processes);
        simix_global->context_factory->suspend = smx_ctx_raw_suspend_parallel;

     #ifdef TIME_BENCH_ENTIRE_SRS
        xbt_os_walltimer_start(timer);
     #endif

        smx_ctx_raw_runall_parallel();

     #ifdef TIME_BENCH_ENTIRE_SRS
        xbt_os_walltimer_stop(timer);
        elapsed = xbt_os_timer_elapsed(timer);
     #endif
    } else {
        XBT_DEBUG("Runall serial %lu", nb_processes);
        simix_global->context_factory->suspend = smx_ctx_raw_suspend_serial;

      #ifdef TIME_BENCH_PER_SR
        smx_ctx_raw_runall_serial(simix_global->process_to_run);
      #else

        #ifdef TIME_BENCH_ENTIRE_SRS
          xbt_os_walltimer_start(timer);
        #endif

        smx_ctx_raw_runall_serial();

        #ifdef TIME_BENCH_ENTIRE_SRS
          xbt_os_walltimer_stop(timer);
          elapsed = xbt_os_timer_elapsed(timer);
        #endif
      #endif
    }

#ifdef TIME_BENCH_ENTIRE_SRS
  XBT_CRITICAL("Total time SR %u = %f, %d", sr_count, elapsed, nb_processes);
#endif
}
#endif
