/* Copyright (c) 2006-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_COND_VARIABLE_HPP
#define SIMGRID_S4U_COND_VARIABLE_HPP

#include <mutex>
#include <utility> // std::swap

#include <simgrid/simix.h>
#include <simgrid/s4u/mutex.hpp>

namespace simgrid {
namespace s4u {

class Mutex;

XBT_PUBLIC_CLASS ConditionVariable {
  
public:
  ConditionVariable();

  ConditionVariable(ConditionVariable* cond) : cond_(SIMIX_cond_ref(cond->cond_)) {}
  ~ConditionVariable();

  // Copy+move (with the copy-and-swap idiom):
  ConditionVariable(ConditionVariable const& cond) : cond_(SIMIX_cond_ref(cond.cond_)) {}
  friend void swap(ConditionVariable& first, ConditionVariable& second)
  {
    using std::swap;
    swap(first.cond_, second.cond_);
  }
  ConditionVariable& operator=(ConditionVariable cond)
  {
    swap(*this, cond);
    return *this;
  }
  ConditionVariable(ConditionVariable&& cond) : cond_(nullptr)
  {
    swap(*this, cond);
  }

  bool valid() const
  {
    return cond_ != nullptr;
  }
  
  /**
  * Wait functions
  */
  void wait(std::unique_lock<Mutex>& lock);
  void wait_for(std::unique_lock<Mutex>& lock, double time);

  /**
  * Notify functions
  */
  void notify();
  void notify_all();

private:
  smx_cond_t cond_;

};
}} // namespace simgrid::s4u

#endif /* SIMGRID_S4U_COND_VARIABLE_HPP */
