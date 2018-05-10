/* Copyright (c) 2012-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_MUTEXIMPL_HPP
#define SIMIX_MUTEXIMPL_HPP

#include "simgrid/s4u/ConditionVariable.hpp"
#include "src/simix/ActorImpl.hpp"
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

  void lock(smx_actor_t issuer);
  bool try_lock(smx_actor_t issuer);
  void unlock(smx_actor_t issuer);

  bool locked       = false;
  smx_actor_t owner = nullptr;
  // List of sleeping processes:
  simgrid::kernel::actor::SynchroList sleeping;

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

  simgrid::s4u::Mutex& mutex() { return piface_; }

private:
  std::atomic_int_fast32_t refcount_{1};
  simgrid::s4u::Mutex piface_;
};
}
}
}
#endif /* SIMIX_MUTEXIMPL_HPP */
