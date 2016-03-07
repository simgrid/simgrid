/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <iostream>

#include <limits.h>
#include <stdlib.h>
#include <sys/types.h>
#ifndef WIN32
#include <sys/mman.h>
#include <unistd.h>

#include "../xbt/memory_map.hpp"

#include "private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_memory, smpi, "Memory layout support for SMPI");

#define TOPAGE(addr) (void *)(((unsigned long)(addr) / xbt_pagesize) * xbt_pagesize)

#define PROT_RWX (PROT_READ | PROT_WRITE | PROT_EXEC)
#define PROT_RW (PROT_READ | PROT_WRITE )
#define PROT_RX (PROT_READ | PROT_EXEC )

void smpi_get_executable_global_size(void)
{
  char buffer[PATH_MAX];
  char* full_name = realpath(xbt_binary_name, buffer);
  if (full_name == nullptr)
    xbt_die("Could not resolve binary file name");

  std::vector<simgrid::xbt::VmMap> map = simgrid::xbt::get_memory_map(getpid());
  for (auto i = map.begin(); i != map.end() ; ++i) {
    // TODO, In practice, this implementation would not detect a completely
    // anonymous data segment. This does not happen in practice, however.

    // File backed RW entry:
    if (i->pathname == full_name && (i->prot & PROT_RWX) == PROT_RW) {
      smpi_start_data_exe = (char*) i->start_addr;
      smpi_size_data_exe = i->end_addr - i->start_addr;
      ++i;
      /* Here we are making the assumption that a suitable empty region
         following the rw- area is the end of the data segment. It would
         be better to check with the size of the data segment. */
      if (i != map.end() && i->pathname.empty() && (i->prot & PROT_RWX) == PROT_RW
          && i->start_addr == (std::uint64_t) smpi_start_data_exe + smpi_size_data_exe) {
        smpi_size_data_exe = i->end_addr - (std::uint64_t) smpi_start_data_exe;
      }
      return;
    }
  }
  xbt_die("Did not find my data segment.");
}
#endif
