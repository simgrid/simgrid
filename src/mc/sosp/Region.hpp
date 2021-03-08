/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SOSP_REGION_HPP
#define SIMGRID_MC_SOSP_REGION_HPP

#include "src/mc/remote/RemotePtr.hpp"
#include "src/mc/sosp/ChunkedData.hpp"

#include <memory>
#include <vector>

namespace simgrid {
namespace mc {

enum class RegionType { Heap = 1, Data = 2 };

/** A copy/snapshot of a given memory region, where identical pages are stored only once */
class Region {
public:
  static const RegionType HeapRegion    = RegionType::Heap;
  static const RegionType DataRegion    = RegionType::Data;

private:
  RegionType region_type_;
  simgrid::mc::ObjectInformation* object_info_ = nullptr;

  /** @brief  Virtual address of the region in the simulated process */
  void* start_addr_ = nullptr;

  /** @brief Size of the data region in bytes */
  std::size_t size_ = 0;

  ChunkedData chunks_;

public:
  Region(RegionType type, void* start_addr, size_t size);
  Region(Region const&) = delete;
  Region& operator=(Region const&) = delete;
  Region(Region&& that)            = delete;
  Region& operator=(Region&& that) = delete;

  // Data

  ChunkedData const& get_chunks() const { return chunks_; }

  simgrid::mc::ObjectInformation* object_info() const { return object_info_; }
  void object_info(simgrid::mc::ObjectInformation* info) { object_info_ = info; }

  // Other getters

  RemotePtr<void> start() const { return remote(start_addr_); }
  RemotePtr<void> end() const { return remote((char*)start_addr_ + size_); }
  std::size_t size() const { return size_; }
  RegionType region_type() const { return region_type_; }

  bool contain(RemotePtr<void> p) const { return p >= start() && p < end(); }

  /** @brief Restore a region from a snapshot */
  void restore() const;

  /** @brief Read memory that was snapshotted in this region
   *
   *  @param target  Buffer to store contiguously the value if it spans over several pages
   *  @param addr    Process (non-snapshot) address of the data
   *  @param size    Size of the data to read in bytes
   *  @return Pointer where the data is located (either target buffer or original location)
   */
  void* read(void* target, const void* addr, std::size_t size) const;
};

} // namespace mc
} // namespace simgrid

int MC_snapshot_region_memcmp(const void* addr1, const simgrid::mc::Region* region1, const void* addr2,
                              const simgrid::mc::Region* region2, std::size_t size);

static XBT_ALWAYS_INLINE void* MC_region_read_pointer(const simgrid::mc::Region* region, const void* addr)
{
  void* res;
  return *(void**)region->read(&res, addr, sizeof(void*));
}

#endif
