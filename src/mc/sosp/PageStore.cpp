/* Copyright (c) 2015-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <sys/mman.h>
#ifdef __FreeBSD__
#define MAP_POPULATE MAP_PREFAULT_READ
#endif

#include "src/internal_config.h"
#include "xbt/log.h"
#include "xbt/sysdep.h"

#ifdef SG_HAVE_CPP14
#include "src/include/xxhash.hpp"
#endif
#include "src/mc/mc_mmu.hpp"
#include "src/mc/sosp/PageStore.hpp"

#include <cstring> // memcpy, memcmp
#include <unistd.h>

namespace simgrid {
namespace mc {

/** @brief Compute a hash for the given memory page
 *
 *  The page is used before inserting the page in the page store in order to find duplicate of this page in the page
 *  store.
 *
 *  @param data Memory page
 *  @return hash off the page
 */
static XBT_ALWAYS_INLINE PageStore::hash_type mc_hash_page(const void* data)
{
#ifdef SG_HAVE_CPP14
  return xxh::xxhash<64>(data, xbt_pagesize);
#else
  const std::uint64_t* values = (const uint64_t*)data;
  std::size_t n               = xbt_pagesize / sizeof(uint64_t);

  // This djb2:
  std::uint64_t hash = 5381;
  for (std::size_t i = 0; i != n; ++i)
    hash = ((hash << 5) + hash) + values[i];
  return hash;
#endif
}

// ***** snapshot_page_manager

PageStore::PageStore(std::size_t size) : memory_(nullptr), capacity_(size), top_index_(0)
{
  // Using mmap in order to be able to expand the region by relocating it somewhere else in the virtual memory space:
  void* memory =
      ::mmap(nullptr, size << xbt_pagebits, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
  if (memory == MAP_FAILED)
    xbt_die("Could not mmap initial snapshot pages.");

  this->top_index_ = 0;
  this->memory_    = memory;
  this->page_counts_.resize(size);
}

PageStore::~PageStore()
{
  ::munmap(this->memory_, this->capacity_ << xbt_pagebits);
}

void PageStore::resize(std::size_t size)
{
  size_t old_bytesize = this->capacity_ << xbt_pagebits;
  size_t new_bytesize = size << xbt_pagebits;
  void* new_memory;

  // Expand the memory region by moving it into another
  // virtual memory address if necessary:
#if HAVE_MREMAP
  new_memory = mremap(this->memory_, old_bytesize, new_bytesize, MREMAP_MAYMOVE);
  if (new_memory == MAP_FAILED)
    xbt_die("Could not mremap snapshot pages.");
#else
  if (new_bytesize > old_bytesize) {
    // Grow: first try to add new space after current map
    new_memory = mmap((char*)this->memory_ + old_bytesize, new_bytesize - old_bytesize, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
    if (new_memory == MAP_FAILED)
      xbt_die("Could not mremap snapshot pages.");
    // Check if expanding worked
    if (new_memory != (char*)this->memory_ + old_bytesize) {
      // New memory segment could not be put at the end of this->memory_,
      // so cancel this one and try to relocate everything and copy data
      munmap(new_memory, new_bytesize - old_bytesize);
      new_memory =
          mmap(nullptr, new_bytesize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
      if (new_memory == MAP_FAILED)
        xbt_die("Could not mremap snapshot pages.");
      memcpy(new_memory, this->memory_, old_bytesize);
      munmap(this->memory_, old_bytesize);
    }
  } else {
    // We don't have functions to shrink a mapping, so leave memory as
    // it is for now
    new_memory = this->memory_;
  }
#endif

  this->capacity_ = size;
  this->memory_   = new_memory;
  this->page_counts_.resize(size, 0);
}

/** Allocate a free page
 *
 *  @return index of the free page
 */
std::size_t PageStore::alloc_page()
{
  if (this->free_pages_.empty()) {
    // Expand the region:
    if (this->top_index_ == this->capacity_)
      // All the pages are allocated, we need add more pages:
      this->resize(2 * this->capacity_);

    // Use a page from the top:
    return this->top_index_++;
  } else {
    // Use a page from free_pages_ (inside of the region):
    size_t res = this->free_pages_[this->free_pages_.size() - 1];
    this->free_pages_.pop_back();
    return res;
  }
}

void PageStore::remove_page(std::size_t pageno)
{
  this->free_pages_.push_back(pageno);
  const void* page = this->get_page(pageno);
  hash_type hash   = mc_hash_page(page);
  this->hash_index_[hash].erase(pageno);
}

/** Store a page in memory */
std::size_t PageStore::store_page(void* page)
{
  xbt_assert(top_index_ <= this->capacity_, "top_index is not consistent");

  // First, we check if a page with the same content is already in the page store:
  //  1. compute the hash of the page
  //  2. find pages with the same hash using `hash_index_`
  //  3. find a page with the same content
  hash_type hash = mc_hash_page(page);

  // Try to find a duplicate in set of pages with the same hash:
  page_set_type& page_set = this->hash_index_[hash];
  for (size_t const& pageno : page_set) {
    const void* snapshot_page = this->get_page(pageno);
    if (memcmp(page, snapshot_page, xbt_pagesize) == 0) {
      // If a page with the same content is already in the page store it's reused and its refcount is incremented.
      page_counts_[pageno]++;
      return pageno;
    }
  }

  // Otherwise, a new page is allocated in the page store and the content of the page is `memcpy()`-ed to this new page.
  std::size_t pageno = alloc_page();
  xbt_assert(this->page_counts_[pageno] == 0, "Allocated page is already used");
  void* snapshot_page = (void*)this->get_page(pageno);
  memcpy(snapshot_page, page, xbt_pagesize);
  page_set.insert(pageno);
  page_counts_[pageno]++;
  return pageno;
}

} // namespace mc
} // namespace simgrid
