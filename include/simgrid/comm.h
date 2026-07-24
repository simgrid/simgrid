/* Copyright (c) 2018-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INCLUDE_SIMGRID_COMM_H_
#define INCLUDE_SIMGRID_COMM_H_

#include <simgrid/forward.h>
#include <sys/types.h> /* ssize_t */

/* C interface */
SG_BEGIN_DECL

/** Returns whether the given activity is a communication */
XBT_PUBLIC int sg_comm_isinstance(sg_activity_t acti);

/** Start the comm, and ignore its result. It can be completely forgotten after that; @a clean_function (if not
 *  NULL) is called on the comm's data once it completes, so that you can release any resource associated to it. */
XBT_PUBLIC void sg_comm_detach(sg_comm_t comm, void (*clean_function)(void*));
/** Returns whether the communication is finished */
XBT_PUBLIC int sg_comm_test(sg_comm_t comm);
/** Block this actor until this communication is finished */
XBT_PUBLIC sg_error_t sg_comm_wait(sg_comm_t comm);
/** Block this actor until this communication is finished, or until the timeout is elapsed */
XBT_PUBLIC sg_error_t sg_comm_wait_for(sg_comm_t comm, double timeout);
/** Release the reference to that communication. Once its refcount reaches 0, the communication is destroyed. */
XBT_PUBLIC void sg_comm_unref(sg_comm_t comm);

SG_END_DECL

#endif /* INCLUDE_SIMGRID_COMM_H_ */
