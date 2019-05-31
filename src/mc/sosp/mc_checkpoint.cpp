/* Copyright (c) 2008-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef WIN32
#include <sys/mman.h>
#endif

#include "xbt/file.hpp"

#include "src/mc/mc_config.hpp"
#include "src/mc/mc_hash.hpp"
#include "src/mc/mc_private.hpp"
#include "src/mc/mc_smx.hpp"
#include "src/mc/sosp/mc_snapshot.hpp"

using simgrid::mc::remote;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_checkpoint, mc, "Logging specific to mc_checkpoint");

#define PROT_RWX (PROT_READ | PROT_WRITE | PROT_EXEC)
#define PROT_RW (PROT_READ | PROT_WRITE)
#define PROT_RX (PROT_READ | PROT_EXEC)

namespace simgrid {
namespace mc {

/************************************  Free functions **************************************/
/*****************************************************************************************/



/** @brief Fills the position of the segments (executable, read-only, read/write).
 * */
// TODO, use the ELF segment information for more robustness
void find_object_address(std::vector<simgrid::xbt::VmMap> const& maps, simgrid::mc::ObjectInformation* result)
{
  std::string name = simgrid::xbt::Path(result->file_name).get_base_name();

  for (size_t i = 0; i < maps.size(); ++i) {
    simgrid::xbt::VmMap const& reg = maps[i];
    if (maps[i].pathname.empty())
      continue;
    std::string map_basename = simgrid::xbt::Path(maps[i].pathname).get_base_name();
    if (map_basename != name)
      continue;

    // This is the non-GNU_RELRO-part of the data segment:
    if (reg.prot == PROT_RW) {
      xbt_assert(not result->start_rw, "Multiple read-write segments for %s, not supported", maps[i].pathname.c_str());
      result->start_rw = (char*)reg.start_addr;
      result->end_rw   = (char*)reg.end_addr;

      // The next VMA might be end of the data segment:
      if (i + 1 < maps.size() && maps[i + 1].pathname.empty() && maps[i + 1].prot == PROT_RW &&
          maps[i + 1].start_addr == reg.end_addr)
        result->end_rw = (char*)maps[i + 1].end_addr;
    }

    // This is the text segment:
    else if (reg.prot == PROT_RX) {
      xbt_assert(not result->start_exec, "Multiple executable segments for %s, not supported",
                 maps[i].pathname.c_str());
      result->start_exec = (char*)reg.start_addr;
      result->end_exec   = (char*)reg.end_addr;

      // The next VMA might be end of the data segment:
      if (i + 1 < maps.size() && maps[i + 1].pathname.empty() && maps[i + 1].prot == PROT_RW &&
          maps[i + 1].start_addr == reg.end_addr) {
        result->start_rw = (char*)maps[i + 1].start_addr;
        result->end_rw   = (char*)maps[i + 1].end_addr;
      }
    }

    // This is the GNU_RELRO-part of the data segment:
    else if (reg.prot == PROT_READ) {
      xbt_assert(not result->start_ro, "Multiple read only segments for %s, not supported", maps[i].pathname.c_str());
      result->start_ro = (char*)reg.start_addr;
      result->end_ro   = (char*)reg.end_addr;
    }
  }

  result->start = result->start_rw;
  if ((const void*)result->start_ro < result->start)
    result->start = result->start_ro;
  if ((const void*)result->start_exec < result->start)
    result->start = result->start_exec;

  result->end = result->end_rw;
  if (result->end_ro && (const void*)result->end_ro > result->end)
    result->end = result->end_ro;
  if (result->end_exec && (const void*)result->end_exec > result->end)
    result->end = result->end_exec;

  xbt_assert(result->start_exec || result->start_rw || result->start_ro);
}

} // namespace mc
} // namespace simgrid
