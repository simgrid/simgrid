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
  virtual Context* create_context(std::function<void()>&& code, actor::ActorImpl* actor) = 0;

  /** Turn the current thread into a simulation context */
  virtual Context* attach(actor::ActorImpl* actor);
  /** Turn the current thread into maestro (the old maestro becomes a regular actor) */
  virtual Context* create_maestro(std::function<void()>&& code, actor::ActorImpl* actor);

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
  actor::ActorImpl* actor_ = nullptr;
  void declare_context(std::size_t size);

public:
  bool iwannadie = false;

  Context(std::function<void()>&& code, actor::ActorImpl* actor);
  Context(const Context&) = delete;
  Context& operator=(const Context&) = delete;
  virtual ~Context();

  void operator()() { code_(); }
  bool has_code() const { return static_cast<bool>(code_); }
  actor::ActorImpl* get_actor() { return this->actor_; }

  // Scheduling methods
  virtual void stop();
  virtual void suspend() = 0;

  // Retrieving the self() context
  /** @brief Retrieves the current context of this thread */
  static Context* self();
  /** @brief Sets the current context of this thread */
  static void set_current(Context* self);
};

class XBT_PUBLIC AttachContext : public Context {
public:
  AttachContext(std::function<void()>&& code, actor::ActorImpl* actor) : Context(std::move(code), actor) {}
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


/* This allows Java to hijack the context factory (Java induces factories of factory :) */
typedef ContextFactory* (*ContextFactoryInitializer)();
XBT_PUBLIC_DATA ContextFactoryInitializer factory_initializer;

XBT_PRIVATE ContextFactory* thread_factory();
XBT_PRIVATE ContextFactory* sysv_factory();
XBT_PRIVATE ContextFactory* raw_factory();
XBT_PRIVATE ContextFactory* boost_factory();

} // namespace context
} // namespace kernel
} // namespace simgrid

typedef simgrid::kernel::context::ContextFactory *smx_context_factory_t;

XBT_PRIVATE void SIMIX_context_mod_init();
XBT_PRIVATE void SIMIX_context_mod_exit();

#ifndef WIN32
XBT_PUBLIC_DATA unsigned char sigsegv_stack[SIGSTKSZ];
#endif

XBT_PRIVATE simgrid::simix::ActorCodeFactory& SIMIX_get_actor_code_factory(const std::string& name);

#endif
