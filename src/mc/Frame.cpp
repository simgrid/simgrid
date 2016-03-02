/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <libunwind.h>

#include <xbt/sysdep.h>

#include "src/mc/Frame.hpp"

namespace simgrid {
namespace mc {

void* Frame::frame_base(unw_cursor_t& unw_cursor) const
{
  simgrid::dwarf::Location location = simgrid::dwarf::resolve(
                             frame_base_location, object_info,
                             &unw_cursor, nullptr, nullptr, -1);
  if (location.in_memory())
    return location.address();
  else if (location.in_register()) {
    // This is a special case.
    // The register if not the location of the frame base
    // (a frame base cannot be located in a register)
    // Instead, DWARF defines this to mean that the register
    // contains the address of the frame base.
    unw_word_t word;
    unw_get_reg(&unw_cursor, location.register_id(), &word);
    return (void*) word;
  }
  else xbt_die("Unexpected location type");
}

}
}