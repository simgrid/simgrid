/* Public interface to the Barrier datatype                                 */

/* Copyright (c) 2018-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INCLUDE_SIMGRID_BARRIER_H_
#define INCLUDE_SIMGRID_BARRIER_H_

#include <simgrid/forward.h>

#ifdef __cplusplus
extern "C++"
{
  constexpr bool SG_BARRIER_SERIAL_THREAD = true;
}
#else
#define SG_BARRIER_SERIAL_THREAD -1
#endif

/* C interface */
SG_BEGIN_DECL

/** Creates a barrier for the given amount of actors */
XBT_PUBLIC sg_bar_t sg_barrier_init(unsigned int count);
/** Release the reference to that barrier. Once its refcount reaches 0, the barrier is destroyed. */
XBT_PUBLIC void sg_barrier_destroy(sg_bar_t bar);
/** @brief Performs a barrier already initialized.
 *
 * @return 0 for all actors but one: exactly one actor will get SG_BARRIER_SERIAL_THREAD as a return value. */
XBT_PUBLIC int sg_barrier_wait(sg_bar_t bar);

SG_END_DECL

#endif /* INCLUDE_SIMGRID_BARRIER_H_ */
