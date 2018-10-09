/* a fast and simple context switching library                              */

/* Copyright (c) 2009-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/modelchecker.h"
#include "src/internal_config.h"
#include "src/simix/smx_private.hpp"
#include "xbt/config.hpp"

#include <thread>

#ifdef _WIN32
#include <windows.h>
#include <malloc.h>
#else
#include <sys/mman.h>
#endif

#ifdef __MINGW32__
#define _aligned_malloc __mingw_aligned_malloc
#define _aligned_free  __mingw_aligned_free
#endif /*MINGW*/

#if HAVE_VALGRIND_H
# include <valgrind/valgrind.h>
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_context, simix, "Context switching mechanism");

static std::pair<const char*, simgrid::kernel::context::ContextFactoryInitializer> context_factories[] = {
#if HAVE_RAW_CONTEXTS
  { "raw", &simgrid::kernel::context::raw_factory },
#endif
#if HAVE_UCONTEXT_CONTEXTS
  { "ucontext", &simgrid::kernel::context::sysv_factory },
#endif
#if HAVE_BOOST_CONTEXTS
  { "boost", &simgrid::kernel::context::boost_factory },
#endif
#if HAVE_THREAD_CONTEXTS
  { "thread", &simgrid::kernel::context::thread_factory },
#endif
};

static_assert(sizeof(context_factories) != 0, "No context factories are enabled for this build");

// Create the list of possible contexts:
static inline
std::string contexts_list()
{
  std::string res;
  const std::size_t n = sizeof(context_factories) / sizeof(context_factories[0]);
  for (std::size_t i = 1; i != n; ++i) {
    res += ", ";
    res += context_factories[i].first;
  }
  return res;
}

static simgrid::config::Flag<std::string> context_factory_name(
  "contexts/factory",
  (std::string("Possible values: ")+contexts_list()).c_str(),
  context_factories[0].first);

unsigned smx_context_stack_size;
int smx_context_stack_size_was_set = 0;
unsigned smx_context_guard_size;
int smx_context_guard_size_was_set = 0;
static thread_local smx_context_t smx_current_context_parallel;
static smx_context_t smx_current_context_serial;
static int smx_parallel_contexts = 1;
static int smx_parallel_threshold = 2;
static e_xbt_parmap_mode_t smx_parallel_synchronization_mode = XBT_PARMAP_DEFAULT;

/**
 * This function is called by SIMIX_global_init() to initialize the context module.
 */
void SIMIX_context_mod_init()
{
  xbt_assert(simix_global->context_factory == nullptr);

  smx_context_stack_size_was_set = not simgrid::config::is_default("contexts/stack-size");
  smx_context_guard_size_was_set = not simgrid::config::is_default("contexts/guard-size");

#if HAVE_SMPI && (defined(__APPLE__) || defined(__NetBSD__))
  std::string priv = simgrid::config::get_value<std::string>("smpi/privatization");
  if (context_factory_name == "thread" && (priv == "dlopen" || priv == "yes" || priv == "default" || priv == "1")) {
    XBT_WARN("dlopen+thread broken on Apple and BSD. Switching to raw contexts.");
    context_factory_name = "raw";
  }
#endif

#if HAVE_SMPI && defined(__FreeBSD__)
  if (context_factory_name == "thread" && simgrid::config::get_value<std::string>("smpi/privatization") != "no") {
    XBT_WARN("mmap broken on FreeBSD, but dlopen+thread broken too. Switching to dlopen+raw contexts.");
    context_factory_name = "raw";
  }
#endif

  /* select the context factory to use to create the contexts */
  if (simgrid::kernel::context::factory_initializer != nullptr) { // Give Java a chance to hijack the factory mechanism
    simix_global->context_factory = simgrid::kernel::context::factory_initializer();
    return;
  }
  /* use the factory specified by --cfg=contexts/factory:value */
  for (auto const& factory : context_factories)
    if (context_factory_name == factory.first) {
      simix_global->context_factory = factory.second();
      break;
    }

  if (simix_global->context_factory == nullptr) {
    XBT_ERROR("Invalid context factory specified. Valid factories on this machine:");
#if HAVE_RAW_CONTEXTS
    XBT_ERROR("  raw: high performance context factory implemented specifically for SimGrid");
#else
    XBT_ERROR("  (raw contexts were disabled at compilation time on this machine -- check configure logs for details)");
#endif
#if HAVE_UCONTEXT_CONTEXTS
    XBT_ERROR("  ucontext: classical system V contexts (implemented with makecontext, swapcontext and friends)");
#else
    XBT_ERROR("  (ucontext was disabled at compilation time on this machine -- check configure logs for details)");
#endif
#if HAVE_BOOST_CONTEXTS
    XBT_ERROR("  boost: this uses the boost libraries context implementation");
#else
    XBT_ERROR("  (boost was disabled at compilation time on this machine -- check configure logs for details. Did you install the libboost-context-dev package?)");
#endif
    XBT_ERROR("  thread: slow portability layer using pthreads as provided by gcc");
    xbt_die("Please use a valid factory.");
  }
}

/**
 * This function is called by SIMIX_clean() to finalize the context module.
 */
void SIMIX_context_mod_exit()
{
  delete simix_global->context_factory;
  simix_global->context_factory = nullptr;
}

void *SIMIX_context_stack_new()
{
  void *stack;

  if (smx_context_guard_size > 0 && not MC_is_active()) {

#if !defined(PTH_STACKGROWTH) || (PTH_STACKGROWTH != -1)
    xbt_die("Stack overflow protection is known to be broken on your system: you stacks grow upwards (or detection is "
            "broken). "
            "Please disable stack guards with --cfg=contexts:guard-size:0");
    /* Current code for stack overflow protection assumes that stacks are growing downward (PTH_STACKGROWTH == -1).
     * Protected pages need to be put after the stack when PTH_STACKGROWTH == 1. */
#endif

    size_t size = smx_context_stack_size + smx_context_guard_size;
#if SIMGRID_HAVE_MC
    /* Cannot use posix_memalign when SIMGRID_HAVE_MC. Align stack by hand, and save the
     * pointer returned by xbt_malloc0. */
    char *alloc = (char*)xbt_malloc0(size + xbt_pagesize);
    stack = alloc - ((uintptr_t)alloc & (xbt_pagesize - 1)) + xbt_pagesize;
    *((void **)stack - 1) = alloc;
#elif !defined(_WIN32)
    if (posix_memalign(&stack, xbt_pagesize, size) != 0)
      xbt_die("Failed to allocate stack.");
#else
    stack = _aligned_malloc(size, xbt_pagesize);
#endif

#ifndef _WIN32
    if (mprotect(stack, smx_context_guard_size, PROT_NONE) == -1) {
      xbt_die(
          "Failed to protect stack: %s.\n"
          "If you are running a lot of actors, you may be exceeding the amount of mappings allowed per process.\n"
          "On Linux systems, change this value with sudo sysctl -w vm.max_map_count=newvalue (default value: 65536)\n"
          "Please see http://simgrid.gforge.inria.fr/simgrid/latest/doc/html/options.html#options_virt for more info.",
          strerror(errno));
      /* This is fatal. We are going to fail at some point when we try reusing this. */
    }
#endif
    stack = (char *)stack + smx_context_guard_size;
  } else {
    stack = xbt_malloc0(smx_context_stack_size);
  }

#if HAVE_VALGRIND_H
  unsigned int valgrind_stack_id = VALGRIND_STACK_REGISTER(stack, (char *)stack + smx_context_stack_size);
  memcpy((char *)stack + smx_context_usable_stack_size, &valgrind_stack_id, sizeof valgrind_stack_id);
#endif

  return stack;
}

void SIMIX_context_stack_delete(void *stack)
{
  if (not stack)
    return;

#if HAVE_VALGRIND_H
  unsigned int valgrind_stack_id;
  memcpy(&valgrind_stack_id, (char *)stack + smx_context_usable_stack_size, sizeof valgrind_stack_id);
  VALGRIND_STACK_DEREGISTER(valgrind_stack_id);
#endif

#ifndef _WIN32
  if (smx_context_guard_size > 0 && not MC_is_active()) {
    stack = (char *)stack - smx_context_guard_size;
    if (mprotect(stack, smx_context_guard_size, PROT_READ | PROT_WRITE) == -1) {
      XBT_WARN("Failed to remove page protection: %s", strerror(errno));
      /* try to pursue anyway */
    }
#if SIMGRID_HAVE_MC
    /* Retrieve the saved pointer.  See SIMIX_context_stack_new above. */
    stack = *((void **)stack - 1);
#endif
  }
#endif /* not windows */

  xbt_free(stack);
}

/** @brief Returns whether some parallel threads are used for the user contexts. */
int SIMIX_context_is_parallel() {
  return smx_parallel_contexts > 1;
}

/**
 * @brief Returns the number of parallel threads used for the user contexts.
 * @return the number of threads (1 means no parallelism)
 */
int SIMIX_context_get_nthreads() {
  return smx_parallel_contexts;
}

/**
 * @brief Sets the number of parallel threads to use
 * for the user contexts.
 *
 * This function should be called before initializing SIMIX.
 * A value of 1 means no parallelism (1 thread only).
 * If the value is greater than 1, the thread support must be enabled.
 *
 * @param nb_threads the number of threads to use
 */
void SIMIX_context_set_nthreads(int nb_threads) {
  if (nb_threads<=0) {
    nb_threads = std::thread::hardware_concurrency();
    XBT_INFO("Auto-setting contexts/nthreads to %d", nb_threads);
  }
#if !HAVE_THREAD_CONTEXTS
  xbt_assert(nb_threads == 1, "Parallel runs are impossible when the pthreads are missing.");
#endif
  smx_parallel_contexts = nb_threads;
}

/**
 * @brief Returns the threshold above which user processes are run in parallel.
 *
 * If the number of threads is set to 1, there is no parallelism and this
 * threshold has no effect.
 *
 * @return when the number of user processes ready to run is above
 * this threshold, they are run in parallel
 */
int SIMIX_context_get_parallel_threshold() {
  return smx_parallel_threshold;
}

/**
 * @brief Sets the threshold above which user processes are run in parallel.
 *
 * If the number of threads is set to 1, there is no parallelism and this
 * threshold has no effect.
 *
 * @param threshold when the number of user processes ready to run is above
 * this threshold, they are run in parallel
 */
void SIMIX_context_set_parallel_threshold(int threshold) {
  smx_parallel_threshold = threshold;
}

/**
 * @brief Returns the synchronization mode used when processes are run in
 * parallel.
 * @return how threads are synchronized if processes are run in parallel
 */
e_xbt_parmap_mode_t SIMIX_context_get_parallel_mode() {
  return smx_parallel_synchronization_mode;
}

/**
 * @brief Sets the synchronization mode to use when processes are run in
 * parallel.
 * @param mode how to synchronize threads if processes are run in parallel
 */
void SIMIX_context_set_parallel_mode(e_xbt_parmap_mode_t mode) {
  smx_parallel_synchronization_mode = mode;
}

/**
 * @brief Returns the current context of this thread.
 * @return the current context of this thread
 */
smx_context_t SIMIX_context_get_current()
{
  if (SIMIX_context_is_parallel()) {
    return smx_current_context_parallel;
  }
  else {
    return smx_current_context_serial;
  }
}

/**
 * @brief Sets the current context of this thread.
 * @param context the context to set
 */
void SIMIX_context_set_current(smx_context_t context)
{
  if (SIMIX_context_is_parallel()) {
    smx_current_context_parallel = context;
  }
  else {
    smx_current_context_serial = context;
  }
}
