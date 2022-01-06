/* Copyright (c) 2007-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_XBT_MEMORY_MAP_HPP
#define SIMGRID_XBT_MEMORY_MAP_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace simgrid {
namespace xbt {

/** An virtual memory map entry from /proc/$pid/maps */
struct VmMap {
  std::uint64_t start_addr;
  std::uint64_t end_addr;
  int prot;                     /* Memory protection */
  int flags;                    /* Additional memory flags */
  std::uint64_t offset;         /* Offset in the file/whatever */
  char dev_major;               /* Major of the device */
  char dev_minor;               /* Minor of the device */
  unsigned long inode;          /* Inode in the device */
  std::string pathname;         /* Path name of the mapped file */
};

std::vector<VmMap> get_memory_map(pid_t pid);
}
}

#endif
