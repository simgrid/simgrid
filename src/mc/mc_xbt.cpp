/* Copyright (c) 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/sysdep.h>

#include "src/mc/AddressSpace.hpp"
#include "mc_xbt.hpp"

namespace simgrid {
namespace mc {

void read_element(AddressSpace const& as,
  void* local, remote_ptr<s_xbt_dynar_t> addr, size_t i, size_t len)
{
  s_xbt_dynar_t d;
  as.read_bytes(&d, sizeof(d), addr);
  if (i >= d.used)
    xbt_die("Out of bound index %zi/%lu", i, d.used);
  if (len != d.elmsize)
    xbt_die("Bad size in simgrid::mc::read_element");
  as.read_bytes(local, len, remote(xbt_dynar_get_ptr(&d, i)));
}

std::size_t read_length(AddressSpace const& as, remote_ptr<s_xbt_dynar_t> addr)
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
