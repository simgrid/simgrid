/* Copyright (c) 2012-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_SYNCHRO_PRIVATE_H
#define SIMIX_SYNCHRO_PRIVATE_H

#include "src/simix/ActorImpl.hpp"

smx_activity_t SIMIX_synchro_wait(sg_host_t smx_host, double timeout);

XBT_PRIVATE void SIMIX_synchro_stop_waiting(smx_actor_t process, smx_simcall_t simcall);

#endif
