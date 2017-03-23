/* Copyright (c) 2012-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_SYNCHRO_PRIVATE_H
#define SIMIX_SYNCHRO_PRIVATE_H

#include "simgrid/s4u/ConditionVariable.hpp"
#include "xbt/swag.h"

namespace simgrid {
namespace simix {

class XBT_PUBLIC() Mutex {
public:
  Mutex();
  ~Mutex();
  Mutex(Mutex const&) = delete;
  Mutex& operator=(Mutex const&) = delete;

  void lock(smx_actor_t issuer);
  bool try_lock(smx_actor_t issuer);
  void unlock(smx_actor_t issuer);

  bool locked = false;
  smx_actor_t owner = nullptr;
  // List of sleeping processes:
  xbt_swag_t sleeping = nullptr;

  // boost::intrusive_ptr<Mutex> support:
  friend void intrusive_ptr_add_ref(Mutex* mutex)
  {
    // Atomic operation! Do not split in two instructions!
    auto previous = (mutex->refcount_)++;
    xbt_assert(previous != 0);
    (void) previous;
  }
  friend void intrusive_ptr_release(Mutex* mutex)
  {
    // Atomic operation! Do not split in two instructions!
    auto count = --(mutex->refcount_);
    if (count == 0)
      delete mutex;
  }

  simgrid::s4u::Mutex& mutex() { return mutex_; }

private:
  std::atomic_int_fast32_t refcount_ { 1 };
  simgrid::s4u::Mutex mutex_;
};

}
}

typedef struct s_smx_cond {
  s_smx_cond() : cond_(this) {}

  std::atomic_int_fast32_t refcount_ { 1 };
  smx_mutex_t mutex = nullptr;
  xbt_swag_t sleeping = nullptr;  /* list of sleeping process */
  simgrid::s4u::ConditionVariable cond_;
} s_smx_cond_t;

typedef struct s_smx_sem {
  unsigned int value;
  xbt_swag_t sleeping;          /* list of sleeping process */
} s_smx_sem_t;

XBT_PRIVATE void SIMIX_post_synchro(smx_activity_t synchro);
XBT_PRIVATE void SIMIX_synchro_stop_waiting(smx_actor_t process, smx_simcall_t simcall);
XBT_PRIVATE void SIMIX_synchro_destroy(smx_activity_t synchro);
XBT_PRIVATE void SIMIX_synchro_finish(smx_activity_t synchro);

XBT_PRIVATE smx_cond_t SIMIX_cond_init();
XBT_PRIVATE void SIMIX_cond_broadcast(smx_cond_t cond);
XBT_PRIVATE void SIMIX_cond_signal(smx_cond_t cond);
XBT_PRIVATE void intrusive_ptr_add_ref(s_smx_cond_t *cond);
XBT_PRIVATE void intrusive_ptr_release(s_smx_cond_t *cond);

XBT_PRIVATE XBT_PRIVATE smx_sem_t SIMIX_sem_init(unsigned int value);
XBT_PRIVATE void SIMIX_sem_release(smx_sem_t sem);
XBT_PRIVATE int SIMIX_sem_would_block(smx_sem_t sem);
XBT_PRIVATE int SIMIX_sem_get_capacity(smx_sem_t sem);

#endif
