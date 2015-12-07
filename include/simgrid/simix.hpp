/* Copyright (c) 2007-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_SIMIX_HPP
#define SIMGRID_SIMIX_HPP

#include <utility>
#include <memory>

#include <xbt/function_types.h>
#include <simgrid/simix.h>

namespace simgrid {
namespace simix {

class Context;
class ContextFactory;

class ContextFactory {
private:
  std::string name_;
public:

  ContextFactory(std::string name) : name_(std::move(name)) {}
  virtual ~ContextFactory();
  virtual Context* create_context(
    xbt_main_func_t, int, char **, void_pfn_smxprocess_t,
    smx_process_t process
    ) = 0;
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
protected:
  xbt_main_func_t code_ = nullptr;
  int argc_ = 0;
  char **argv_ = nullptr;
private:
  void_pfn_smxprocess_t cleanup_func_ = nullptr;
  smx_process_t process_ = nullptr;
public:
  bool iwannadie;
public:
  Context(xbt_main_func_t code,
          int argc, char **argv,
          void_pfn_smxprocess_t cleanup_func,
          smx_process_t process);
  int operator()()
  {
    return code_(argc_, argv_);
  }
  smx_process_t process()
  {
    return this->process_;
  }

  // Virtual methods
  virtual ~Context();
  virtual void stop();
  virtual void suspend() = 0;
};

}
}

#endif