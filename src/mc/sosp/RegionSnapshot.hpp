/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_REGION_SNAPSHOT_HPP
#define SIMGRID_MC_REGION_SNAPSHOT_HPP

#include "src/mc/remote/RemotePtr.hpp"
#include "src/mc/sosp/ChunkedData.hpp"

#include <memory>
#include <vector>

namespace simgrid {
namespace mc {

enum class RegionType { Unknown = 0, Heap = 1, Data = 2 };

/** A copy/snapshot of a given memory region, where identical pages are stored only once */
class RegionSnapshot {
public:
  static const RegionType UnknownRegion = RegionType::Unknown;
  static const RegionType HeapRegion    = RegionType::Heap;
  static const RegionType DataRegion    = RegionType::Data;

protected:
  RegionType region_type_                      = UnknownRegion;
  simgrid::mc::ObjectInformation* object_info_ = nullptr;

  /** @brief  Virtual address of the region in the simulated process */
  void* start_addr_ = nullptr;

  /** @brief Size of the data region in bytes */
  std::size_t size_ = 0;

  /** @brief Permanent virtual address of the region
   *
   * This is usually the same address as the simuilated process address.
   * However, when using SMPI privatization of global variables,
   * each SMPI process has its own set of global variables stored
   * at a different virtual address. The scheduler maps those region
   * on the region of the global variables.
   *
   * */
  void* permanent_addr_ = nullptr;

  ChunkedData page_numbers_;

public:
  RegionSnapshot(RegionType type, void* start_addr, void* permanent_addr, size_t size);
  ~RegionSnapshot()                     = default;
  RegionSnapshot(RegionSnapshot const&) = delete;
  RegionSnapshot& operator=(RegionSnapshot const&) = delete;
  RegionSnapshot(RegionSnapshot&& that)
      : region_type_(that.region_type_)
      , object_info_(that.object_info_)
      , start_addr_(that.start_addr_)
      , size_(that.size_)
      , permanent_addr_(that.permanent_addr_)
      , page_numbers_(std::move(that.page_numbers_))
  {
    that.clear();
  }
  RegionSnapshot& operator=(RegionSnapshot&& that)
  {
    region_type_        = that.region_type_;
    object_info_        = that.object_info_;
    start_addr_         = that.start_addr_;
    size_               = that.size_;
    permanent_addr_     = that.permanent_addr_;
    page_numbers_       = std::move(that.page_numbers_);
    that.clear();
    return *this;
  }

  // Data

  void clear()
  {
    region_type_  = UnknownRegion;
    page_numbers_.clear();
    object_info_    = nullptr;
    start_addr_     = nullptr;
    size_           = 0;
    permanent_addr_ = nullptr;
  }

  void clear_data()
  {
    page_numbers_.clear();
  }

  ChunkedData const& page_data() const { return page_numbers_; }

  simgrid::mc::ObjectInformation* object_info() const { return object_info_; }
  void object_info(simgrid::mc::ObjectInformation* info) { object_info_ = info; }

  // Other getters

  RemotePtr<void> start() const { return remote(start_addr_); }
  RemotePtr<void> end() const { return remote((char*)start_addr_ + size_); }
  RemotePtr<void> permanent_address() const { return remote(permanent_addr_); }
  std::size_t size() const { return size_; }
  RegionType region_type() const { return region_type_; }

  bool contain(RemotePtr<void> p) const { return p >= start() && p < end(); }

  /** @brief Restore a region from a snapshot */
  void restore();
};

} // namespace mc
} // namespace simgrid

#endif
