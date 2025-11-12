/* Copyright (c) 2018-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INCLUDE_SIMGRID_COMM_H_
#define INCLUDE_SIMGRID_COMM_H_

#include <simgrid/forward.h>
#include <sys/types.h> /* ssize_t */

/* C interface */
SG_BEGIN_DECL

XBT_PUBLIC int sg_comm_isinstance(sg_activity_t acti);

XBT_PUBLIC void sg_comm_detach(sg_comm_t comm, void (*clean_function)(void*));
/** Returns whether the communication is finished */
XBT_PUBLIC int sg_comm_test(sg_comm_t comm);
/** Block this actor until this communication is finished */
XBT_PUBLIC sg_error_t sg_comm_wait(sg_comm_t comm);
XBT_PUBLIC sg_error_t sg_comm_wait_for(sg_comm_t comm, double timeout);
XBT_PUBLIC void sg_comm_unref(sg_comm_t comm);

SG_END_DECL

#endif /* INCLUDE_SIMGRID_COMM_H_ */
