/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_OBJECT_INFO_H
#define SIMGRID_MC_OBJECT_INFO_H

#include <vector>
#include <memory>

#include <xbt/base.h>

#include "mc_forward.hpp"
#include "mc_memory_map.h"

XBT_PRIVATE std::shared_ptr<simgrid::mc::ObjectInformation> MC_find_object_info(
  std::vector<simgrid::mc::VmMap> const& maps, const char* name, int executable);
XBT_PRIVATE  void MC_post_process_object_info(simgrid::mc::Process* process, simgrid::mc::ObjectInformation* info);

XBT_PRIVATE  void MC_dwarf_get_variables(simgrid::mc::ObjectInformation* info);
XBT_PRIVATE  void MC_dwarf_get_variables_libdw(simgrid::mc::ObjectInformation* info);

#endif
