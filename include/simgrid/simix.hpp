/* Copyright (c) 2007-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_SIMIX_HPP
#define SIMGRID_SIMIX_HPP

#include <cstddef>

#include <string>
#include <utility>
#include <memory>
#include <functional>

#include <xbt/function_types.h>
#include <xbt/future.hpp>
#include <xbt/functional.hpp>

#include <simgrid/simix.h>

XBT_PUBLIC(void) simcall_run_kernel(std::function<void()> const& code);

/** Execute some code in the kernel and block
 *
 * run_blocking() is a generic blocking simcall. It is given a callback
 * which is executed immediately in the SimGrid kernel. The callback is
 * responsible for setting the suitable logic for waking up the process
 * when needed.
 *
 * @ref simix::kernelSync() is a higher level wrapper for this.
 */
XBT_PUBLIC(void) simcall_run_blocking(std::function<void()> const& code);

template<class F> inline
void simcall_run_kernel(F& f)
{
  simcall_run_kernel(std::function<void()>(std::ref(f)));
}
template<class F> inline
void simcall_run_blocking(F& f)
{
  simcall_run_blocking(std::function<void()>(std::ref(f)));
}

namespace simgrid {

namespace simix {

/** Execute some code in the kernel/maestro
 *
 *  This can be used to enforce mutual exclusion with other simcall.
 *  More importantly, this enforces a deterministic/reproducible ordering
 *  of the operation with respect to other simcalls.
 */
template<class F>
typename std::result_of<F()>::type kernelImmediate(F&& code)
{
  // If we are in the maestro, we take the fast path and execute the
  // code directly without simcall mashalling/unmarshalling/dispatch:
  if (SIMIX_is_maestro())
    return std::forward<F>(code)();

  // If we are in the application, pass the code to the maestro which
  // executes it for us and reports the result. We use a std::future which
  // conveniently handles the success/failure value for us.
  typedef typename std::result_of<F()>::type R;
  simgrid::xbt::Result<R> result;
  simcall_run_kernel([&]{
    xbt_assert(SIMIX_is_maestro(), "Not in maestro");
    simgrid::xbt::fulfillPromise(result, std::forward<F>(code));
  });
  return result.get();
}

class Context;
class ContextFactory;

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

XBT_PUBLIC(void) set_maestro(std::function<void()> code);
XBT_PUBLIC(void) create_maestro(std::function<void()> code);

// What's executed as SIMIX actor code:
typedef std::function<void()> ActorCode;

// Create ActorCode based on argv:
typedef std::function<ActorCode(std::vector<std::string> args)> ActorCodeFactory;

XBT_PUBLIC(void) registerFunction(const char* name, ActorCodeFactory factory);

}
}

/*
 * Type of function that creates a process.
 * The function must accept the following parameters:
 * void* process: the process created will be stored there
 * const char *name: a name for the object. It is for user-level information and can be NULL
 * xbt_main_func_t code: is a function describing the behavior of the process
 * void *data: data a pointer to any data one may want to attach to the new object.
 * sg_host_t host: the location where the new process is executed
 * int argc, char **argv: parameters passed to code
 * xbt_dict_t pros: properties
 */
typedef smx_process_t (*smx_creation_func_t) (
                                      /* name */ const char*,
                                      std::function<void()> code,
                                      /* userdata */ void*,
                                      /* hostname */ const char*,
                                      /* kill_time */ double,
                                      /* props */ xbt_dict_t,
                                      /* auto_restart */ int,
                                      /* parent_process */ smx_process_t);

extern "C"
XBT_PUBLIC(void) SIMIX_function_register_process_create(smx_creation_func_t function);

XBT_PUBLIC(smx_process_t) simcall_process_create(const char *name,
                                          std::function<void()> code,
                                          void *data,
                                          const char *hostname,
                                          double kill_time,
                                          xbt_dict_t properties,
                                          int auto_restart);

XBT_PUBLIC(smx_timer_t) SIMIX_timer_set(double date, simgrid::xbt::Task<void()> callback);

template<class F> inline
XBT_PUBLIC(smx_timer_t) SIMIX_timer_set(double date, F callback)
{
  return SIMIX_timer_set(date, simgrid::xbt::Task<void()>(std::move(callback)));
}

template<class R, class T> inline
XBT_PUBLIC(smx_timer_t) SIMIX_timer_set(double date, R(*callback)(T*), T* arg)
{
  return SIMIX_timer_set(date, [=](){ callback(arg); });
}

#endif
