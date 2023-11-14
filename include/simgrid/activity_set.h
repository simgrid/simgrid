/* Copyright (c) 2018-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INCLUDE_SIMGRID_ACTIVITY_SET_H
#define INCLUDE_SIMGRID_ACTIVITY_SET_H

#include <simgrid/forward.h>
#include <sys/types.h> /* ssize_t */

/* C interface */
SG_BEGIN_DECL

XBT_PUBLIC sg_activity_set_t sg_activity_set_init();
XBT_PUBLIC void sg_activity_set_push(sg_activity_set_t as, sg_activity_t acti);
XBT_PUBLIC void sg_activity_set_erase(sg_activity_set_t as, sg_activity_t acti);
XBT_PUBLIC size_t sg_activity_set_size(sg_activity_set_t as);
XBT_PUBLIC int sg_activity_set_empty(sg_activity_set_t as);

XBT_PUBLIC sg_activity_t sg_activity_set_test_any(sg_activity_set_t as);
XBT_PUBLIC void sg_activity_set_wait_all(sg_activity_set_t as);
/** Returns true if it terminated successfully (or false on timeout) */
XBT_PUBLIC int sg_activity_set_wait_all_for(sg_activity_set_t as, double timeout);
XBT_PUBLIC sg_activity_t sg_activity_set_wait_any(sg_activity_set_t as);
XBT_PUBLIC sg_activity_t sg_activity_set_wait_any_for(sg_activity_set_t as, double timeout);
XBT_PUBLIC void sg_activity_set_delete(sg_activity_set_t as);

/** You must call this function manually on activities extracted from an activity_set with waitany and friends */
XBT_PUBLIC void sg_activity_unref(sg_activity_t acti);

SG_END_DECL

#endif /* INCLUDE_SIMGRID_ACTIVITY_SET_H */
