/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_REGION_SNAPSHOT_HPP
#define SIMGRID_MC_REGION_SNAPSHOT_HPP

#include <cstddef>
#include <utility>

#include <xbt/base.h>

#include "PageStore.hpp"
#include "AddressSpace.hpp"

namespace simgrid {
namespace mc {

class PerPageCopy {
  PageStore* store_;
  std::vector<std::size_t> pagenos_;
public:
  PerPageCopy() : store_(nullptr) {}
  PerPageCopy(PerPageCopy const& that)
  {
    store_ = that.store_;
    pagenos_ = that.pagenos_;
    for (std::size_t pageno : pagenos_)
      store_->ref_page(pageno);
  }
  void clear()
  {
    for (std::size_t pageno : pagenos_)
      store_->unref_page(pageno);
    pagenos_.clear();
  }
  ~PerPageCopy() {
    clear();
  }

  PerPageCopy(PerPageCopy&& that)
  {
    store_ = that.store_;
    that.store_ = nullptr;
    pagenos_ = std::move(that.pagenos_);
    that.pagenos_.clear();
  }
  PerPageCopy& operator=(PerPageCopy const& that)
  {
    this->clear();
    store_ = that.store_;
    pagenos_ = that.pagenos_;
    for (std::size_t pageno : pagenos_)
      store_->ref_page(pageno);
    return *this;
  }
  PerPageCopy& operator=(PerPageCopy && that)
  {
    this->clear();
    store_ = that.store_;
    that.store_ = nullptr;
    pagenos_ = std::move(that.pagenos_);
    that.pagenos_.clear();
    return *this;
  }

  std::size_t page_count() const
  {
    return pagenos_.size();
  }

  std::size_t pageno(std::size_t i) const
  {
    return pagenos_[i];
  }

  const void* page(std::size_t i) const
  {
    return store_->get_page(pagenos_[i]);
  }

  PerPageCopy(PageStore& store, AddressSpace& as,
    remote_ptr<void> addr, std::size_t page_count);
};

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

/** @brief Copy/snapshot of a given memory region
 *
 *  Different types of region snapshot storage types exist:
 *  <ul>
 *    <li>flat/dense snapshots are a simple copy of the region;</li>
 *    <li>sparse/per-page snapshots are snaapshots which shared
 *    identical pages.</li>
 *    <li>privatized (SMPI global variable privatisation).
 *  </ul>
 *
 *  This is handled with a variant based approch:
 *
 *    * `storage_type` identified the type of storage;
 *    * an anonymous enum is used to distinguish the relevant types for
 *      each type.
 */
class RegionSnapshot {
  static const RegionType UnknownRegion = RegionType::Unknown;
  static const RegionType HeapRegion = RegionType::Heap;
  static const RegionType DataRegion = RegionType::Data;
  static const StorageType NoData = StorageType::NoData;
  static const StorageType FlatData = StorageType::Flat;
  static const StorageType ChunkedData = StorageType::Chunked;
  static const StorageType PrivatizedData = StorageType::Privatized;
private:
  RegionType region_type_;
  StorageType storage_type_;
  simgrid::mc::ObjectInformation* object_info_;

  /** @brief  Virtual address of the region in the simulated process */
  void *start_addr_;

  /** @brief Size of the data region in bytes */
  size_t size_;

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

  std::vector<char> flat_data_;
  PerPageCopy page_numbers_;
  std::vector<RegionSnapshot> privatized_regions_;
public:
  RegionSnapshot() :
    region_type_(UnknownRegion),
    storage_type_(NoData),
    object_info_(nullptr),
    start_addr_(nullptr),
    size_(0),
    permanent_addr_(nullptr)
  {}
  RegionSnapshot(RegionType type, void *start_addr, void* permanent_addr, size_t size) :
    region_type_(type),
    storage_type_(NoData),
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
    storage_type_ = NoData;
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
    storage_type_ = NoData;
    flat_data_.clear();
    page_numbers_.clear();
    privatized_regions_.clear();
  }
  
  void flat_data(std::vector<char> data)
  {
    storage_type_ = FlatData;
    flat_data_ = std::move(data);
    page_numbers_.clear();
    privatized_regions_.clear();
  }
  std::vector<char> const& flat_data() const { return flat_data_; }

  void page_data(PerPageCopy page_data)
  {
    storage_type_ = ChunkedData;
    flat_data_.clear();
    page_numbers_ = std::move(page_data);
    privatized_regions_.clear();
  }
  PerPageCopy const& page_data() const { return page_numbers_; }

  void privatized_data(std::vector<RegionSnapshot> data)
  {
    storage_type_ = PrivatizedData;
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

  remote_ptr<void> start() const { return remote(start_addr_); }
  remote_ptr<void> end() const { return remote((char*)start_addr_ + size_); }
  remote_ptr<void> permanent_address() const { return remote(permanent_addr_); }
  std::size_t size() const { return size_; }
  StorageType storage_type() const { return storage_type_; }
  RegionType region_type() const { return region_type_; }

  bool contain(remote_ptr<void> p) const
  {
    return p >= start() && p < end();
  }
};

RegionSnapshot privatized_region(
  RegionType type, void *start_addr, void* data_addr, size_t size);
RegionSnapshot dense_region(
  RegionType type, void *start_addr, void* data_addr, size_t size);
simgrid::mc::RegionSnapshot sparse_region(
  RegionType type, void *start_addr, void* data_addr, size_t size);
simgrid::mc::RegionSnapshot region(
  RegionType type, void *start_addr, void* data_addr, size_t size);
  
}
}

typedef class simgrid::mc::RegionSnapshot s_mc_mem_region_t, *mc_mem_region_t;

#endif
