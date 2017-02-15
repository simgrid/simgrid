/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstdint>
#include <climits>

#include <vector>

#include <stdlib.h>
#include <sys/types.h>

#ifndef WIN32
#include <sys/mman.h>
#include <unistd.h>

#include "../xbt/memory_map.hpp"

#include "private.h"
#include "private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_memory, smpi, "Memory layout support for SMPI");

static const int PROT_RWX = (PROT_READ | PROT_WRITE | PROT_EXEC);
static const int PROT_RW  = (PROT_READ | PROT_WRITE );
XBT_ATTRIB_UNUSED static const int PROT_RX  = (PROT_READ | PROT_EXEC );

void smpi_get_executable_global_size()
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
          && (char*)i->start_addr ==  smpi_start_data_exe + smpi_size_data_exe) {
        smpi_size_data_exe = (char*)i->end_addr - smpi_start_data_exe;
      }
      return;
    }
  }
  xbt_die("Did not find my data segment.");
}
#endif
