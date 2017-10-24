/* Copyright (c) 2009-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* \file ThreadContext.hpp Context switching with native threads */

#ifndef SIMGRID_SIMIX_THREAD_CONTEXT_HPP
#define SIMGRID_SIMIX_THREAD_CONTEXT_HPP

#include <simgrid/simix.hpp>


namespace simgrid {
namespace kernel {
namespace context {

class ThreadContext : public AttachContext {
public:
  ThreadContext(std::function<void()> code, void_pfn_smxprocess_t cleanup_func, smx_actor_t process, bool maestro);
  ~ThreadContext() override;
  void stop() override;
  void suspend() override;
  void attach_start() override;
  void attach_stop() override;

  bool isMaestro() const { return is_maestro_; }
  void release(); // unblock context's start()
  void wait();    // wait for context's yield()

private:
  /** A portable thread */
  xbt_os_thread_t thread_ = nullptr;
  /** Semaphore used to schedule/yield the process */
  xbt_os_sem_t begin_ = nullptr;
  /** Semaphore used to schedule/unschedule */
  xbt_os_sem_t end_ = nullptr;
  bool is_maestro_;

  void start();                // match a call to release()
  void yield();                // match a call to yield()
  virtual void start_hook() { /* empty placeholder, called after start() */}
  virtual void yield_hook() { /* empty placeholder, called before yield() */}

  static void* wrapper(void *param);
};

class SerialThreadContext : public ThreadContext {
public:
  SerialThreadContext(std::function<void()> code, void_pfn_smxprocess_t cleanup_func, smx_actor_t process, bool maestro)
      : ThreadContext(std::move(code), cleanup_func, process, maestro)
  {
  }

  static void run_all();
};

class ParallelThreadContext : public ThreadContext {
public:
  ParallelThreadContext(std::function<void()> code, void_pfn_smxprocess_t cleanup_func, smx_actor_t process,
                        bool maestro)
      : ThreadContext(std::move(code), cleanup_func, process, maestro)
  {
  }

  static void initialize();
  static void finalize();
  static void run_all();

private:
  static xbt_os_sem_t thread_sem_;

  void start_hook() override;
  void yield_hook() override;
};

class ThreadContextFactory : public ContextFactory {
public:
  ThreadContextFactory();
  ~ThreadContextFactory() override;
  ThreadContext* create_context(std::function<void()> code, void_pfn_smxprocess_t cleanup_func,
                                smx_actor_t process) override
  {
    bool maestro = not code;
    return create_context(std::move(code), cleanup_func, process, maestro);
  }
  void run_all() override;
  ThreadContext* self() override { return static_cast<ThreadContext*>(xbt_os_thread_get_extra_data()); }

  // Optional methods:
  ThreadContext* attach(void_pfn_smxprocess_t cleanup_func, smx_actor_t process) override
  {
    return create_context(std::function<void()>(), cleanup_func, process, false);
  }
  ThreadContext* create_maestro(std::function<void()> code, smx_actor_t process) override
  {
    return create_context(std::move(code), nullptr, process, true);
  }

private:
  bool parallel_;

  ThreadContext* create_context(std::function<void()> code, void_pfn_smxprocess_t cleanup_func, smx_actor_t process,
                                bool maestro);
};
}}} // namespace

#endif
