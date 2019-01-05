/* Copyright (c) 2009-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/modelchecker.h"
#include "src/internal_config.h"
#include "src/kernel/context/context_private.hpp"
#include "src/simix/ActorImpl.hpp"
#include "src/simix/smx_private.hpp"
#include "xbt/parmap.hpp"

#include "src/kernel/context/ContextSwapped.hpp"

#ifdef _WIN32
#include <malloc.h>
#include <windows.h>
#else
#include <sys/mman.h>
#endif

#ifdef __MINGW32__
#define _aligned_malloc __mingw_aligned_malloc
#define _aligned_free __mingw_aligned_free
#endif /*MINGW*/

#if HAVE_VALGRIND_H
#include <valgrind/valgrind.h>
#endif

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_context);

namespace simgrid {
namespace kernel {
namespace context {

/* Sequential execution */
unsigned long SwappedContext::process_index_;

/* Parallel execution */
simgrid::xbt::Parmap<smx_actor_t>* SwappedContext::parmap_;
std::atomic<uintptr_t> SwappedContext::threads_working_;       /* number of threads that have started their work */
thread_local uintptr_t SwappedContext::worker_id_;             /* thread-specific storage for the thread id */
std::vector<SwappedContext*> SwappedContext::workers_context_; /* space to save the worker's context in each thread */

void SwappedContext::initialize()
{
  parmap_ = nullptr;
  workers_context_.clear();
  workers_context_.resize(SIMIX_context_get_nthreads(), nullptr);
}

void SwappedContext::finalize()
{
  delete parmap_;
  parmap_ = nullptr;
  workers_context_.clear();
}

SwappedContext* SwappedContext::maestro_context_ = nullptr;

SwappedContext::SwappedContext(std::function<void()> code, void_pfn_smxprocess_t cleanup_func, smx_actor_t process)
    : Context(std::move(code), cleanup_func, process)
{
  if (has_code()) {
    if (smx_context_guard_size > 0 && not MC_is_active()) {

#if !defined(PTH_STACKGROWTH) || (PTH_STACKGROWTH != -1)
      xbt_die(
          "Stack overflow protection is known to be broken on your system: you stacks grow upwards (or detection is "
          "broken). "
          "Please disable stack guards with --cfg=contexts:guard-size:0");
      /* Current code for stack overflow protection assumes that stacks are growing downward (PTH_STACKGROWTH == -1).
       * Protected pages need to be put after the stack when PTH_STACKGROWTH == 1. */
#endif

      size_t size = smx_context_stack_size + smx_context_guard_size;
#if SIMGRID_HAVE_MC
      /* Cannot use posix_memalign when SIMGRID_HAVE_MC. Align stack by hand, and save the
       * pointer returned by xbt_malloc0. */
      char* alloc           = (char*)xbt_malloc0(size + xbt_pagesize);
      stack_                = alloc - ((uintptr_t)alloc & (xbt_pagesize - 1)) + xbt_pagesize;
      *((void**)stack_ - 1) = alloc;
#elif !defined(_WIN32)
      if (posix_memalign(&this->stack_, xbt_pagesize, size) != 0)
        xbt_die("Failed to allocate stack.");
#else
      this->stack_ = _aligned_malloc(size, xbt_pagesize);
#endif

#ifndef _WIN32
      if (mprotect(this->stack_, smx_context_guard_size, PROT_NONE) == -1) {
        xbt_die(
            "Failed to protect stack: %s.\n"
            "If you are running a lot of actors, you may be exceeding the amount of mappings allowed per process.\n"
            "On Linux systems, change this value with sudo sysctl -w vm.max_map_count=newvalue (default value: 65536)\n"
            "Please see http://simgrid.gforge.inria.fr/simgrid/latest/doc/html/options.html#options_virt for more "
            "info.",
            strerror(errno));
        /* This is fatal. We are going to fail at some point when we try reusing this. */
      }
#endif
      this->stack_ = (char*)this->stack_ + smx_context_guard_size;
    } else {
      this->stack_ = xbt_malloc0(smx_context_stack_size);
    }

#if HAVE_VALGRIND_H
    unsigned int valgrind_stack_id =
        VALGRIND_STACK_REGISTER(this->stack_, (char*)this->stack_ + smx_context_stack_size);
    memcpy((char*)this->stack_ + smx_context_usable_stack_size, &valgrind_stack_id, sizeof valgrind_stack_id);
#endif
  }
}

SwappedContext::~SwappedContext()
{
  if (stack_ == nullptr)
    return;

#if HAVE_VALGRIND_H
  unsigned int valgrind_stack_id;
  memcpy(&valgrind_stack_id, (char*)stack_ + smx_context_usable_stack_size, sizeof valgrind_stack_id);
  VALGRIND_STACK_DEREGISTER(valgrind_stack_id);
#endif

#ifndef _WIN32
  if (smx_context_guard_size > 0 && not MC_is_active()) {
    stack_ = (char*)stack_ - smx_context_guard_size;
    if (mprotect(stack_, smx_context_guard_size, PROT_READ | PROT_WRITE) == -1) {
      XBT_WARN("Failed to remove page protection: %s", strerror(errno));
      /* try to pursue anyway */
    }
#if SIMGRID_HAVE_MC
    /* Retrieve the saved pointer.  See SIMIX_context_stack_new above. */
    stack_ = *((void**)stack_ - 1);
#endif
  }
#endif /* not windows */

  xbt_free(stack_);
}

void SwappedContext::suspend()
{
  /* determine the next context */
  SwappedContext* next_context;
  unsigned long int i = process_index_;
  process_index_++;

  if (i < simix_global->process_to_run.size()) {
    /* execute the next process */
    XBT_DEBUG("Run next process");
    next_context = static_cast<SwappedContext*>(simix_global->process_to_run[i]->context_);
  } else {
    /* all processes were run, return to maestro */
    XBT_DEBUG("No more process to run");
    next_context = static_cast<SwappedContext*>(get_maestro());
  }
  Context::set_current(next_context);
  this->swap_into(next_context);
}

void SwappedContext::resume()
{
  SwappedContext* old = static_cast<SwappedContext*>(self());
  Context::set_current(this);
  old->swap_into(this);
}

void SwappedContext::run_all()
{
  if (simix_global->process_to_run.empty())
    return;
  smx_actor_t first_process = simix_global->process_to_run.front();
  process_index_            = 1;
  /* execute the first process */
  static_cast<SwappedContext*>(first_process->context_)->resume();
}
} // namespace context
} // namespace kernel
} // namespace simgrid
