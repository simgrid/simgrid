/* Copyright (c) 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_XBT_HPP
#define SIMGRID_MC_XBT_HPP

#include <cstddef>

#include <xbt/dynar.h>

#include "src/mc/RemotePtr.hpp"
#include "src/mc/AddressSpace.hpp"

namespace simgrid {
namespace mc {

XBT_PRIVATE void read_element(AddressSpace const& as,
  void* local, RemotePtr<s_xbt_dynar_t> addr, std::size_t i, std::size_t len);
XBT_PRIVATE std::size_t read_length(
  AddressSpace const& as, RemotePtr<s_xbt_dynar_t> addr);

}
}

#endif
