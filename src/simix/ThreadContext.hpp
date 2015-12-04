/* Copyright (c) 2009-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* \file ThreadContext.hpp Context switching with native threads */

#ifndef SIMGRID_SIMIX_THREAD_CONTEXT_HPP
#define SIMGRID_SIMIX_THREAD_CONTEXT_HPP

#include <simgrid/simix.hpp>


namespace simgrid {
namespace simix {

class ThreadContext;
class ThreadContextFactory;

class ThreadContext : public Context {
public:
  friend ThreadContextFactory;
  ThreadContext(xbt_main_func_t code,
          int argc, char **argv,
          void_pfn_smxprocess_t cleanup_func,
          smx_process_t process);
  ~ThreadContext();
  void stop() override;
  void suspend() override;
private:
  /** A portable thread */
  xbt_os_thread_t thread_;
  /** Semaphore used to schedule/yield the process */
  xbt_os_sem_t begin_;
  /** Semaphore used to schedule/unschedule */
  xbt_os_sem_t end_;
private:
  static void* wrapper(void *param);
};

class ThreadContextFactory : public ContextFactory {
public:
  ThreadContextFactory();
  ~ThreadContextFactory();
  virtual ThreadContext* create_context(
    xbt_main_func_t, int, char **, void_pfn_smxprocess_t,
    smx_process_t process
    ) override;
  void run_all() override;
  ThreadContext* self() override;
};

}
}

#endif