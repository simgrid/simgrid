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

enum class StorageType { NoData = 0, Flat = 1, Chunked = 2 };

class Buffer {
private:
  void* data_ = nullptr;
  std::size_t size_;

  explicit Buffer(std::size_t size) : size_(size) { data_ = ::operator new(size_); }

  Buffer(void* data, std::size_t size) : data_(data), size_(size) {}

public:
  Buffer() = default;
  void clear() noexcept
  {
    ::operator delete(data_);
    data_ = nullptr;
    size_ = 0;
  }

  ~Buffer() noexcept { clear(); }

  static Buffer malloc(std::size_t size) { return Buffer(size); }

  // No copy
  Buffer(Buffer const& buffer) = delete;
  Buffer& operator=(Buffer const& buffer) = delete;

  // Move
  Buffer(Buffer&& that) noexcept : data_(that.data_), size_(that.size_)
  {
    that.data_ = nullptr;
    that.size_ = 0;
  }
  Buffer& operator=(Buffer&& that) noexcept
  {
    clear();
    data_      = that.data_;
    size_      = that.size_;
    that.data_ = nullptr;
    that.size_ = 0;
    return *this;
  }

  void* get() { return data_; }
  const void* get() const { return data_; }
  std::size_t size() const { return size_; }
};

/** A copy/snapshot of a given memory region
 *
 *  Different types of region snapshot storage types exist:
 *
 *  * flat/dense snapshots are a simple copy of the region;
 *
 *  * sparse/per-page snapshots are snaapshots which shared
 *    identical pages.
 *
 *  * privatized (SMPI global variable privatization).
 *
 *  This is handled with a variant based approach:
 *
 *  * `storage_type` identified the type of storage;
 *
 *  * an anonymous enum is used to distinguish the relevant types for
 *    each type.
 */
class RegionSnapshot {
public:
  static const RegionType UnknownRegion = RegionType::Unknown;
  static const RegionType HeapRegion    = RegionType::Heap;
  static const RegionType DataRegion    = RegionType::Data;

protected:
  RegionType region_type_                      = UnknownRegion;
  StorageType storage_type_                    = StorageType::NoData;
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

  Buffer flat_data_;
  ChunkedData page_numbers_;

public:
  RegionSnapshot() {}
  RegionSnapshot(RegionType type, void* start_addr, void* permanent_addr, size_t size)
      : region_type_(type)
      , start_addr_(start_addr)
      , size_(size)
      , permanent_addr_(permanent_addr)
  {
  }
  ~RegionSnapshot()                     = default;
  RegionSnapshot(RegionSnapshot const&) = delete;
  RegionSnapshot& operator=(RegionSnapshot const&) = delete;
  RegionSnapshot(RegionSnapshot&& that)
      : region_type_(that.region_type_)
      , storage_type_(that.storage_type_)
      , object_info_(that.object_info_)
      , start_addr_(that.start_addr_)
      , size_(that.size_)
      , permanent_addr_(that.permanent_addr_)
      , flat_data_(std::move(that.flat_data_))
      , page_numbers_(std::move(that.page_numbers_))
  {
    that.clear();
  }
  RegionSnapshot& operator=(RegionSnapshot&& that)
  {
    region_type_        = that.region_type_;
    storage_type_       = that.storage_type_;
    object_info_        = that.object_info_;
    start_addr_         = that.start_addr_;
    size_               = that.size_;
    permanent_addr_     = that.permanent_addr_;
    flat_data_          = std::move(that.flat_data_);
    page_numbers_       = std::move(that.page_numbers_);
    that.clear();
    return *this;
  }

  // Data

  void clear()
  {
    region_type_  = UnknownRegion;
    storage_type_ = StorageType::NoData;
    page_numbers_.clear();
    flat_data_.clear();
    object_info_    = nullptr;
    start_addr_     = nullptr;
    size_           = 0;
    permanent_addr_ = nullptr;
  }

  void clear_data()
  {
    storage_type_ = StorageType::NoData;
    flat_data_.clear();
    page_numbers_.clear();
  }

  const Buffer& flat_data() const { return flat_data_; }
  Buffer& flat_data() { return flat_data_; }

  ChunkedData const& page_data() const { return page_numbers_; }

  simgrid::mc::ObjectInformation* object_info() const { return object_info_; }
  void object_info(simgrid::mc::ObjectInformation* info) { object_info_ = info; }

  // Other getters

  RemotePtr<void> start() const { return remote(start_addr_); }
  RemotePtr<void> end() const { return remote((char*)start_addr_ + size_); }
  RemotePtr<void> permanent_address() const { return remote(permanent_addr_); }
  std::size_t size() const { return size_; }
  StorageType storage_type() const { return storage_type_; }
  RegionType region_type() const { return region_type_; }

  bool contain(RemotePtr<void> p) const { return p >= start() && p < end(); }

  /** @brief Restore a region from a snapshot */
  void restore();
};

class RegionDense : public RegionSnapshot {
public:
  RegionDense(RegionType type, void* start_addr, void* data_addr, std::size_t size);
};
class RegionSparse : public RegionSnapshot {
public:
  RegionSparse(RegionType type, void* start_addr, void* data_addr, std::size_t size);
};

RegionSnapshot* region(RegionType type, void* start_addr, void* data_addr, std::size_t size);

} // namespace mc
} // namespace simgrid

#endif
