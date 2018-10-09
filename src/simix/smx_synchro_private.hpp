/* Copyright (c) 2012-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_SYNCHRO_PRIVATE_H
#define SIMIX_SYNCHRO_PRIVATE_H

#include "src/simix/ActorImpl.hpp"

smx_activity_t SIMIX_synchro_wait(sg_host_t smx_host, double timeout);

struct s_smx_sem_t {
  unsigned int value;
  simgrid::kernel::actor::SynchroList sleeping; /* list of sleeping processes */
};

XBT_PRIVATE void SIMIX_synchro_stop_waiting(smx_actor_t process, smx_simcall_t simcall);
XBT_PRIVATE void SIMIX_synchro_finish(smx_activity_t synchro);

XBT_PRIVATE XBT_PRIVATE smx_sem_t SIMIX_sem_init(unsigned int value);
XBT_PRIVATE void SIMIX_sem_release(smx_sem_t sem);
XBT_PRIVATE int SIMIX_sem_would_block(smx_sem_t sem);
XBT_PRIVATE int SIMIX_sem_get_capacity(smx_sem_t sem);

#endif
