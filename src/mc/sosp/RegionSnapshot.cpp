/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstdlib>

#include <sys/mman.h>
#ifdef __FreeBSD__
#define MAP_POPULATE MAP_PREFAULT_READ
#endif

#include "src/mc/ModelChecker.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_forward.hpp"

#include "src/mc/mc_smx.hpp"
#include "src/mc/sosp/RegionSnapshot.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_RegionSnaphot, mc, "Logging specific to region snapshots");

namespace simgrid {
namespace mc {

/** @brief Take a snapshot of a given region
 *
 * @param type
 * @param start_addr   Address of the region in the simulated process
 * @param permanent_addr Permanent address of this data (for privatized variables, this is the virtual address of the
 * privatized mapping)
 * @param size         Size of the data*
 */
RegionSnapshot* region(RegionType type, void* start_addr, void* permanent_addr, size_t size)
{
    return new RegionSparse(type, start_addr, permanent_addr, size);
}

RegionSparse::RegionSparse(RegionType region_type, void* start_addr, void* permanent_addr, size_t size)
    : RegionSnapshot(region_type, start_addr, permanent_addr, size)
{
  simgrid::mc::RemoteClient* process = &mc_model_checker->process();
  assert(process != nullptr);

  xbt_assert((((uintptr_t)start_addr) & (xbt_pagesize - 1)) == 0, "Start address not at the beginning of a page");
  xbt_assert((((uintptr_t)permanent_addr) & (xbt_pagesize - 1)) == 0,
             "Permanent address not at the beginning of a page");

  page_numbers_ =
      ChunkedData(mc_model_checker->page_store(), *process, RemotePtr<void>(permanent_addr), mmu::chunk_count(size));
}

/** @brief Restore a region from a snapshot
 *
 *  @param region     Target region
 */
void RegionSnapshot::restore()
{
      xbt_assert(((permanent_address().address()) & (xbt_pagesize - 1)) == 0, "Not at the beginning of a page");
      xbt_assert(simgrid::mc::mmu::chunk_count(size()) == page_data().page_count());

      for (size_t i = 0; i != page_data().page_count(); ++i) {
        void* target_page = (void*)simgrid::mc::mmu::join(i, (std::uintptr_t)(void*)permanent_address().address());
        const void* source_page = page_data().page(i);
        mc_model_checker->process().write_bytes(source_page, xbt_pagesize, remote(target_page));
      }
}

} // namespace mc
} // namespace simgrid
