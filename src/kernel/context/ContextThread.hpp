/* Copyright (c) 2009-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* \file ThreadContext.hpp Context switching with native threads */

#ifndef SIMGRID_KERNEL_CONTEXT_THREAD_CONTEXT_HPP
#define SIMGRID_KERNEL_CONTEXT_THREAD_CONTEXT_HPP

#include "simgrid/simix.hpp"
#include "src/kernel/context/Context.hpp"
#include "src/xbt/OsSemaphore.hpp"

#include <thread>

namespace simgrid {
namespace kernel {
namespace context {

class XBT_PUBLIC ThreadContext : public AttachContext {
public:
  ThreadContext(std::function<void()>&& code, actor::ActorImpl* actor, bool maestro);
  ThreadContext(const ThreadContext&) = delete;
  ThreadContext& operator=(const ThreadContext&) = delete;
  ~ThreadContext() override;
  void stop() override;
  void suspend() override;
  void attach_start() override;
  void attach_stop() override;

  bool is_maestro() const { return is_maestro_; }
  void release(); // unblock context's start()
  void wait();    // wait for context's yield()

private:
  /** A portable thread */
  std::thread* thread_ = nullptr;
  /** Semaphore used to schedule/yield the actor (not needed when the maestro is in main, but harmless then) */
  xbt::OsSemaphore begin_{0};
  /** Semaphore used to schedule/unschedule (not needed when the maestro is in main, but harmless then) */
  xbt::OsSemaphore end_{0};
  bool is_maestro_;

  void start();                // match a call to release()
  void yield();                // match a call to yield()
  virtual void start_hook() { /* empty placeholder, called after start(). Used in parallel mode and Java */}
  virtual void yield_hook() { /* empty placeholder, called before yield(). Used in parallel mode */}
  virtual void stop_hook() { /* empty placeholder, called at stop(). Used in Java */}

  static void wrapper(ThreadContext* context);
};

class XBT_PUBLIC SerialThreadContext : public ThreadContext {
public:
  SerialThreadContext(std::function<void()>&& code, actor::ActorImpl* actor, bool maestro)
      : ThreadContext(std::move(code), actor, maestro)
  {
  }

  static void run_all();
};

class ParallelThreadContext : public ThreadContext {
public:
  ParallelThreadContext(std::function<void()>&& code, actor::ActorImpl* actor, bool maestro)
      : ThreadContext(std::move(code), actor, maestro)
  {
  }

  static void initialize();
  static void finalize();
  static void run_all();

private:
  static xbt::OsSemaphore* thread_sem_;

  void start_hook() override;
  void yield_hook() override;
};

class ThreadContextFactory : public ContextFactory {
public:
  ThreadContextFactory();
  ThreadContextFactory(const ThreadContextFactory&) = delete;
  ThreadContextFactory& operator=(const ThreadContextFactory&) = delete;
  ~ThreadContextFactory() override;
  ThreadContext* create_context(std::function<void()>&& code, actor::ActorImpl* actor) override
  {
    bool maestro = not code;
    return create_context(std::move(code), actor, maestro);
  }
  void run_all() override;

  // Optional methods:
  ThreadContext* attach(actor::ActorImpl* actor) override
  {
    return create_context(std::function<void()>(), actor, false);
  }
  ThreadContext* create_maestro(std::function<void()>&& code, actor::ActorImpl* actor) override
  {
    return create_context(std::move(code), actor, true);
  }

private:
  ThreadContext* create_context(std::function<void()>&& code, actor::ActorImpl* actor, bool maestro);
};
} // namespace context
} // namespace kernel
} // namespace simgrid

#endif
