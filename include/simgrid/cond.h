/* Copyright (c) 2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INCLUDE_SIMGRID_COND_H_
#define INCLUDE_SIMGRID_COND_H_

#include <simgrid/forward.h>

/* C interface */
SG_BEGIN_DECL
/** @brief Creates a condition variable */
XBT_PUBLIC sg_cond_t sg_cond_init();

/** @brief Blocks onto the given condition variable */
XBT_PUBLIC void sg_cond_wait(sg_cond_t cond, sg_mutex_t mutex);
/** @brief Blocks onto the given condition variable, but only for the given amount of time.
 *  @return 0 on success, 1 on timeout */
XBT_PUBLIC int sg_cond_wait_for(sg_cond_t cond, sg_mutex_t mutex, double delay);
/** @brief Signals the given mutex variable */
XBT_PUBLIC void sg_cond_notify_one(sg_cond_t cond);
/** @brief Broadcasts the given mutex variable */
XBT_PUBLIC void sg_cond_notify_all(sg_cond_t cond);
/** @brief Destroys the given mutex variable */
XBT_PUBLIC void sg_cond_destroy(const_sg_cond_t cond);

SG_END_DECL

#endif /* INCLUDE_SIMGRID_COND_H_ */
