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

RegionDense::RegionDense(RegionType region_type, void* start_addr, void* permanent_addr, size_t size)
    : RegionSnapshot(region_type, start_addr, permanent_addr, size)
{
  flat_data_ = Buffer::malloc(size);

  mc_model_checker->process().read_bytes(flat_data_.get(), size, remote(permanent_addr),
                                         simgrid::mc::ProcessIndexDisabled);

  storage_type_ = StorageType::Flat;
  page_numbers_.clear();
  privatized_regions_.clear();

  XBT_DEBUG("New region : type : %s, data : %p (real addr %p), size : %zu",
            (region_type == RegionType::Heap ? "Heap" : (region_type == RegionType::Data ? "Data" : "?")),
            flat_data().get(), permanent_addr, size);
}

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
  if (_sg_mc_sparse_checkpoint)
    return new RegionSparse(type, start_addr, permanent_addr, size);
  else
    return new RegionDense(type, start_addr, permanent_addr, size);
}

RegionSparse::RegionSparse(RegionType region_type, void* start_addr, void* permanent_addr, size_t size)
    : RegionSnapshot(region_type, start_addr, permanent_addr, size)
{
  simgrid::mc::RemoteClient* process = &mc_model_checker->process();
  assert(process != nullptr);

  xbt_assert((((uintptr_t)start_addr) & (xbt_pagesize - 1)) == 0, "Start address not at the beginning of a page");
  xbt_assert((((uintptr_t)permanent_addr) & (xbt_pagesize - 1)) == 0,
             "Permanent address not at the beginning of a page");

  storage_type_ = StorageType::Chunked;
  flat_data_.clear();
  privatized_regions_.clear();

  page_numbers_ =
      ChunkedData(mc_model_checker->page_store(), *process, RemotePtr<void>(permanent_addr), mmu::chunk_count(size));
}

#if HAVE_SMPI
RegionPrivatized::RegionPrivatized(RegionType region_type, void* start_addr, void* permanent_addr, std::size_t size)
    : RegionSnapshot(region_type, start_addr, permanent_addr, size)
{
  storage_type_ = StorageType::Privatized;

  size_t process_count = MC_smpi_process_count();

  // Read smpi_privatization_regions from MCed:
  smpi_privatization_region_t remote_smpi_privatization_regions;
  mc_model_checker->process().read_variable("smpi_privatization_regions", &remote_smpi_privatization_regions,
                                            sizeof(remote_smpi_privatization_regions));
  s_smpi_privatization_region_t privatization_regions[process_count];
  mc_model_checker->process().read_bytes(&privatization_regions, sizeof(privatization_regions),
                                         remote(remote_smpi_privatization_regions));

  privatized_regions_.reserve(process_count);
  for (size_t i = 0; i < process_count; i++)
    privatized_regions_.push_back(std::unique_ptr<simgrid::mc::RegionSnapshot>(
        region(region_type, start_addr, privatization_regions[i].address, size)));

  flat_data_.clear();
  page_numbers_.clear();
}
#endif

} // namespace mc
} // namespace simgrid
