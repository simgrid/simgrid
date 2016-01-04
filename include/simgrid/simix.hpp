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

template<class F>
typename std::result_of<F()>::type kernel(F&& code)
{
  typedef typename std::result_of<F()>::type R;
  std::promise<R> promise;
  simcall_run_kernel([&]{
    try {
      promise.set_value(code());
    }
    catch(...) {
      promise.set_exception(std::current_exception());
    }
  });
  return promise.get_future().get();
}

class Context;
class ContextFactory;

class ContextFactory {
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

class Context {
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