/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_NETWORK_PRIVATE_HPP
#define SIMIX_NETWORK_PRIVATE_HPP

#include "simgrid/forward.h"
#include "src/simix/popping_private.hpp"

XBT_PRIVATE smx_activity_t SIMIX_comm_iprobe(smx_mailbox_t mbox, int type, simix_match_func_t match_fun, void* data);

#endif
