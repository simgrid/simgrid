/* Copyright (c) 2007-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SIMGRID_KERNEL_CONTEXT_CONTEXT_HPP
#define _SIMGRID_KERNEL_CONTEXT_CONTEXT_HPP

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include <xbt/functional.hpp>

#include "src/internal_config.h"
#include "simgrid/simix.h"
#include "surf/surf.h"
#include "xbt/base.h"
#include "xbt/fifo.h"
#include "xbt/swag.h"
#include "xbt/dict.h"
#include "xbt/mallocator.h"
#include "xbt/config.h"
#include "xbt/xbt_os_time.h"
#include "xbt/function_types.h"
#include "src/xbt/ex_interface.h"
#include "src/instr/instr_private.h"
#include "src/simix/smx_host_private.h"
#include "src/simix/smx_io_private.h"
#include "src/simix/smx_network_private.h"
#include "src/simix/popping_private.h"
#include "src/simix/smx_synchro_private.h"

#include <signal.h>
#include "src/simix/ActorImpl.hpp"

#include <simgrid/simix.hpp>

namespace simgrid {
namespace kernel {
namespace context {

  class Context;

  XBT_PUBLIC_CLASS ContextFactory {
  private:
    std::string name_;
  public:

    explicit ContextFactory(std::string name) : name_(std::move(name)) {}
    virtual ~ContextFactory();
    virtual Context* create_context(std::function<void()> code,
      void_pfn_smxprocess_t cleanup, smx_process_t process) = 0;

    // Optional methods for attaching main() as a context:

    /** Creates a context from the current context of execution
     *
     *  This will not work on all implementation of `ContextFactory`.
     */
    virtual Context* attach(void_pfn_smxprocess_t cleanup_func, smx_process_t process);
    virtual Context* create_maestro(std::function<void()> code, smx_process_t process);

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
    smx_process_t process_ = nullptr;
  public:
    bool iwannadie;
  public:
    Context(std::function<void()> code,
            void_pfn_smxprocess_t cleanup_func,
            smx_process_t process);
    void operator()()
    {
      code_();
    }
    bool has_code() const
    {
      return (bool) code_;
    }
    smx_process_t process()
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
            smx_process_t process)
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

SG_BEGIN_DECL()


XBT_PRIVATE void SIMIX_context_mod_init();
XBT_PRIVATE void SIMIX_context_mod_exit();

XBT_PRIVATE smx_context_t SIMIX_context_new(
  std::function<void()> code,
  void_pfn_smxprocess_t cleanup_func,
  smx_process_t simix_process);

#ifndef WIN32
XBT_PUBLIC_DATA(char sigsegv_stack[SIGSTKSZ]);
#endif

/* We are using the bottom of the stack to save some information, like the
 * valgrind_stack_id. Define smx_context_usable_stack_size to give the remaining
 * size for the stack. */
#if HAVE_VALGRIND_H
# define smx_context_usable_stack_size                                  \
  (smx_context_stack_size - sizeof(unsigned int)) /* for valgrind_stack_id */
#else
# define smx_context_usable_stack_size smx_context_stack_size
#endif

/** @brief Executes all the processes to run (in parallel if possible). */
XBT_PRIVATE void SIMIX_context_runall();
/** @brief returns the current running context */
XBT_PUBLIC(smx_context_t) SIMIX_context_self(); // public because it's used in simgrid-java

XBT_PRIVATE void *SIMIX_context_stack_new();
XBT_PRIVATE void SIMIX_context_stack_delete(void *stack);

XBT_PRIVATE void SIMIX_context_set_current(smx_context_t context);
XBT_PRIVATE smx_context_t SIMIX_context_get_current();

XBT_PUBLIC(int) SIMIX_process_get_maxpid();

XBT_PRIVATE void SIMIX_post_create_environment();

SG_END_DECL()

XBT_PRIVATE simgrid::simix::ActorCodeFactory& SIMIX_get_actor_code_factory(const char *name);

#endif
