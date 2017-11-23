/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_CONTEXT_CONTEXT_HPP
#define SIMGRID_KERNEL_CONTEXT_CONTEXT_HPP

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include <xbt/functional.hpp>

#include "simgrid/simix.h"
#include "src/instr/instr_private.hpp"
#include "src/internal_config.h"
#include "src/simix/popping_private.hpp"
#include "src/simix/smx_host_private.hpp"
#include "src/simix/smx_io_private.hpp"
#include "src/simix/smx_network_private.hpp"
#include "src/simix/smx_synchro_private.hpp"
#include "surf/surf.hpp"
#include "xbt/base.h"
#include "xbt/config.h"
#include "xbt/function_types.h"
#include "xbt/mallocator.h"
#include "xbt/xbt_os_time.h"

#include "src/simix/ActorImpl.hpp"
#include <csignal>

#include <simgrid/simix.hpp>

namespace simgrid {
namespace kernel {
namespace context {

  XBT_PUBLIC_CLASS ContextFactory {
  private:
    std::string name_;
  public:

    explicit ContextFactory(std::string name) : name_(std::move(name)) {}
    virtual ~ContextFactory();
    virtual Context* create_context(std::function<void()> code,
      void_pfn_smxprocess_t cleanup, smx_actor_t process) = 0;

    // Optional methods for attaching main() as a context:

    /** Creates a context from the current context of execution
     *
     *  This will not work on all implementation of `ContextFactory`.
     */
    virtual Context* attach(void_pfn_smxprocess_t cleanup_func, smx_actor_t process);
    virtual Context* create_maestro(std::function<void()> code, smx_actor_t process);

    virtual void run_all() = 0;
    virtual Context* self();
    std::string const& name() const
    {
      return name_;
    }
  private:
    void declare_context(void* T, std::size_t size);
  protected:
    template<class T, class... Args>
    T* new_context(Args&&... args)
    {
      T* context = new T(std::forward<Args>(args)...);
      this->declare_context(context, sizeof(T));
      return context;
    }
  };

  XBT_PUBLIC_CLASS Context {
  private:
    std::function<void()> code_;
    void_pfn_smxprocess_t cleanup_func_ = nullptr;
    smx_actor_t process_ = nullptr;
  public:
    class StopRequest {
      /** @brief Exception launched to kill a process, in order to properly unwind its stack and release RAII stuff
       *
       * Nope, Sonar, this should not inherit of std::exception.
       * Otherwise, users may accidentally catch it with a try {} catch (std::exception)
       */
    };
    bool iwannadie;

    Context(std::function<void()> code,
            void_pfn_smxprocess_t cleanup_func,
            smx_actor_t process);
    void operator()()
    {
      code_();
    }
    bool has_code() const
    {
      return static_cast<bool>(code_);
    }
    smx_actor_t process()
    {
      return this->process_;
    }
    void set_cleanup(void_pfn_smxprocess_t cleanup)
    {
      cleanup_func_ = cleanup;
    }

    // Virtual methods
    virtual ~Context();
    virtual void stop();
    virtual void suspend() = 0;
  };

  XBT_PUBLIC_CLASS AttachContext : public Context {
  public:

    AttachContext(std::function<void()> code,
            void_pfn_smxprocess_t cleanup_func,
            smx_actor_t process)
      : Context(std::move(code), cleanup_func, process)
    {}

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
XBT_PUBLIC_DATA(ContextFactoryInitializer) factory_initializer;

XBT_PRIVATE ContextFactory* thread_factory();
XBT_PRIVATE ContextFactory* sysv_factory();
XBT_PRIVATE ContextFactory* raw_factory();
XBT_PRIVATE ContextFactory* boost_factory();

}}}

typedef simgrid::kernel::context::ContextFactory *smx_context_factory_t;

extern "C" {

XBT_PRIVATE void SIMIX_context_mod_init();
XBT_PRIVATE void SIMIX_context_mod_exit();

XBT_PUBLIC(smx_context_t)
SIMIX_context_new(std::function<void()> code, void_pfn_smxprocess_t cleanup_func, smx_actor_t simix_process);

#ifndef WIN32
XBT_PUBLIC_DATA(char sigsegv_stack[SIGSTKSZ]);
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
/** @brief returns the current running context */
XBT_PUBLIC(smx_context_t) SIMIX_context_self(); // public because it's used in simgrid-java

XBT_PRIVATE void *SIMIX_context_stack_new();
XBT_PRIVATE void SIMIX_context_stack_delete(void *stack);

XBT_PUBLIC(void) SIMIX_context_set_current(smx_context_t context);
XBT_PRIVATE smx_context_t SIMIX_context_get_current();

XBT_PUBLIC(int) SIMIX_process_get_maxpid();

XBT_PRIVATE void SIMIX_post_create_environment();
}

XBT_PRIVATE simgrid::simix::ActorCodeFactory& SIMIX_get_actor_code_factory(const char *name);

#endif
