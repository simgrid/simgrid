/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <unistd.h>
#include <string.h> // memcpy, memcp

#include <sys/mman.h>

#include <xbt/base.h>
#include <xbt/log.h>
#include <xbt/sysdep.h>

#include "src/mc/PageStore.hpp"

#include "src/mc/mc_mmu.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_page_snapshot, mc,
                                "Logging specific to mc_page_snapshot");

namespace simgrid {
namespace mc {

/** @brief Compte a hash for the given memory page
 *
 *  The page is used before inserting the page in the page store
 *  in order to find duplicate of this page in the page store.
 *
 *  @param data Memory page
 *  @return hash off the page
 */
static inline  __attribute__ ((always_inline))
PageStore::hash_type mc_hash_page(const void* data)
{
  const std::uint64_t* values = (const uint64_t*) data;
  std::size_t n = xbt_pagesize / sizeof(uint64_t);

  // This djb2:
  std::uint64_t hash = 5381;
  for (std::size_t i = 0; i != n; ++i)
    hash = ((hash << 5) + hash) + values[i];
  return hash;
}

// ***** snapshot_page_manager

PageStore::PageStore(size_t size) :
  memory_(nullptr), capacity_(0), top_index_(0)
{
  // Using mmap in order to be able to expand the region
  // by relocating it somewhere else in the virtual memory
  // space:
  void* memory = ::mmap(nullptr, size << xbt_pagebits, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_POPULATE, -1, 0);
  if (memory == MAP_FAILED)
    xbt_die("Could not mmap initial snapshot pages.");

  this->top_index_ = 0;
  this->capacity_ = size;
  this->memory_ = memory;
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

  // Expand the memory region by moving it into another
  // virtual memory address if necessary:
  void* new_memory = mremap(this->memory_, old_bytesize, new_bytesize, MREMAP_MAYMOVE);
  if (new_memory == MAP_FAILED)
    xbt_die("Could not mremap snapshot pages.");

  this->capacity_ = size;
  this->memory_ = new_memory;
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
  hash_type hash = mc_hash_page(page);
  this->hash_index_[hash].erase(pageno);
}

/** Store a page in memory */
std::size_t PageStore::store_page(void* page)
{
  xbt_assert(top_index_ <= this->capacity_, "top_index is not consistent");

  // First, we check if a page with the same content is already in the page
  // store:
  //  1. compute the hash of the page;
  //  2. find pages with the same hash using `hash_index_`;
  //  3. find a page with the same content.
  hash_type hash = mc_hash_page(page);

  // Try to find a duplicate in set of pages with the same hash:
  page_set_type& page_set = this->hash_index_[hash];
  for (size_t pageno : page_set) {
    const void* snapshot_page = this->get_page(pageno);
    if (memcmp(page, snapshot_page, xbt_pagesize) == 0) {

      // If a page with the same content is already in the page store it is
      // reused and its reference count is incremented.
      page_counts_[pageno]++;
      return pageno;

    }
  }

  // Otherwise, a new page is allocated in the page store and the content
  // of the page is `memcpy()`-ed to this new page.
  std::size_t pageno = alloc_page();
  xbt_assert(this->page_counts_[pageno]==0, "Allocated page is already used");
  void* snapshot_page = (void*) this->get_page(pageno);
  memcpy(snapshot_page, page, xbt_pagesize);
  page_set.insert(pageno);
  page_counts_[pageno]++;
  return pageno;
}

}
}

#ifdef SIMGRID_TEST

#include <cstring>
#include <cstdint>

#include <unistd.h>
#include <sys/mman.h>

#include <memory>

#include "src/mc/PageStore.hpp"

static int value = 0;

static void new_content(void* data, std::size_t size)
{
  ::memset(data, ++value, size);
}

static void* getpage()
{
  return mmap(nullptr, getpagesize(), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
}

XBT_TEST_SUITE("mc_page_store", "Page store");

XBT_TEST_UNIT("base", test_mc_page_store, "Test adding/removing pages in the store")
{
  using simgrid::mc::PageStore;
  
  xbt_test_add("Init");
  std::size_t pagesize = (size_t) getpagesize();
  std::unique_ptr<PageStore> store
    = std::unique_ptr<PageStore>(new simgrid::mc::PageStore(500));
  void* data = getpage();
  xbt_test_assert(store->size()==0, "Bad size");

  xbt_test_add("Store the page once");
  new_content(data, pagesize);
  size_t pageno1 = store->store_page(data);
  xbt_test_assert(store->get_ref(pageno1)==1, "Bad refcount");
  const void* copy = store->get_page(pageno1);
  xbt_test_assert(::memcmp(data, copy, pagesize)==0, "Page data should be the same");
  xbt_test_assert(store->size()==1, "Bad size");

  xbt_test_add("Store the same page again");
  size_t pageno2 = store->store_page(data);
  xbt_test_assert(pageno1==pageno2, "Page should be the same");
  xbt_test_assert(store->get_ref(pageno1)==2, "Bad refcount");
  xbt_test_assert(store->size()==1, "Bad size");

  xbt_test_add("Store a new page");
  new_content(data, pagesize);
  size_t pageno3 = store->store_page(data);
  xbt_test_assert(pageno1 != pageno3, "New page should be different");
  xbt_test_assert(store->size()==2, "Bad size");

  xbt_test_add("Unref pages");
  store->unref_page(pageno1);
  xbt_assert(store->get_ref(pageno1)==1, "Bad refcount");
  xbt_assert(store->size()==2, "Bad size");
  store->unref_page(pageno2);
  xbt_test_assert(store->size()==1, "Bad size");

  xbt_test_add("Reallocate page");
  new_content(data, pagesize);
  size_t pageno4 = store->store_page(data);
  xbt_test_assert(pageno1 == pageno4, "Page was not reused");
  xbt_test_assert(store->get_ref(pageno4)==1, "Bad refcount");
  xbt_test_assert(store->size()==2, "Bad size");
}

#endif /* SIMGRID_TEST */
