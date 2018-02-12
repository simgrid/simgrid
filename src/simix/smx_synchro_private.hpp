/* Copyright (c) 2012-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_SYNCHRO_PRIVATE_H
#define SIMIX_SYNCHRO_PRIVATE_H

#include "simgrid/s4u/ConditionVariable.hpp"
#include "src/simix/ActorImpl.hpp"
#include <boost/intrusive/list.hpp>

smx_activity_t SIMIX_synchro_wait(sg_host_t smx_host, double timeout);

struct s_smx_cond_t {
  s_smx_cond_t() : cond_(this) {}

  std::atomic_int_fast32_t refcount_{1};
  smx_mutex_t mutex   = nullptr;
  simgrid::simix::SynchroList sleeping; /* list of sleeping processes */
  simgrid::s4u::ConditionVariable cond_;
};

struct s_smx_sem_t {
  unsigned int value;
  simgrid::simix::SynchroList sleeping; /* list of sleeping processes */
};

XBT_PRIVATE void SIMIX_synchro_stop_waiting(smx_actor_t process, smx_simcall_t simcall);
XBT_PRIVATE void SIMIX_synchro_finish(smx_activity_t synchro);

XBT_PRIVATE smx_cond_t SIMIX_cond_init();
XBT_PRIVATE void SIMIX_cond_broadcast(smx_cond_t cond);
XBT_PRIVATE void SIMIX_cond_signal(smx_cond_t cond);
XBT_PRIVATE void intrusive_ptr_add_ref(s_smx_cond_t* cond);
XBT_PRIVATE void intrusive_ptr_release(s_smx_cond_t* cond);

XBT_PRIVATE XBT_PRIVATE smx_sem_t SIMIX_sem_init(unsigned int value);
XBT_PRIVATE void SIMIX_sem_release(smx_sem_t sem);
XBT_PRIVATE int SIMIX_sem_would_block(smx_sem_t sem);
XBT_PRIVATE int SIMIX_sem_get_capacity(smx_sem_t sem);

#endif
