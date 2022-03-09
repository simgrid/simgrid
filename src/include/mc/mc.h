/* Copyright (c) 2008-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_MC_H
#define MC_MC_H

#include <simgrid/forward.h>
#include <simgrid/modelchecker.h> /* our public interface (and definition of SIMGRID_HAVE_MC) */

#ifdef __cplusplus
XBT_PUBLIC void MC_process_clock_add(const simgrid::kernel::actor::ActorImpl*, double);
XBT_PUBLIC double MC_process_clock_get(const simgrid::kernel::actor::ActorImpl*);
#endif

#endif
