/* Copyright (c) 2004-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_PMGR_H
#define SURF_PMGR_H

#include "simgrid/forward.h"
#include "src/kernel/resource/profile/Profile.hpp"
#include "xbt/sysdep.h"

#include <queue>
#include <vector>

extern XBT_PRIVATE simgrid::kernel::profile::FutureEvtSet future_evt_set;

/**
 * @brief Free a trace event structure
 *
 * This function frees a trace_event if it can be freed, ie, if it has the free_me flag set to 1.
 * This flag indicates whether the structure is still used somewhere or not.
 * When the structure is freed, the argument is set to nullptr
 */
XBT_PUBLIC void tmgr_trace_event_unref(simgrid::kernel::profile::Event** trace_event);
XBT_PUBLIC void tmgr_finalize();

#endif /* SURF_PMGR_H */
