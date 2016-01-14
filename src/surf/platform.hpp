/* Copyright (c) 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_PLATFORM_HPP
#define SIMGRID_PLATFORM_HPP

#include <xbt/base.h>
#include <xbt/signal.hpp>
#include <simgrid/forward.h>

namespace simgrid {
namespace surf {

extern XBT_PRIVATE simgrid::xbt::signal<void(sg_platf_link_cbarg_t)> on_link;
extern XBT_PRIVATE simgrid::xbt::signal<void(sg_platf_cluster_cbarg_t)> on_cluster;
extern XBT_PRIVATE simgrid::xbt::signal<void(void)> on_postparse;

}
}

#endif
