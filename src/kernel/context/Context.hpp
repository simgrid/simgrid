/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_CONTEXT_CONTEXT_HPP
#define SIMGRID_KERNEL_CONTEXT_CONTEXT_HPP

#include "simgrid/forward.h"
#include "src/internal_config.h"
#include "src/kernel/activity/ActivityImpl.hpp"

#include <csignal>

/* Process creation/destruction callbacks */
typedef void (*void_pfn_smxprocess_t)(smx_actor_t);

namespace simgrid {
namespace kernel {
namespace context {

class XBT_PUBLIC ContextFactory {
public:
  explicit ContextFactory() {}
  virtual ~ContextFactory();
  virtual Context* create_context(std::function<void()> code, void_pfn_smxprocess_t cleanup, smx_actor_t process) = 0;

  /** Turn the current thread into a simulation context */
  virtual Context* attach(void_pfn_smxprocess_t cleanup_func, smx_actor_t process);
  /** Turn the current thread into maestro (the old maestro becomes a regular actor) */
  virtual Context* create_maestro(std::function<void()> code, smx_actor_t process);

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

private:
  std::function<void()> code_;
  void_pfn_smxprocess_t cleanup_func_ = nullptr;
  smx_actor_t actor_                  = nullptr;
  void declare_context(std::size_t size);

public:
  bool iwannadie = false;

  Context(std::function<void()> code, void_pfn_smxprocess_t cleanup_func, smx_actor_t actor);
  Context(const Context&) = delete;
  Context& operator=(const Context&) = delete;
  virtual ~Context();

  void operator()() { code_(); }
  bool has_code() const { return static_cast<bool>(code_); }
  smx_actor_t get_actor() { return this->actor_; }
  void set_cleanup(void_pfn_smxprocess_t cleanup) { cleanup_func_ = cleanup; }

  // Scheduling methods
  virtual void stop();
  virtual void suspend() = 0;

  // Retrieving the self() context
  /** @brief Retrives the current context of this thread */
  static Context* self();
  /** @brief Sets the current context of this thread */
  static void set_current(Context* self);

  class StopRequest {
    /** @brief Exception launched to kill a process, in order to properly unwind its stack and release RAII stuff
     *
     * Nope, Sonar, this should not inherit of std::exception nor of simgrid::Exception.
     * Otherwise, users may accidentally catch it with a try {} catch (std::exception)
     */
  public:
    StopRequest() = default;
    explicit StopRequest(std::string msg) : msg_(std::string("Actor killed (") + msg + std::string(").")) {}
    const char* what() const noexcept { return msg_.c_str(); }

  private:
    std::string msg_ = std::string("Actor killed.");
  };
};

class XBT_PUBLIC AttachContext : public Context {
public:
  AttachContext(std::function<void()> code, void_pfn_smxprocess_t cleanup_func, smx_actor_t actor)
      : Context(std::move(code), cleanup_func, actor)
  {
  }

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

}}}

typedef simgrid::kernel::context::ContextFactory *smx_context_factory_t;

XBT_PRIVATE void SIMIX_context_mod_init();
XBT_PRIVATE void SIMIX_context_mod_exit();

XBT_PUBLIC smx_context_t SIMIX_context_new(std::function<void()> code, void_pfn_smxprocess_t cleanup_func,
                                           smx_actor_t simix_process);

#ifndef WIN32
XBT_PUBLIC_DATA char sigsegv_stack[SIGSTKSZ];
#endif

/* We are using the bottom of the stack to save some information, like the
 * valgrind_stack_id. Define smx_context_usable_stack_size to give the remaining
 * size for the stack. Round its value to a multiple of 16 (asan wants the stacks to be aligned this way). */
#if HAVE_VALGRIND_H
#define smx_context_usable_stack_size                                                                                  \
  ((smx_context_stack_size - sizeof(unsigned int)) & ~0xf) /* for valgrind_stack_id */
#else
#define smx_context_usable_stack_size (smx_context_stack_size & ~0xf)
#endif

/** @brief Executes all the processes to run (in parallel if possible). */
XBT_PRIVATE void SIMIX_context_runall();

XBT_PUBLIC int SIMIX_process_get_maxpid();

XBT_PRIVATE void SIMIX_post_create_environment();

XBT_PRIVATE simgrid::simix::ActorCodeFactory& SIMIX_get_actor_code_factory(std::string name);

#endif
