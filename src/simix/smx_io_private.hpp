/* Copyright (c) 2007-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_IO_PRIVATE_HPP
#define SIMIX_IO_PRIVATE_HPP

#include <xbt/base.h>
#include "simgrid/s4u/Io.hpp"

XBT_PRIVATE simgrid::kernel::activity::IoImplPtr SIMIX_io_start(std::string name, sg_size_t size, sg_storage_t storage,
                                                                simgrid::s4u::Io::OpType type);

XBT_PRIVATE void SIMIX_io_finish(smx_activity_t synchro);

#endif
