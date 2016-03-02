/* Copyright (c) 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstddef>

#include "src/mc/RemotePtr.hpp"
#include "src/mc/AddressSpace.hpp"
#include "src/mc/mc_xbt.hpp"

#include <xbt/dynar.h>
#include <xbt/sysdep.h>

namespace simgrid {
namespace mc {

void read_element(AddressSpace const& as,
  void* local, RemotePtr<s_xbt_dynar_t> addr, std::size_t i, std::size_t len)
{
  s_xbt_dynar_t d;
  as.read_bytes(&d, sizeof(d), addr);
  if (i >= d.used)
    xbt_die("Out of bound index %zi/%lu", i, d.used);
  if (len != d.elmsize)
    xbt_die("Bad size in simgrid::mc::read_element");
  as.read_bytes(local, len, remote(xbt_dynar_get_ptr(&d, i)));
}

std::size_t read_length(AddressSpace const& as, RemotePtr<s_xbt_dynar_t> addr)
{
  if (!addr)
    return 0;
  unsigned long res;
  as.read_bytes(&res, sizeof(res),
    remote(&((xbt_dynar_t)addr.address())->used));
  return res;
}

}
}
