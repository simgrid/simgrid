/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <sys/mman.h>

#include "mc/mc.h"
#include "mc_snapshot.h"
#include "RegionSnapshot.hpp"

extern "C" {

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_RegionSnaphot, mc,
                                "Logging specific to region snapshots");

}

namespace simgrid {
namespace mc {

static inline
const char* to_cstr(RegionType region)
{
  switch (region) {
  case RegionType::Unknown:
    return "unknown";
  case RegionType::Heap:
    return "Heap";
  case RegionType::Data:
    return "Data";
  default:
    return "?";
  }
}

void data_deleter::operator()(void* p) const
{
  switch(type_) {
  case Free:
    free(p);
    break;
  case Munmap:
    munmap(p, size_);
    break;
  }
}

RegionSnapshot dense_region(
  RegionType region_type,
  void *start_addr, void* permanent_addr, size_t size)
{
  simgrid::mc::RegionSnapshot::flat_data_ptr data((char*) malloc(size));
  mc_model_checker->process().read_bytes(data.get(), size,
    remote(permanent_addr),
    simgrid::mc::ProcessIndexDisabled);

  simgrid::mc::RegionSnapshot region(
    region_type, start_addr, permanent_addr, size);
  region.flat_data(std::move(data));

  XBT_DEBUG("New region : type : %s, data : %p (real addr %p), size : %zu",
            to_cstr(region_type), region.flat_data(), permanent_addr, size);
  return std::move(region);
}

/** @brief Take a snapshot of a given region
 *
 * @param type
 * @param start_addr   Address of the region in the simulated process
 * @param permanent_addr Permanent address of this data (for privatized variables, this is the virtual address of the privatized mapping)
 * @param size         Size of the data*
 */
RegionSnapshot region(
  RegionType type, void *start_addr, void* permanent_addr, size_t size)
{
  if (_sg_mc_sparse_checkpoint) {
    return sparse_region(type, start_addr, permanent_addr, size);
  } else  {
    return dense_region(type, start_addr, permanent_addr, size);
  }
}

RegionSnapshot sparse_region(RegionType region_type,
  void *start_addr, void* permanent_addr, size_t size)
{
  simgrid::mc::Process* process = &mc_model_checker->process();

  xbt_assert((((uintptr_t)start_addr) & (xbt_pagesize-1)) == 0,
    "Not at the beginning of a page");
  xbt_assert((((uintptr_t)permanent_addr) & (xbt_pagesize-1)) == 0,
    "Not at the beginning of a page");
  size_t page_count = mc_page_count(size);

  simgrid::mc::PerPageCopy page_data(mc_model_checker->page_store(), *process,
      permanent_addr, page_count);

  simgrid::mc::RegionSnapshot region(
    region_type, start_addr, permanent_addr, size);
  region.page_data(std::move(page_data));
  return std::move(region);
}
  
}
}
