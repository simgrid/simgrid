/* Copyright (c) 2007-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_SNAPSHOT_H
#define MC_SNAPSHOT_H

#include <sys/types.h> // off_t
#include <stdint.h> // size_t

#include <simgrid_config.h>
#include "../xbt/mmalloc/mmprivate.h"
#include <xbt/asserts.h>
#include <xbt/dynar.h>

#include "mc_forward.h"
#include "ModelChecker.hpp"
#include "PageStore.hpp"
#include "mc_mmalloc.h"
#include "mc/AddressSpace.hpp"
#include "mc_unw.h"

SG_BEGIN_DECL()

// ***** Snapshot region

typedef enum e_mc_region_type_t {
  MC_REGION_TYPE_UNKNOWN = 0,
  MC_REGION_TYPE_HEAP = 1,
  MC_REGION_TYPE_DATA = 2
} mc_region_type_t;

// TODO, use OO instead of this
typedef enum e_mc_region_storage_type_t {
  MC_REGION_STORAGE_TYPE_NONE = 0,
  MC_REGION_STORAGE_TYPE_FLAT = 1,
  MC_REGION_STORAGE_TYPE_CHUNKED = 2,
  MC_REGION_STORAGE_TYPE_PRIVATIZED = 3
} mc_region_storage_type_t;

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
private:
  mc_region_type_t region_type_;
  mc_region_storage_type_t storage_type_;
  mc_object_info_t object_info_;

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
  std::vector<std::unique_ptr<RegionSnapshot>> privatized_regions_;
public:
  RegionSnapshot() :
    region_type_(MC_REGION_TYPE_UNKNOWN),
    storage_type_(MC_REGION_STORAGE_TYPE_NONE),
    object_info_(nullptr),
    start_addr_(nullptr),
    size_(0),
    permanent_addr_(nullptr)
  {}
  RegionSnapshot(mc_region_type_t type, void *start_addr, void* permanent_addr, size_t size) :
    region_type_(type),
    storage_type_(MC_REGION_STORAGE_TYPE_NONE),
    object_info_(nullptr),
    start_addr_(start_addr),
    size_(size),
    permanent_addr_(permanent_addr)
  {}
  ~RegionSnapshot();
  RegionSnapshot(RegionSnapshot const&) = delete;
  RegionSnapshot& operator=(RegionSnapshot const&) = delete;

  // Data

  void clear_data()
  {
    storage_type_ = MC_REGION_STORAGE_TYPE_NONE;
    flat_data_.clear();
    page_numbers_.clear();
    privatized_regions_.clear();
  }
  
  void flat_data(std::vector<char> data)
  {
    storage_type_ = MC_REGION_STORAGE_TYPE_FLAT;
    flat_data_ = std::move(data);
    page_numbers_.clear();
    privatized_regions_.clear();
  }
  std::vector<char> const& flat_data() const { return flat_data_; }

  void page_data(PerPageCopy page_data)
  {
    storage_type_ = MC_REGION_STORAGE_TYPE_CHUNKED;
    flat_data_.clear();
    page_numbers_ = std::move(page_data);
    privatized_regions_.clear();
  }
  PerPageCopy const& page_data() const { return page_numbers_; }

  void privatized_data(std::vector<std::unique_ptr<RegionSnapshot>> data)
  {
    storage_type_ = MC_REGION_STORAGE_TYPE_PRIVATIZED;
    flat_data_.clear();
    page_numbers_.clear();
    privatized_regions_ = std::move(data);
  }
  std::vector<std::unique_ptr<RegionSnapshot>> const& privatized_data() const
  {
    return privatized_regions_;
  }

  mc_object_info_t object_info() const { return object_info_; }
  void object_info(mc_object_info_t info) { object_info_ = info; }

  // Other getters

  remote_ptr<void> start() const { return remote(start_addr_); }
  remote_ptr<void> end() const { return remote((char*)start_addr_ + size_); }
  remote_ptr<void> permanent_address() const { return remote(permanent_addr_); }
  std::size_t size() const { return size_; }
  mc_region_storage_type_t storage_type() const { return storage_type_; }
  mc_region_type_t region_type() const { return region_type_; }

  bool contain(remote_ptr<void> p) const
  {
    return p >= start() && p < end();
  }
};

}
}

typedef class simgrid::mc::RegionSnapshot s_mc_mem_region_t, *mc_mem_region_t;

MC_SHOULD_BE_INTERNAL mc_mem_region_t mc_region_new_sparse(
  mc_region_type_t type, void *start_addr, void* data_addr, size_t size);
XBT_INTERNAL void mc_region_restore_sparse(mc_process_t process, mc_mem_region_t reg);

static inline  __attribute__ ((always_inline))
bool mc_region_contain(mc_mem_region_t region, const void* p)
{
  return region->contain(simgrid::mc::remote(p));
}

static inline __attribute__((always_inline))
void* mc_translate_address_region_chunked(uintptr_t addr, mc_mem_region_t region)
{
  size_t pageno = mc_page_number((void*)region->start().address(), (void*) addr);
  const void* snapshot_page =
    region->page_data().page(pageno);
  return (char*) snapshot_page + mc_page_offset((void*) addr);
}

static inline __attribute__((always_inline))
void* mc_translate_address_region(uintptr_t addr, mc_mem_region_t region, int process_index)
{
  switch (region->storage_type()) {
  case MC_REGION_STORAGE_TYPE_NONE:
  default:
    xbt_die("Storage type not supported");

  case MC_REGION_STORAGE_TYPE_FLAT:
    {
      uintptr_t offset = (uintptr_t) addr - (uintptr_t) region->start().address();
      return (void *) ((uintptr_t) region->flat_data().data() + offset);
    }

  case MC_REGION_STORAGE_TYPE_CHUNKED:
    return mc_translate_address_region_chunked(addr, region);

  case MC_REGION_STORAGE_TYPE_PRIVATIZED:
    {
      xbt_assert(process_index >=0,
        "Missing process index for privatized region");
      xbt_assert((size_t) process_index < region->privatized_data().size(),
        "Out of range process index");
      mc_mem_region_t subregion = region->privatized_data()[process_index].get();
      xbt_assert(subregion, "Missing memory region for process %i", process_index);
      return mc_translate_address_region(addr, subregion, process_index);
    }
  }
}

XBT_INTERNAL mc_mem_region_t mc_get_snapshot_region(
  const void* addr, const s_mc_snapshot_t *snapshot, int process_index);

// ***** MC Snapshot

/** Ignored data
 *
 *  Some parts of the snapshot are ignored by zeroing them out: the real
 *  values is stored here.
 * */
typedef struct s_mc_snapshot_ignored_data {
  void* start;
  size_t size;
  void* data;
} s_mc_snapshot_ignored_data_t, *mc_snapshot_ignored_data_t;

typedef struct s_fd_infos{
  char *filename;
  int number;
  off_t current_position;
  int flags;
}s_fd_infos_t, *fd_infos_t;

}

namespace simgrid {
namespace mc {

class Snapshot : public AddressSpace {
public:
  Snapshot();
  ~Snapshot();
  const void* read_bytes(void* buffer, std::size_t size,
    remote_ptr<void> address, int process_index = ProcessIndexAny,
    ReadMode mode = Normal) const MC_OVERRIDE;
public: // To be private
  mc_process_t process;
  int num_state;
  size_t heap_bytes_used;
  mc_mem_region_t* snapshot_regions;
  size_t snapshot_regions_count;
  xbt_dynar_t enabled_processes;
  int privatization_index;
  size_t *stack_sizes;
  xbt_dynar_t stacks;
  xbt_dynar_t to_ignore;
  uint64_t hash;
  xbt_dynar_t ignored_data;
  int total_fd;
  fd_infos_t *current_fd;
};

}
}

extern "C" {

static inline __attribute__ ((always_inline))
mc_mem_region_t mc_get_region_hinted(void* addr, mc_snapshot_t snapshot, int process_index, mc_mem_region_t region)
{
  if (mc_region_contain(region, addr))
    return region;
  else
    return mc_get_snapshot_region(addr, snapshot, process_index);
}

/** Information about a given stack frame
 *
 */
typedef struct s_mc_stack_frame {
  /** Instruction pointer */
  unw_word_t ip;
  /** Stack pointer */
  unw_word_t sp;
  unw_word_t frame_base;
  dw_frame_t frame;
  char* frame_name;
  unw_cursor_t unw_cursor;
} s_mc_stack_frame_t, *mc_stack_frame_t;

typedef struct s_mc_snapshot_stack{
  xbt_dynar_t local_variables;
  mc_unw_context_t context;
  xbt_dynar_t stack_frames; // mc_stack_frame_t
  int process_index;
}s_mc_snapshot_stack_t, *mc_snapshot_stack_t;

typedef struct s_mc_global_t {
  mc_snapshot_t snapshot;
  int prev_pair;
  char *prev_req;
  int initial_communications_pattern_done;
  int recv_deterministic;
  int send_deterministic;
  char *send_diff;
  char *recv_diff;
}s_mc_global_t, *mc_global_t;

static const void* mc_snapshot_get_heap_end(mc_snapshot_t snapshot);

XBT_INTERNAL mc_snapshot_t MC_take_snapshot(int num_state);
XBT_INTERNAL void MC_restore_snapshot(mc_snapshot_t);

XBT_INTERNAL void mc_restore_page_snapshot_region(
  mc_process_t process,
  void* start_addr, simgrid::mc::PerPageCopy const& pagenos);

MC_SHOULD_BE_INTERNAL const void* MC_region_read_fragmented(
  mc_mem_region_t region, void* target, const void* addr, size_t size);

MC_SHOULD_BE_INTERNAL int MC_snapshot_region_memcmp(
  const void* addr1, mc_mem_region_t region1,
  const void* addr2, mc_mem_region_t region2, size_t size);
XBT_INTERNAL int MC_snapshot_memcmp(
  const void* addr1, mc_snapshot_t snapshot1,
  const void* addr2, mc_snapshot_t snapshot2, int process_index, size_t size);

static inline __attribute__ ((always_inline))
const void* mc_snapshot_get_heap_end(mc_snapshot_t snapshot)
{
  if(snapshot==NULL)
      xbt_die("snapshot is NULL");
  return mc_model_checker->process().get_heap()->breakval;
}

/** @brief Read memory from a snapshot region
 *
 *  @param addr    Process (non-snapshot) address of the data
 *  @param region  Snapshot memory region where the data is located
 *  @param target  Buffer to store the value
 *  @param size    Size of the data to read in bytes
 *  @return Pointer where the data is located (target buffer of original location)
 */
static inline __attribute__((always_inline))
const void* MC_region_read(mc_mem_region_t region, void* target, const void* addr, size_t size)
{
  xbt_assert(region);

  uintptr_t offset = (uintptr_t) addr - (uintptr_t) region->start().address();

  xbt_assert(mc_region_contain(region, addr),
    "Trying to read out of the region boundary.");

  switch (region->storage_type()) {
  case MC_REGION_STORAGE_TYPE_NONE:
  default:
    xbt_die("Storage type not supported");

  case MC_REGION_STORAGE_TYPE_FLAT:
    return (char*) region->flat_data().data() + offset;

  case MC_REGION_STORAGE_TYPE_CHUNKED:
    {
      // Last byte of the region:
      void* end = (char*) addr + size - 1;
      if (mc_same_page(addr, end) ) {
        // The memory is contained in a single page:
        return mc_translate_address_region_chunked((uintptr_t) addr, region);
      } else {
        // The memory spans several pages:
        return MC_region_read_fragmented(region, target, addr, size);
      }
    }

  // We currently do not pass the process_index to this function so we assume
  // that the privatized region has been resolved in the callers:
  case MC_REGION_STORAGE_TYPE_PRIVATIZED:
    xbt_die("Storage type not supported");
  }
}

static inline __attribute__ ((always_inline))
void* MC_region_read_pointer(mc_mem_region_t region, const void* addr)
{
  void* res;
  return *(void**) MC_region_read(region, &res, addr, sizeof(void*));
}

SG_END_DECL()

#endif
