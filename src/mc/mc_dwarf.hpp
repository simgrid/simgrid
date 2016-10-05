/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#if !defined(SIMGRID_MC_DWARF_HPP)
#define SIMGRID_MC_DWARF_HPP

#include <memory>

#include <string.h>

#include <xbt/base.h>
#include <xbt/sysdep.h>

#define DW_LANG_Objc DW_LANG_ObjC       /* fix spelling error in older dwarf.h */
#include <dwarf.h>

#include "src/mc/mc_forward.hpp"

namespace simgrid {
namespace dwarf {

XBT_PRIVATE const char* attrname(int attr);
XBT_PRIVATE const char* tagname(int tag);

XBT_PRIVATE void* resolve_member(
  const void* base, simgrid::mc::Type* type, simgrid::mc::Member* member,
  simgrid::mc::AddressSpace* snapshot, int process_index);

XBT_PRIVATE
int dwarf_register_to_libunwind(int dwarf_register);

}
}

#endif
