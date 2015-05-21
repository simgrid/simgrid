/* Copyright (c) 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_XBT_HPP
#define SIMGRID_MC_XBT_HPP

#include "mc/AddressSpace.hpp"

namespace simgrid {
namespace mc {

void read_element(AddressSpace const& as,
  void* local, remote_ptr<s_xbt_dynar_t> addr, size_t i, size_t len);
std::size_t read_length(AddressSpace const& as, remote_ptr<s_xbt_dynar_t> addr);

}
}

#endif
