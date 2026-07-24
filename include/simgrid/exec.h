/* Copyright (c) 2018-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INCLUDE_SIMGRID_EXEC_H_
#define INCLUDE_SIMGRID_EXEC_H_

#include <simgrid/forward.h>
#include <sys/types.h> /* ssize_t */

/* C interface */
SG_BEGIN_DECL

/** Returns whether the given activity is an execution */
XBT_PUBLIC int sg_exec_isinstance(sg_activity_t acti);

/** @brief change the execution bound
 * This means changing the maximal amount of flops per second that it may consume, regardless of what the host may
 * deliver. Currently, this cannot be changed once the exec started. */
XBT_PUBLIC void sg_exec_set_bound(sg_exec_t exec, double bound);
/** Retrieve the name of this execution */
XBT_PUBLIC const char* sg_exec_get_name(const_sg_exec_t exec);
/** Set the name of this execution, for logging and tracing purposes */
XBT_PUBLIC void sg_exec_set_name(sg_exec_t exec, const char* name);
/** @brief Change the host on which this execution takes place.
 *
 * This cannot be done once the activity is terminated, but it can be done on started executions. */
XBT_PUBLIC void sg_exec_set_host(sg_exec_t exec, sg_host_t new_host);
/** On sequential executions, returns the amount of flops that remain to be done; This cannot be used on parallel
 *  executions. */
XBT_PUBLIC double sg_exec_get_remaining(const_sg_exec_t exec);
/** @brief Returns the ratio of elements that are still to do
 *
 * The returned value is between 0 (completely done) and 1 (nothing done yet). */
XBT_PUBLIC double sg_exec_get_remaining_ratio(const_sg_exec_t exec);

/** Starts a previously created execution. This function is optional: you can call sg_exec_wait() even if you
 *  didn't call sg_exec_start() */
XBT_PUBLIC void sg_exec_start(sg_exec_t exec);
/** Cancel that execution */
XBT_PUBLIC void sg_exec_cancel(sg_exec_t exec);
/** Returns whether the execution is finished */
XBT_PUBLIC int sg_exec_test(sg_exec_t exec);
/** Block this actor until this execution is finished */
XBT_PUBLIC sg_error_t sg_exec_wait(sg_exec_t exec);
/** Block this actor until this execution is finished, or until the timeout is elapsed */
XBT_PUBLIC sg_error_t sg_exec_wait_for(sg_exec_t exec, double timeout);

SG_END_DECL

#endif /* INCLUDE_SIMGRID_EXEC_H_ */
