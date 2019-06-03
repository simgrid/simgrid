/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/ModelChecker.hpp"
#include "src/mc/mc_config.hpp"
#include "src/mc/mc_forward.hpp"

#include "src/mc/mc_smx.hpp"
#include "src/mc/sosp/Region.hpp"

#include <cstdlib>
#include <sys/mman.h>
#ifdef __FreeBSD__
#define MAP_POPULATE MAP_PREFAULT_READ
#endif

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_RegionSnaphot, mc, "Logging specific to region snapshots");

namespace simgrid {
namespace mc {

Region::Region(RegionType region_type, void* start_addr, size_t size)
    : region_type_(region_type), start_addr_(start_addr), size_(size)
{
  xbt_assert((((uintptr_t)start_addr) & (xbt_pagesize - 1)) == 0, "Start address not at the beginning of a page");

  chunks_ = ChunkedData(mc_model_checker->page_store(), mc_model_checker->process(), RemotePtr<void>(start_addr),
                        mmu::chunk_count(size));
}

/** @brief Restore a region from a snapshot
 *
 *  @param region     Target region
 */
void Region::restore()
{
  xbt_assert(((start().address()) & (xbt_pagesize - 1)) == 0, "Not at the beginning of a page");
  xbt_assert(simgrid::mc::mmu::chunk_count(size()) == get_chunks().page_count());

  for (size_t i = 0; i != get_chunks().page_count(); ++i) {
    void* target_page       = (void*)simgrid::mc::mmu::join(i, (std::uintptr_t)(void*)start().address());
    const void* source_page = get_chunks().page(i);
    mc_model_checker->process().write_bytes(source_page, xbt_pagesize, remote(target_page));
  }
}

} // namespace mc
} // namespace simgrid
