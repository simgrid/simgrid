/* Copyright (c) 2018-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INCLUDE_SIMGRID_ACTIVITY_SET_H
#define INCLUDE_SIMGRID_ACTIVITY_SET_H

#include <simgrid/forward.h>
#include <sys/types.h> /* ssize_t */

/* C interface */
SG_BEGIN_DECL

/** Create an empty ActivitySet */
XBT_PUBLIC sg_activity_set_t sg_activity_set_init();
/** Add an activity to the set */
XBT_PUBLIC void sg_activity_set_push(sg_activity_set_t as, sg_activity_t acti);
/** Remove that activity from the set (no-op if the activity is not in the set) */
XBT_PUBLIC void sg_activity_set_erase(sg_activity_set_t as, sg_activity_t acti);
/** Get the amount of activities in the set. Failed activities (if any) are not counted */
XBT_PUBLIC size_t sg_activity_set_size(sg_activity_set_t as);
/** Return whether the set is empty. Failed activities (if any) are not counted */
XBT_PUBLIC int sg_activity_set_empty(sg_activity_set_t as);

/** Returns the first terminated activity if any, or NULL if no activity is terminated. You must call
 *  sg_activity_unref() on the returned activity once you are done with it. */
XBT_PUBLIC sg_activity_t sg_activity_set_test_any(sg_activity_set_t as);
/** Wait for the completion of all activities in the set. The set is NOT emptied afterward. */
XBT_PUBLIC void sg_activity_set_wait_all(sg_activity_set_t as);
/** Returns true if it terminated successfully (or false on timeout) */
XBT_PUBLIC int sg_activity_set_wait_all_for(sg_activity_set_t as, double timeout);
/** Wait for the completion of one activity from the set.
 *
 * @return the first terminated activity, which is automatically removed from the set. You must call
 * sg_activity_unref() on it once you are done with it. */
XBT_PUBLIC sg_activity_t sg_activity_set_wait_any(sg_activity_set_t as);
/** Wait for the completion of one activity from the set, but not longer than the provided timeout.
 *
 * @return the first terminated activity, which is automatically removed from the set, or NULL on timeout. You
 * must call sg_activity_unref() on the returned activity once you are done with it. */
XBT_PUBLIC sg_activity_t sg_activity_set_wait_any_for(sg_activity_set_t as, double timeout);
/** Destroy that ActivitySet */
XBT_PUBLIC void sg_activity_set_delete(sg_activity_set_t as);

/** You must call this function manually on activities extracted from an activity_set with waitany and friends */
XBT_PUBLIC void sg_activity_unref(sg_activity_t acti);

SG_END_DECL

#endif /* INCLUDE_SIMGRID_ACTIVITY_SET_H */
