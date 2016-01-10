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
#include <future>
#include <type_traits>

#include <xbt/function_types.h>
#include <simgrid/simix.h>

XBT_PUBLIC(void) simcall_run_kernel(std::function<void()> const& code);

namespace simgrid {
namespace simix {

/** Fulfill a promise by executing a given code */
template<class R, class F>
void fulfill_promise(std::promise<R>& promise, F&& code)
{
  try {
    promise.set_value(std::forward<F>(code)());
  }
  catch(...) {
    promise.set_exception(std::current_exception());
  }
}

/** Fulfill a promise by executing a given code
 *
 *  This is a special version for `std::promise<void>` because the default
 *  version does not compile in this case.
 */
template<class F>
void fulfill_promise(std::promise<void>& promise, F&& code)
{
  try {
    std::forward<F>(code)();
    promise.set_value();
  }
  catch(...) {
    promise.set_exception(std::current_exception());
  }
}

/** Execute some code in the kernel/maestro
 *
 *  This can be used to enforce mutual exclusion with other simcall.
 *  More importantly, this enforces a deterministic/reproducible ordering
 *  of the operation with respect to other simcalls.
 */
template<class F>
typename std::result_of<F()>::type kernel(F&& code)
{
  // If we are in the maestro, we take the fast path and execute the
  // code directly without simcall mashalling/unmarshalling/dispatch:
  if (SIMIX_is_maestro())
    return std::forward<F>(code)();

  // If we are in the application, pass the code to the maestro which is
  // executes it for us and reports the result. We use a std::future which
  // conveniently handles the success/failure value for us.
  typedef typename std::result_of<F()>::type R;
  std::promise<R> promise;
  simcall_run_kernel([&]{
    xbt_assert(SIMIX_is_maestro(), "Not in maestro");
    fulfill_promise(promise, std::forward<F>(code));
  });
  return promise.get_future().get();
}

class Context;
class ContextFactory;

XBT_PUBLIC_CLASS ContextFactory {
private:
  std::string name_;
public:

  ContextFactory(std::string name) : name_(std::move(name)) {}
  virtual ~ContextFactory();
  virtual Context* create_context(std::function<void()> code,
    void_pfn_smxprocess_t cleanup, smx_process_t process) = 0;
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

}
}

#endif
