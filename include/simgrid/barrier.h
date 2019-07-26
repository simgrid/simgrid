/* Public interface to the Link datatype                                    */

/* Copyright (c) 2018-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INCLUDE_SIMGRID_BARRIER_H_
#define INCLUDE_SIMGRID_BARRIER_H_

#include <simgrid/forward.h>

#ifdef __cplusplus
constexpr int SG_BARRIER_SERIAL_THREAD = -1;
#else
#define SG_BARRIER_SERIAL_THREAD -1
#endif

/* C interface */
SG_BEGIN_DECL()

XBT_PUBLIC sg_bar_t sg_barrier_init(unsigned int count);
XBT_PUBLIC void sg_barrier_destroy(sg_bar_t bar);
XBT_PUBLIC int sg_barrier_wait(sg_bar_t bar);

SG_END_DECL()

#endif /* INCLUDE_SIMGRID_BARRIER_H_ */
