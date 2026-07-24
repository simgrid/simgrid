/* Public interface to the Link datatype                                    */

/* Copyright (c) 2018-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INCLUDE_SIMGRID_SEMAPHORE_H_
#define INCLUDE_SIMGRID_SEMAPHORE_H_

#include <simgrid/forward.h>

/* C interface */
SG_BEGIN_DECL
/** Constructs a new semaphore, with the given initial capacity */
XBT_PUBLIC sg_sem_t sg_sem_init(int initial_value);
/** Acquire the semaphore, blocking the current actor until it is available if needed. */
XBT_PUBLIC void sg_sem_acquire(sg_sem_t sem);
/** Just like sg_sem_acquire(), but with a timeout. Returns 1 if there was a timeout, 0 if the semaphore was
 *  acquired normally. */
XBT_PUBLIC int sg_sem_acquire_timeout(sg_sem_t sem, double timeout);
/** Release the semaphore, increasing its capacity by one, and waking up a blocked actor if any. */
XBT_PUBLIC void sg_sem_release(sg_sem_t sem);
/** Retrieve the current capacity of the semaphore. */
XBT_PUBLIC int sg_sem_get_capacity(const_sg_sem_t sem);
/** Release the reference to that semaphore. Once its refcount reaches 0, the semaphore is destroyed. */
XBT_PUBLIC void sg_sem_destroy(const_sg_sem_t sem);
/** Returns whether calling sg_sem_acquire() would block the current actor. */
XBT_PUBLIC int sg_sem_would_block(const_sg_sem_t sem);

SG_END_DECL

#endif /* INCLUDE_SIMGRID_SEMAPHORE_H_ */
