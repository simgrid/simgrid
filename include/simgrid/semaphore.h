/* Public interface to the Link datatype                                    */

/* Copyright (c) 2018-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INCLUDE_SIMGRID_SEMAPHORE_H_
#define INCLUDE_SIMGRID_SEMAPHORE_H_

#include <simgrid/forward.h>

/* C interface */
SG_BEGIN_DECL
XBT_PUBLIC sg_sem_t sg_sem_init(int initial_value);
XBT_PUBLIC void sg_sem_acquire(sg_sem_t sem);
XBT_PUBLIC int sg_sem_acquire_timeout(sg_sem_t sem, double timeout);
XBT_PUBLIC void sg_sem_release(sg_sem_t sem);
XBT_PUBLIC int sg_sem_get_capacity(const_sg_sem_t sem);
XBT_PUBLIC void sg_sem_destroy(const_sg_sem_t sem);
XBT_PUBLIC int sg_sem_would_block(const_sg_sem_t sem);

SG_END_DECL

#endif /* INCLUDE_SIMGRID_SEMAPHORE_H_ */
