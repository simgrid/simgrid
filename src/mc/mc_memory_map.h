/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_MEMORY_MAP_H
#define SIMGRID_MC_MEMORY_MAP_H

#include <cstdint>

#include <string>
#include <vector>

#include <sys/types.h>

#include <xbt/base.h>

#include <simgrid_config.h>
#include "mc_forward.hpp"

namespace simgrid {
namespace mc {

/** An virtual memory map entry from /proc/$pid/maps */
struct VmMap {
  std::uint64_t start_addr, end_addr;
  int prot;                     /* Memory protection */
  int flags;                    /* Additional memory flags */
  std::uint64_t offset;         /* Offset in the file/whatever */
  char dev_major;               /* Major of the device */
  char dev_minor;               /* Minor of the device */
  unsigned long inode;          /* Inode in the device */
  std::string pathname;         /* Path name of the mapped file */
};

XBT_PRIVATE std::vector<VmMap> get_memory_map(pid_t pid);

}
}

extern "C" {

XBT_PRIVATE void MC_find_object_address(
  std::vector<simgrid::mc::VmMap> const& maps, simgrid::mc::ObjectInformation* result);

}

#endif
