/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstdlib>

#include <sys/mman.h>

#include "mc/mc.h"
#include "src/mc/mc_snapshot.h"

#include "src/mc/ChunkedData.hpp"
#include "src/mc/RegionSnapshot.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_RegionSnaphot, mc,
                                "Logging specific to region snapshots");

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

Buffer::Buffer(std::size_t size, Type type) : size_(size), type_(type)
{
  switch(type_) {
  case Type::Malloc:
    data_ = ::malloc(size_);
    break;
  case Type::Mmap:
    data_ = ::mmap(nullptr, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_POPULATE, -1, 0);
    if (data_ == MAP_FAILED) {
      data_ = nullptr;
      size_ = 0;
      type_ = Type::Malloc;
      throw std::bad_alloc();
    }
    break;
  default:
    abort();
  }
}

void Buffer::clear() noexcept
{
  switch(type_) {
  case Type::Malloc:
    std::free(data_);
    break;
  case Type::Mmap:
    if (munmap(data_, size_) != 0)
      abort();
    break;
  default:
    abort();
  }
  data_ = nullptr;
  size_ = 0;
  type_ = Type::Malloc;
}

RegionSnapshot dense_region(
  RegionType region_type,
  void *start_addr, void* permanent_addr, size_t size)
{
  // When KSM support is enables, we allocate memory using mmap:
  // * we don't want to madvise bits of the heap;
  // * mmap gives data aligned on page boundaries which is merge friendly.
  simgrid::mc::Buffer data;
  if (_sg_mc_ksm)
    data = Buffer::mmap(size);
  else
    data = Buffer::malloc(size);

  mc_model_checker->process().read_bytes(data.get(), size,
    remote(permanent_addr),
    simgrid::mc::ProcessIndexDisabled);

  if (_sg_mc_ksm)
    // Mark the region as mergeable *after* we have written into it.
    // Trying to merge them before is useless/counterproductive.
    madvise(data.get(), size, MADV_MERGEABLE);

  simgrid::mc::RegionSnapshot region(
    region_type, start_addr, permanent_addr, size);
  region.flat_data(std::move(data));

  XBT_DEBUG("New region : type : %s, data : %p (real addr %p), size : %zu",
            to_cstr(region_type), region.flat_data().get(), permanent_addr, size);
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
  RegionType type, void *start_addr, void* permanent_addr, size_t size,
  RegionSnapshot const* ref_region)
{
  if (_sg_mc_sparse_checkpoint)
    return sparse_region(type, start_addr, permanent_addr, size, ref_region);
  else
    return dense_region(type, start_addr, permanent_addr, size);
}

RegionSnapshot sparse_region(RegionType region_type,
  void *start_addr, void* permanent_addr, size_t size,
  RegionSnapshot const* ref_region)
{
  simgrid::mc::Process* process = &mc_model_checker->process();
  assert(process != nullptr);

  bool use_soft_dirty = _sg_mc_sparse_checkpoint && _sg_mc_soft_dirty
    && ref_region != nullptr
    && ref_region->storage_type() == simgrid::mc::StorageType::Chunked;

  xbt_assert((((uintptr_t)start_addr) & (xbt_pagesize-1)) == 0,
    "Not at the beginning of a page");
  xbt_assert((((uintptr_t)permanent_addr) & (xbt_pagesize-1)) == 0,
    "Not at the beginning of a page");
  size_t page_count = simgrid::mc::mmu::chunkCount(size);

  std::vector<std::uint64_t> pagemap;
  const size_t* ref_page_numbers = nullptr;
  if (use_soft_dirty) {
    pagemap.resize(page_count);
    process->read_pagemap(pagemap.data(),
      simgrid::mc::mmu::split((std::size_t) permanent_addr).first, page_count);
    ref_page_numbers = ref_region->page_data().pagenos();
  }

  simgrid::mc::ChunkedData page_data(
    mc_model_checker->page_store(), *process, permanent_addr, page_count,
    ref_page_numbers,
    use_soft_dirty ? pagemap.data() : nullptr);

  simgrid::mc::RegionSnapshot region(
    region_type, start_addr, permanent_addr, size);
  region.page_data(std::move(page_data));
  return std::move(region);
}
  
}
}
