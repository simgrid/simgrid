/* Copyright (c) 2007-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstdlib>

#include <sys/mman.h>
#ifdef __FreeBSD__
# define MAP_POPULATE MAP_PREFAULT_READ
#endif

#include "mc/mc.h"
#include "src/mc/mc_snapshot.hpp"

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
    data_ = ::operator new(size_);
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
    ::operator delete(data_);
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
  // * we don't want to advise bits of the heap as mergable
  // * mmap gives data aligned on page boundaries which is merge friendly
  simgrid::mc::Buffer data;
  if (_sg_mc_ksm)
    data = Buffer::mmap(size);
  else
    data = Buffer::malloc(size);

  mc_model_checker->process().read_bytes(data.get(), size,
    remote(permanent_addr),
    simgrid::mc::ProcessIndexDisabled);

#ifdef __linux__
  if (_sg_mc_ksm)
    // Mark the region as mergeable *after* we have written into it.
    // Trying to merge them before is useless/counterproductive.
    madvise(data.get(), size, MADV_MERGEABLE);
#endif

  simgrid::mc::RegionSnapshot region(
    region_type, start_addr, permanent_addr, size);
  region.flat_data(std::move(data));

  XBT_DEBUG("New region : type : %s, data : %p (real addr %p), size : %zu",
            to_cstr(region_type), region.flat_data().get(), permanent_addr, size);
  return region;
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
  if (_sg_mc_sparse_checkpoint)
    return sparse_region(type, start_addr, permanent_addr, size);
  else
    return dense_region(type, start_addr, permanent_addr, size);
}

RegionSnapshot sparse_region(RegionType region_type,
  void *start_addr, void* permanent_addr, size_t size)
{
  simgrid::mc::RemoteClient* process = &mc_model_checker->process();
  assert(process != nullptr);

  xbt_assert((((uintptr_t)start_addr) & (xbt_pagesize-1)) == 0,
    "Not at the beginning of a page");
  xbt_assert((((uintptr_t)permanent_addr) & (xbt_pagesize-1)) == 0,
    "Not at the beginning of a page");
  size_t page_count = simgrid::mc::mmu::chunkCount(size);

  simgrid::mc::ChunkedData page_data(mc_model_checker->page_store(), *process, RemotePtr<void>(permanent_addr),
                                     page_count);

  simgrid::mc::RegionSnapshot region(
    region_type, start_addr, permanent_addr, size);
  region.page_data(std::move(page_data));
  return region;
}

}
}
