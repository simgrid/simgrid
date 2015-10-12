/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#if !defined(SIMGRID_MC_DWARF_HPP)
#define SIMGRID_MC_DWARF_HPP

#include <memory>

#include <string.h>

#include <xbt/sysdep.h>

#define DW_LANG_Objc DW_LANG_ObjC       /* fix spelling error in older dwarf.h */
#include <dwarf.h>

#include "mc/Variable.hpp"
#include "mc/mc_memory_map.h"

namespace simgrid {
namespace dwarf {

XBT_PRIVATE const char* attrname(int attr);
XBT_PRIVATE const char* tagname(int tag);

}
}

XBT_PRIVATE std::shared_ptr<simgrid::mc::ObjectInformation> MC_find_object_info(
  std::vector<simgrid::mc::VmMap> const& maps, const char* name, int executable);
XBT_PRIVATE void MC_post_process_object_info(simgrid::mc::Process* process, simgrid::mc::ObjectInformation* info);

XBT_PRIVATE void MC_dwarf_get_variables(simgrid::mc::ObjectInformation* info);
XBT_PRIVATE void MC_dwarf_get_variables_libdw(simgrid::mc::ObjectInformation* info);

XBT_PRIVATE void* mc_member_resolve(
  const void* base, simgrid::mc::Type* type, simgrid::mc::Member* member,
  simgrid::mc::AddressSpace* snapshot, int process_index);

#endif
