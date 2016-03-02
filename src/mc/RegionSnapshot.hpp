/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_REGION_SNAPSHOT_HPP
#define SIMGRID_MC_REGION_SNAPSHOT_HPP

#include <cstddef>
#include <utility>

#include <memory>
#include <vector>

#include <xbt/base.h>

#include "src/mc/RemotePtr.hpp"
#include "src/mc/PageStore.hpp"
#include "src/mc/AddressSpace.hpp"
#include "src/mc/ChunkedData.hpp"

namespace simgrid {
namespace mc {

enum class RegionType {
  Unknown = 0,
  Heap = 1,
  Data = 2
};

// TODO, use Boost.Variant instead of this
enum class StorageType {
  NoData = 0,
  Flat = 1,
  Chunked = 2,
  Privatized = 3
};

class Buffer {
private:
  enum class Type {
    Malloc,
    Mmap
  };
  void* data_ = nullptr;
  std::size_t size_;
  Type type_ = Type::Malloc;
private:
  Buffer(std::size_t size, Type type = Type::Malloc);
  Buffer(void* data, std::size_t size, Type type = Type::Malloc) :
    data_(data), size_(size), type_(type) {}
public:
  Buffer() {}
  void clear() noexcept;
  ~Buffer() noexcept { clear(); }

  static Buffer malloc(std::size_t size)
  {
    return Buffer(size, Type::Malloc);
  }
  static Buffer mmap(std::size_t size)
  {
    return Buffer(size, Type::Mmap);
  }

  // No copy
  Buffer(Buffer const& buffer) = delete;
  Buffer& operator=(Buffer const& buffer) = delete;

  // Move
  Buffer(Buffer&& that) noexcept
    : data_(that.data_), size_(that.size_), type_(that.type_)
  {
    that.data_ = nullptr;
    that.size_ = 0;
    that.type_ = Type::Malloc;
  }
  Buffer& operator=(Buffer&& that) noexcept
  {
    clear();
    data_ = that.data_;
    size_ = that.size_;
    type_ = that.type_;
    that.data_ = nullptr;
    that.size_ = 0;
    that.type_ = Type::Malloc;
    return *this;
  }

  void* get()              { return data_; }
  const void* get()  const { return data_; }
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
 *  * privatized (SMPI global variable privatisation).
 *
 *  This is handled with a variant based approch:
 *
 *  * `storage_type` identified the type of storage;
 *
 *  * an anonymous enum is used to distinguish the relevant types for
 *    each type.
 */
class RegionSnapshot {
public:
  static const RegionType UnknownRegion = RegionType::Unknown;
  static const RegionType HeapRegion = RegionType::Heap;
  static const RegionType DataRegion = RegionType::Data;
private:
  RegionType region_type_;
  StorageType storage_type_;
  simgrid::mc::ObjectInformation* object_info_;

  /** @brief  Virtual address of the region in the simulated process */
  void *start_addr_;

  /** @brief Size of the data region in bytes */
  std::size_t size_;

  /** @brief Permanent virtual address of the region
   *
   * This is usually the same address as the simuilated process address.
   * However, when using SMPI privatization of global variables,
   * each SMPI process has its own set of global variables stored
   * at a different virtual address. The scheduler maps those region
   * on the region of the global variables.
   *
   * */
  void *permanent_addr_;

  Buffer flat_data_;
  ChunkedData page_numbers_;
  std::vector<RegionSnapshot> privatized_regions_;
public:
  RegionSnapshot() :
    region_type_(UnknownRegion),
    storage_type_(StorageType::NoData),
    object_info_(nullptr),
    start_addr_(nullptr),
    size_(0),
    permanent_addr_(nullptr)
  {}
  RegionSnapshot(RegionType type, void *start_addr, void* permanent_addr, size_t size) :
    region_type_(type),
    storage_type_(StorageType::NoData),
    object_info_(nullptr),
    start_addr_(start_addr),
    size_(size),
    permanent_addr_(permanent_addr)
  {}
  ~RegionSnapshot() {}
  RegionSnapshot(RegionSnapshot const&) = default;
  RegionSnapshot& operator=(RegionSnapshot const&) = default;
  RegionSnapshot(RegionSnapshot&& that)
  {
    region_type_ = that.region_type_;
    storage_type_ = that.storage_type_;
    object_info_ = that.object_info_;
    start_addr_ = that.start_addr_;
    size_ = that.size_;
    permanent_addr_ = that.permanent_addr_;
    flat_data_ = std::move(that.flat_data_);
    page_numbers_ = std::move(that.page_numbers_);
    privatized_regions_ = std::move(that.privatized_regions_);
    that.clear();
  }
  RegionSnapshot& operator=(RegionSnapshot&& that)
  {
    region_type_ = that.region_type_;
    storage_type_ = that.storage_type_;
    object_info_ = that.object_info_;
    start_addr_ = that.start_addr_;
    size_ = that.size_;
    permanent_addr_ = that.permanent_addr_;
    flat_data_ = std::move(that.flat_data_);
    page_numbers_ = std::move(that.page_numbers_);
    privatized_regions_ = std::move(that.privatized_regions_);
    that.clear();
    return *this;
  }

  // Data

  void clear()
  {
    region_type_ = UnknownRegion;
    storage_type_ = StorageType::NoData;
    privatized_regions_.clear();
    page_numbers_.clear();
    flat_data_.clear();
    object_info_ = nullptr;
    start_addr_ = nullptr;
    size_ = 0;
    permanent_addr_ = nullptr;
  }

  void clear_data()
  {
    storage_type_ = StorageType::NoData;
    flat_data_.clear();
    page_numbers_.clear();
    privatized_regions_.clear();
  }
  
  void flat_data(Buffer data)
  {
    storage_type_ = StorageType::Flat;
    flat_data_ = std::move(data);
    page_numbers_.clear();
    privatized_regions_.clear();
  }
  const Buffer& flat_data() const { return flat_data_; }
  Buffer& flat_data()             { return flat_data_; }

  void page_data(ChunkedData page_data)
  {
    storage_type_ = StorageType::Chunked;
    flat_data_.clear();
    page_numbers_ = std::move(page_data);
    privatized_regions_.clear();
  }
  ChunkedData const& page_data() const { return page_numbers_; }

  void privatized_data(std::vector<RegionSnapshot> data)
  {
    storage_type_ = StorageType::Privatized;
    flat_data_.clear();
    page_numbers_.clear();
    privatized_regions_ = std::move(data);
  }
  std::vector<RegionSnapshot> const& privatized_data() const
  {
    return privatized_regions_;
  }
  std::vector<RegionSnapshot>& privatized_data()
  {
    return privatized_regions_;
  }

  simgrid::mc::ObjectInformation* object_info() const { return object_info_; }
  void object_info(simgrid::mc::ObjectInformation* info) { object_info_ = info; }

  // Other getters

  RemotePtr<void> start() const { return remote(start_addr_); }
  RemotePtr<void> end() const { return remote((char*)start_addr_ + size_); }
  RemotePtr<void> permanent_address() const { return remote(permanent_addr_); }
  std::size_t size() const { return size_; }
  StorageType storage_type() const { return storage_type_; }
  RegionType region_type() const { return region_type_; }

  bool contain(RemotePtr<void> p) const
  {
    return p >= start() && p < end();
  }
};

RegionSnapshot privatized_region(
    RegionType region_type, void *start_addr, void* permanent_addr,
    std::size_t size, const RegionSnapshot* ref_region);
RegionSnapshot dense_region(
  RegionType type, void *start_addr, void* data_addr, std::size_t size);
simgrid::mc::RegionSnapshot sparse_region(
  RegionType type, void *start_addr, void* data_addr, std::size_t size,
  RegionSnapshot const* ref_region);
simgrid::mc::RegionSnapshot region(
  RegionType type, void *start_addr, void* data_addr, std::size_t size,
  RegionSnapshot const* ref_region);

}
}

typedef class simgrid::mc::RegionSnapshot s_mc_mem_region_t, *mc_mem_region_t;

#endif
