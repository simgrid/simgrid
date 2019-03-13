/* Copyright (c) 2012-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_MUTEXIMPL_HPP
#define SIMIX_MUTEXIMPL_HPP

#include "simgrid/s4u/ConditionVariable.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include <boost/intrusive/list.hpp>

namespace simgrid {
namespace kernel {
namespace activity {

class XBT_PUBLIC MutexImpl {
public:
  MutexImpl();
  ~MutexImpl();
  MutexImpl(MutexImpl const&) = delete;
  MutexImpl& operator=(MutexImpl const&) = delete;

  void lock(actor::ActorImpl* issuer);
  bool try_lock(actor::ActorImpl* issuer);
  void unlock(actor::ActorImpl* issuer);

  MutexImpl* ref();
  void unref();
  bool locked_             = false;
  actor::ActorImpl* owner_ = nullptr;
  // List of sleeping actors:
  actor::SynchroList sleeping_;

  // boost::intrusive_ptr<Mutex> support:
  friend void intrusive_ptr_add_ref(MutexImpl* mutex)
  {
    XBT_ATTRIB_UNUSED auto previous = mutex->refcount_.fetch_add(1);
    xbt_assert(previous != 0);
  }

  friend void intrusive_ptr_release(MutexImpl* mutex)
  {
    if (mutex->refcount_.fetch_sub(1) == 1)
      delete mutex;
  }

  s4u::Mutex& mutex() { return piface_; }

private:
  std::atomic_int_fast32_t refcount_{1};
  s4u::Mutex piface_;
};
}
}
}
#endif /* SIMIX_MUTEXIMPL_HPP */
