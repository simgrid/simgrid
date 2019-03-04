/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_CONTEXT_CONTEXT_HPP
#define SIMGRID_KERNEL_CONTEXT_CONTEXT_HPP

#include "simgrid/forward.h"
#include "src/kernel/activity/ActivityImpl.hpp"

#include <csignal>
#include <functional>

namespace simgrid {
namespace kernel {
namespace context {

class XBT_PUBLIC ContextFactory {
public:
  explicit ContextFactory() {}
  ContextFactory(const ContextFactory&) = delete;
  ContextFactory& operator=(const ContextFactory&) = delete;
  virtual ~ContextFactory();
  virtual Context* create_context(std::function<void()> code, smx_actor_t actor) = 0;

  /** Turn the current thread into a simulation context */
  virtual Context* attach(smx_actor_t actor);
  /** Turn the current thread into maestro (the old maestro becomes a regular actor) */
  virtual Context* create_maestro(std::function<void()> code, smx_actor_t actor);

  virtual void run_all() = 0;

protected:
  template <class T, class... Args> T* new_context(Args&&... args)
  {
    T* context = new T(std::forward<Args>(args)...);
    context->declare_context(sizeof(T));
    return context;
  }
};

class XBT_PUBLIC Context {
  friend ContextFactory;

  std::function<void()> code_;
  smx_actor_t actor_                  = nullptr;
  void declare_context(std::size_t size);

public:
  bool iwannadie = false;

  Context(std::function<void()> code, smx_actor_t actor);
  Context(const Context&) = delete;
  Context& operator=(const Context&) = delete;
  virtual ~Context();

  void operator()() { code_(); }
  bool has_code() const { return static_cast<bool>(code_); }
  smx_actor_t get_actor() { return this->actor_; }

  // Scheduling methods
  virtual void stop();
  virtual void suspend() = 0;

  // Retrieving the self() context
  /** @brief Retrives the current context of this thread */
  static Context* self();
  /** @brief Sets the current context of this thread */
  static void set_current(Context* self);
};

class XBT_PUBLIC AttachContext : public Context {
public:
  AttachContext(std::function<void()> code, smx_actor_t actor) : Context(std::move(code), actor) {}
  AttachContext(const AttachContext&) = delete;
  AttachContext& operator=(const AttachContext&) = delete;
  ~AttachContext() override;

  /** Called by the context when it is ready to give control
   *  to the maestro.
   */
  virtual void attach_start() = 0;

  /** Called by the context when it has finished its job */
  virtual void attach_stop() = 0;
};

class XBT_PUBLIC ForcefulKillException {
  /** @brief Exception launched to kill an actor; DO NOT BLOCK IT!
   *
   * This exception is thrown whenever the actor's host is turned off. The actor stack is properly unwinded to release
   * all objects allocated on the stack (RAII powa).
   *
   * You may want to catch this exception to perform some extra cleanups in your simulation, but YOUR ACTORS MUST NEVER
   * SURVIVE a ForcefulKillException, or your simulation will segfault.
   *
   * @verbatim
   * void* payload = malloc(512);
   *
   * try {
   *   simgrid::s4u::this_actor::execute(100000);
   * } catch (simgrid::kernel::context::ForcefulKillException& e) { // oops, my host just turned off
   *   free(malloc);
   *   throw; // I shall never survive on an host that was switched off
   * }
   * @endverbatim
   *
   * Nope, Sonar, this should not inherit of std::exception nor of simgrid::Exception.
   * Otherwise, users may accidentally catch it with a try {} catch (std::exception)
   */
public:
  ForcefulKillException() = default;
  explicit ForcefulKillException(const std::string& msg) : msg_(std::string("Actor killed (") + msg + std::string(")."))
  {
  }
  ~ForcefulKillException();
  const char* what() const noexcept { return msg_.c_str(); }

  static void do_throw();
  static bool try_n_catch(std::function<void(void)> try_block);

private:
  std::string msg_ = std::string("Actor killed.");
};

/* This allows Java to hijack the context factory (Java induces factories of factory :) */
typedef ContextFactory* (*ContextFactoryInitializer)();
XBT_PUBLIC_DATA ContextFactoryInitializer factory_initializer;

XBT_PRIVATE ContextFactory* thread_factory();
XBT_PRIVATE ContextFactory* sysv_factory();
XBT_PRIVATE ContextFactory* raw_factory();
XBT_PRIVATE ContextFactory* boost_factory();

}}}

typedef simgrid::kernel::context::ContextFactory *smx_context_factory_t;

XBT_PRIVATE void SIMIX_context_mod_init();
XBT_PRIVATE void SIMIX_context_mod_exit();

#ifndef WIN32
XBT_PUBLIC_DATA char sigsegv_stack[SIGSTKSZ];
#endif

/** @brief Executes all the processes to run (in parallel if possible). */
XBT_PRIVATE void SIMIX_context_runall();

XBT_PUBLIC int SIMIX_process_get_maxpid();

XBT_PRIVATE simgrid::simix::ActorCodeFactory& SIMIX_get_actor_code_factory(const std::string& name);

#endif
