/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** @file smx_context_boost.cpp Userspace context switching implementation based on Boost.Context */

#include <cstdint>

#include <boost/context/all.hpp>

#include "xbt/log.h"
#include "smx_private.h"
#include "src/internal_config.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_context);

typedef struct s_smx_ctx_boost {
  s_smx_ctx_base_t super;       /* Fields of super implementation */
#if HAVE_BOOST_CONTEXT == 1
  boost::context::fcontext_t* fc;
#else
  boost::context::fcontext_t fc;
#endif
  void* stack;
} s_smx_ctx_boost_t, *smx_ctx_boost_t;

static int smx_ctx_boost_factory_finalize(smx_context_factory_t *factory);
static smx_context_t
smx_ctx_boost_create_context(xbt_main_func_t code, int argc, char **argv,
    void_pfn_smxprocess_t cleanup_func, smx_process_t process);
static void smx_ctx_boost_free(smx_context_t context);

static void smx_ctx_boost_wrapper(std::intptr_t arg);

static void smx_ctx_boost_stop_serial(smx_context_t context);
static void smx_ctx_boost_suspend_serial(smx_context_t context);
static void smx_ctx_boost_resume_serial(smx_process_t first_process);
static void smx_ctx_boost_runall_serial(void);

#ifdef CONTEXT_THREADS
static void smx_ctx_boost_stop_parallel(smx_context_t context);
static void smx_ctx_boost_suspend_parallel(smx_context_t context);
static void smx_ctx_boost_resume_parallel(smx_process_t first_process);
static void smx_ctx_boost_runall_parallel(void);
#endif

#ifdef CONTEXT_THREADS
static xbt_parmap_t boost_parmap;
static smx_ctx_boost_t* boost_workers_context;
static unsigned long boost_threads_working;
static xbt_os_thread_key_t boost_worker_id_key;
#endif

static unsigned long boost_process_index = 0;
static smx_ctx_boost_t boost_maestro_context;

void SIMIX_ctx_boost_factory_init(smx_context_factory_t *factory)
{
  smx_ctx_base_factory_init(factory);
  XBT_VERB("Activating boost context factory");

  (*factory)->finalize = smx_ctx_boost_factory_finalize;
  (*factory)->create_context = smx_ctx_boost_create_context;
  /* Do not overload that method (*factory)->finalize */
  (*factory)->free = smx_ctx_boost_free;
  (*factory)->name = "smx_boost_context_factory";

  if (SIMIX_context_is_parallel()) {
#ifndef CONTEXT_THREADS
    THROWF(arg_error, 0, "No thread support for parallel context execution");
#else
    int nthreads = SIMIX_context_get_nthreads();
    boost_parmap = xbt_parmap_new(nthreads, SIMIX_context_get_parallel_mode());
    boost_workers_context = xbt_new(smx_ctx_boost_t, nthreads);
    boost_maestro_context = NULL;

    xbt_os_thread_key_create(&boost_worker_id_key);

    (*factory)->stop = smx_ctx_boost_stop_parallel;
    (*factory)->suspend = smx_ctx_boost_suspend_parallel;
    (*factory)->runall = smx_ctx_boost_runall_parallel;
#endif
  } else {
    (*factory)->stop = smx_ctx_boost_stop_serial;
    (*factory)->suspend = smx_ctx_boost_suspend_serial;
    (*factory)->runall = smx_ctx_boost_runall_serial;
  }
}

/* Initialization functions */

static int smx_ctx_boost_factory_finalize(smx_context_factory_t *factory)
{
#ifdef CONTEXT_THREADS
  if (boost_parmap) {
    xbt_parmap_destroy(boost_parmap);
    boost_parmap = nullptr;
  }
  xbt_free(boost_workers_context);
  boost_workers_context = nullptr;
#endif
  return smx_ctx_base_factory_finalize(factory);
}

static smx_context_t
smx_ctx_boost_create_context(xbt_main_func_t code, int argc, char **argv,
    void_pfn_smxprocess_t cleanup_func, smx_process_t process)
{
  smx_ctx_boost_t context =
      (smx_ctx_boost_t) smx_ctx_base_factory_create_context_sized(
          sizeof(s_smx_ctx_boost_t),
          code,
          argc,
          argv,
          cleanup_func,
          process);

  /* if the user provided a function for the process then use it,
     otherwise it is the context for maestro */
  if (code) {
    context->stack = SIMIX_context_stack_new();
    // We need to pass the bottom of the stack to make_fcontext,
    // depending on the stack direction it may be the lower or higher address:
#if PTH_STACKGROWTH == -1
    void* stack = (char*) context->stack + smx_context_usable_stack_size - 1;
#else
    void* stack = context->stack;
#endif
    context->fc = boost::context::make_fcontext(
                      stack,
                      smx_context_usable_stack_size,
                      smx_ctx_boost_wrapper);
  } else {
    context->stack = nullptr;
#if HAVE_BOOST_CONTEXT == 1
    context->fc = new boost::context::fcontext_t();
#endif
    if (boost_maestro_context == nullptr)
      boost_maestro_context = context;
  }

  return (smx_context_t) context;
}

static void smx_ctx_boost_free(smx_context_t c)
{
  smx_ctx_boost_t context = (smx_ctx_boost_t) c;
  if (!context)
    return;
#if HAVE_BOOST_CONTEXT == 1
  if (!context->stack)
    delete context->fc;
#endif
  if ((smx_ctx_boost_t) c == boost_maestro_context)
    boost_maestro_context = nullptr;
  SIMIX_context_stack_delete(context->stack);
  smx_ctx_base_free(c);
}

static void smx_ctx_boost_wrapper(std::intptr_t arg)
{
  smx_context_t context = (smx_context_t) arg;
  context->code(context->argc, context->argv);
  smx_ctx_boost_stop_serial(context);
}

static void smx_ctx_boost_stop_serial(smx_context_t context)
{
  smx_ctx_base_stop(context);
  simix_global->context_factory->suspend(context);
}

static void smx_ctx_boost_suspend_serial(smx_context_t context)
{
  /* determine the next context */
  smx_ctx_boost_t next_context;
  unsigned long int i = boost_process_index++;

  if (i < xbt_dynar_length(simix_global->process_to_run)) {
    /* execute the next process */
    XBT_DEBUG("Run next process");
    next_context = (smx_ctx_boost_t) xbt_dynar_get_as(
        simix_global->process_to_run, i, smx_process_t)->context;
  }
  else {
    /* all processes were run, return to maestro */
    XBT_DEBUG("No more process to run");
    next_context = (smx_ctx_boost_t) boost_maestro_context;
  }
  SIMIX_context_set_current((smx_context_t) next_context);
#if HAVE_BOOST_CONTEXT == 1
  boost::context::jump_fcontext(
    ((smx_ctx_boost_t)context)->fc, next_context->fc, (intptr_t)next_context);
#else
  boost::context::jump_fcontext(
    &((smx_ctx_boost_t)context)->fc, next_context->fc, (intptr_t)next_context);
#endif
}

static void smx_ctx_boost_resume_serial(smx_process_t first_process)
{
  smx_ctx_boost_t context = (smx_ctx_boost_t) first_process->context;
  SIMIX_context_set_current((smx_context_t) context);
#if HAVE_BOOST_CONTEXT == 1
  boost::context::jump_fcontext(boost_maestro_context->fc, context->fc,
    (intptr_t)context);
#else
  boost::context::jump_fcontext(&boost_maestro_context->fc, context->fc,
    (intptr_t)context);
#endif
}

static void smx_ctx_boost_runall_serial(void)
{
  smx_process_t first_process =
      xbt_dynar_get_as(simix_global->process_to_run, 0, smx_process_t);
  boost_process_index = 1;

  /* execute the first process */
  smx_ctx_boost_resume_serial(first_process);
}

// **** Parallel code

#ifdef CONTEXT_THREADS

static void smx_ctx_boost_stop_parallel(smx_context_t context)
{
  smx_ctx_base_stop(context);
  smx_ctx_boost_suspend_parallel(context);
}

static void smx_ctx_boost_suspend_parallel(smx_context_t context)
{
  smx_process_t next_work = (smx_process_t) xbt_parmap_next(boost_parmap);
  smx_ctx_boost_t next_context;

  if (next_work != NULL) {
    XBT_DEBUG("Run next process");
    next_context = (smx_ctx_boost_t) next_work->context;
  }
  else {
    XBT_DEBUG("No more processes to run");
    unsigned long worker_id =
        (unsigned long) xbt_os_thread_get_specific(boost_worker_id_key);
    next_context = boost_workers_context[worker_id];
  }

  SIMIX_context_set_current((smx_context_t) next_context);
#if HAVE_BOOST_CONTEXT == 1
  boost::context::jump_fcontext(
    ((smx_ctx_boost_t)context)->fc, next_context->fc, (intptr_t)next_context);
#else
  boost::context::jump_fcontext(
    &((smx_ctx_boost_t)context)->fc, next_context->fc, (intptr_t)next_context);
#endif
}

static void smx_ctx_boost_resume_parallel(smx_process_t process)
{
  unsigned long worker_id = __sync_fetch_and_add(&boost_threads_working, 1);
  xbt_os_thread_set_specific(boost_worker_id_key, (void*) worker_id);

  smx_ctx_boost_t worker_context = (smx_ctx_boost_t)SIMIX_context_self();
  boost_workers_context[worker_id] = worker_context;
  smx_ctx_boost_t context = (smx_ctx_boost_t) process->context;

  SIMIX_context_set_current((smx_context_t) context);
#if HAVE_BOOST_CONTEXT == 1
  boost::context::jump_fcontext(worker_context->fc, context->fc,
    (intptr_t)context);
#else
  boost::context::jump_fcontext(&worker_context->fc, context->fc,
    (intptr_t)context);
#endif
}

static void smx_ctx_boost_runall_parallel(void)
{
  boost_threads_working = 0;
  xbt_parmap_apply(boost_parmap, (void_f_pvoid_t) smx_ctx_boost_resume_parallel,
    simix_global->process_to_run);
}

#endif
