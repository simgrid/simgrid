/* Copyright (c) 2012-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_ACTIVITY_MUTEX_HPP
#define SIMGRID_KERNEL_ACTIVITY_MUTEX_HPP

#include "simgrid/s4u/Mutex.hpp"
#include "src/kernel/activity/ActivityImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "xbt/asserts.h"
#include <boost/intrusive/list.hpp>

namespace simgrid::kernel::activity {

/** Mutex Acquisition: the act / process of acquiring the mutex.
 *
 * You can declare some interest on a mutex without being blocked waiting if it's already occupied.
 * If it gets freed by its current owned, you become the new owner, even if you're still not blocked on it.
 * Nobody can lock it behind your back or overpass you in the queue in any way, even if you're still not blocked on it.
 *
 * Afterward, when you do consider the lock, the test() or wait() operations are both non-blocking since you're the
 * owner. People who declared interest in the mutex after you get stuck in the queue behind you.
 *
 *
 * Splitting the locking process this way is interesting for symmetry with the other activities such as exec or
 * communication that do have an async variant and could be mildly interesting to the users once exposed in S4U, but
 * that's not the only reason. It's also very important to the MC world: the Mutex::lock_async() is always enabled
 * (nothing can prevent you from adding yourself to the queue of potential owners) while Acquisition::wait() is
 * persistent: it's not always enabled but once it gets enabled (because you're the owner), it remains enabled for ever.
 *
 * Mutex::lock() is not persistent: sometimes it's enabled if the mutex is free, and then it gets disabled if
 * someone else locks the mutex, and then it becomes enabled again once the mutex is freed. This is why Mutex::lock()
 * is not used in our MC computational model: we ban non-persistent transitions because they would make some
 * computations much more complex.
 *
 * In particular, computing the extension of an unfolding's configuration is polynomial when you only have persistent
 * transitions while it's O(2^n) when some of the transitions are non-persistent (you have to consider again all subsets
 * of a set if some transitions may become disabled in between, while you don't have to reconsider them if you can reuse
 * your previous computations).
 */
class XBT_PUBLIC MutexAcquisitionImpl : public ActivityImpl_T<MutexAcquisitionImpl> {
  actor::ActorImpl* issuer_ = nullptr;
  MutexImpl* mutex_         = nullptr;
  int recursive_depth_      = 1;
  // TODO: use granted_ this instead of owner_ == self to test().
  // This is mandatory to get double-lock on non-recursive locks to properly deadlock
  bool granted_ = false;

  friend MutexImpl;

public:
  MutexAcquisitionImpl(actor::ActorImpl* issuer, MutexImpl* mutex) : issuer_(issuer), mutex_(mutex) {}
  MutexImplPtr get_mutex() const { return mutex_; }
  actor::ActorImpl* get_issuer() const { return issuer_; }
  void grant() { granted_ = true; }
  bool is_granted() const { return granted_; }

  bool test(actor::ActorImpl* issuer = nullptr) override;
  void wait_for(actor::ActorImpl* issuer, double timeout) override;
  void finish() override;
  void set_exception(actor::ActorImpl* issuer) override
  { /* nothing to do */
  }
};

class XBT_PUBLIC MutexImpl {
  std::atomic_int_fast32_t refcount_{1};
  s4u::Mutex piface_;
  actor::ActorImpl* owner_ = nullptr;
  std::deque<MutexAcquisitionImplPtr> ongoing_acquisitions_;
  static unsigned next_id_;
  unsigned id_ = next_id_++;
  bool is_recursive_  = false;
  int recursive_depth = 0;

  friend MutexAcquisitionImpl;

public:
  explicit MutexImpl(bool recursive = false) : piface_(this), is_recursive_(recursive) {}
  MutexImpl(MutexImpl const&) = delete;
  MutexImpl& operator=(MutexImpl const&) = delete;

  MutexAcquisitionImplPtr lock_async(actor::ActorImpl* issuer);
  bool try_lock(actor::ActorImpl* issuer);
  void unlock(actor::ActorImpl* issuer);
  unsigned get_id() const { return id_; }

  actor::ActorImpl* get_owner() const { return owner_; }

  // boost::intrusive_ptr<Mutex> support:
  friend void intrusive_ptr_add_ref(MutexImpl* mutex)
  {
    XBT_ATTRIB_UNUSED auto previous = mutex->refcount_.fetch_add(1);
    xbt_assert(previous != 0);
  }

  friend void intrusive_ptr_release(MutexImpl* mutex)
  {
    if (mutex->refcount_.fetch_sub(1) == 1) {
      xbt_assert(mutex->ongoing_acquisitions_.empty(), "The destroyed mutex still had ongoing acquisitions");
      xbt_assert(mutex->owner_ == nullptr, "The destroyed mutex is still owned by actor %s",
                 mutex->owner_->get_cname());
      delete mutex;
    }
  }

  s4u::Mutex& get_iface() { return piface_; }
};
} // namespace simgrid::kernel::activity
#endif
