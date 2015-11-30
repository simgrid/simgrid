/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_SURF_CALLBACKS_H
#define SIMGRID_SURF_CALLBACKS_H

/** \file callbacks.h
 *
 *  C interface for the C++ SURF callbacks.
 */

#include <xbt/base.h>
#include "simgrid/host.h"
#include "simgrid/msg.h"

SG_BEGIN_DECL();

XBT_PRIVATE void surf_host_created_callback(void (*callback)(sg_host_t));
XBT_PRIVATE void surf_storage_created_callback(void (*callback)(sg_storage_t));

SG_END_DECL();

#endif
