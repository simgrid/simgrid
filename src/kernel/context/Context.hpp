/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_CONTEXT_CONTEXT_HPP
#define SIMGRID_KERNEL_CONTEXT_CONTEXT_HPP

#include <simgrid/forward.h>
#include <xbt/parmap.h>

#include "src/kernel/activity/ActivityImpl.hpp"

#include <csignal>
#include <functional>

namespace simgrid {
namespace kernel {
namespace context {
extern unsigned stack_size;
extern unsigned guard_size;

class XBT_PUBLIC ContextFactory {
public:
  explicit ContextFactory()             = default;
  ContextFactory(const ContextFactory&) = delete;
  ContextFactory& operator=(const ContextFactory&) = delete;
  virtual ~ContextFactory();
  virtual Context* create_context(std::function<void()>&& code, actor::ActorImpl* actor) = 0;

  /** Turn the current thread into a simulation context */
  virtual Context* attach(actor::ActorImpl* actor);
  /** Turn the current thread into maestro (the old maestro becomes a regular actor) */
  virtual Context* create_maestro(std::function<void()>&& code, actor::ActorImpl* actor);

  virtual void run_all(std::vector<actor::ActorImpl*> const& actors_list) = 0;

  /* This allows Java to hijack the context factory (Java induces factories of factory :) */
  static std::function<ContextFactory*(void)> initializer;

protected:
  template <class T, class... Args> T* new_context(Args&&... args)
  {
    auto* context = new T(std::forward<Args>(args)...);
    context->declare_context(sizeof(T));
    return context;
  }
};

class XBT_PUBLIC Context {
  friend ContextFactory;

  static thread_local Context* current_context_;

  std::function<void()> code_;
  actor::ActorImpl* actor_ = nullptr;
  bool is_maestro_;
  void declare_context(std::size_t size);

public:
#ifndef WIN32
  static int install_sigsegv_stack(stack_t* old_stack, bool enable);
#endif

  Context(std::function<void()>&& code, actor::ActorImpl* actor, bool maestro);
  Context(const Context&) = delete;
  Context& operator=(const Context&) = delete;
  virtual ~Context();

  bool is_maestro() const { return is_maestro_; }
  void operator()() const { code_(); }
  bool has_code() const { return static_cast<bool>(code_); }
  actor::ActorImpl* get_actor() const { return this->actor_; }

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
  AttachContext(std::function<void()>&& code, actor::ActorImpl* actor, bool maestro)
      : Context(std::move(code), actor, maestro)
  {
  }
  AttachContext(const AttachContext&) = delete;
  AttachContext& operator=(const AttachContext&) = delete;
  ~AttachContext() override;

  /** Called by the context when it is ready to give control to the maestro */
  virtual void attach_start() = 0;

  /** Called by the context when it has finished its job */
  virtual void attach_stop() = 0;
};

XBT_PRIVATE ContextFactory* thread_factory();
XBT_PRIVATE ContextFactory* sysv_factory();
XBT_PRIVATE ContextFactory* raw_factory();
XBT_PRIVATE ContextFactory* boost_factory();

XBT_PUBLIC bool is_parallel();
XBT_PUBLIC int get_nthreads();
XBT_PUBLIC void set_nthreads(int nb_threads);
XBT_PUBLIC void set_parallel_mode(e_xbt_parmap_mode_t mode);
XBT_PUBLIC e_xbt_parmap_mode_t get_parallel_mode();
} // namespace context
} // namespace kernel
} // namespace simgrid

#endif
