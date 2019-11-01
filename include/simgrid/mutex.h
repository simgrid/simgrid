/* Copyright (c) 2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INCLUDE_SIMGRID_MUTEX_H_
#define INCLUDE_SIMGRID_MUTEX_H_

#include <simgrid/forward.h>

/* C interface */
SG_BEGIN_DECL
XBT_PUBLIC sg_mutex_t sg_mutex_init();
XBT_PUBLIC void sg_mutex_lock(sg_mutex_t mutex);
XBT_PUBLIC void sg_mutex_unlock(sg_mutex_t mutex);
XBT_PUBLIC int sg_mutex_try_lock(sg_mutex_t mutex);
XBT_PUBLIC void sg_mutex_destroy(sg_mutex_t mutex);

SG_END_DECL

#endif /* INCLUDE_SIMGRID_MUTEX_H_ */
