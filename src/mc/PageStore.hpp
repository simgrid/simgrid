/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_PAGESTORE_HPP
#define SIMGRID_MC_PAGESTORE_HPP

#include <cstdint>
#include <vector>

#include <unordered_map>
#include <unordered_set>

#include <xbt/base.h>

#include "src/mc/mc_mmu.h"
#include "src/mc/mc_forward.hpp"

namespace simgrid {
namespace mc {

/** @brief Storage for snapshot memory pages
 *
 * The first (lower) layer of the per-page snapshot mechanism is a page
 * store: its responsibility is to store immutable shareable
 * reference-counted memory pages independently of the snapshotting
 * logic. Snapshot management and representation, soft-dirty tracking is
 * handled to an higher layer. READMORE
 *
 * Data structure:
 *
 *  * A pointer (`memory_`) to a (currently anonymous) `mmap()`ed memory
 *    region holding the memory pages (the address of the first page).
 *
 *    We want to keep this memory region aligned on the memory pages (so
 *    that we might be able to create non-linear memory mappings on those
 *    pages in the future) and be able to expand it without coyping the
 *    data (there will be a lot of pages here): we will be able to
 *    efficiently expand the memory mapping using `mremap()`, moving it
 *    to another virtual address if necessary.
 *
 *    Because we will move this memory mapping on the virtual address
 *    space, only the index of the page will be stored in the snapshots
 *    and the page will always be looked up by going through `memory`:
 *
 *         void* page = (char*) page_store->memory + page_index << pagebits;
 *
 *  * The number of pages mapped in virtual memory (`capacity_`). Once all
 *    those pages are used, we need to expand the page store with
 *    `mremap()`.
 *
 *  * A reference count for each memory page `page_counts_`. Each time a
 *    snapshot references a page, the counter is incremented. If a
 *    snapshot is freed, the reference count is decremented. When the
 *    reference count, of a page reaches 0 it is added to a list of available
 *    pages (`free_pages_`).
 *
 *  * A list of free pages `free_pages_` which can be reused. This avoids having
 *    to scan the reference count list to find a free page.
 *
 *  * When we are expanding the memory map we do not want to add thousand of page
 *    to the `free_pages_` list and remove them just afterwards. The `top_index_`
 *    field is an index after which all pages are free and are not in the `free_pages_`
 *    list.
 *
 *  * When we are adding a page, we need to check if a page with the same
 *    content is already in the page store in order to reuse it. For this
 *    reason, we maintain an index (`hash_index_`) mapping the hash of a
 *    page to the list of page indices with this hash.
 *    We use a fast (non cryptographic) hash so there may be conflicts:
 *    we must be able to store multiple indices for the same hash.
 *
 */
class PageStore {
public: // Types
  typedef std::uint64_t hash_type;
private: // Types
  // We are using a cheap hash to index a page.
  // We should expect collision and we need to associate multiple page indices
  // to the same hash.
  typedef std::unordered_set<std::size_t> page_set_type;
  typedef std::unordered_map<hash_type, page_set_type> pages_map_type;

private: // Fields:
  /** First page */
  void* memory_;
  /** Number of available pages in virtual memory */
  std::size_t capacity_;
  /** Top of the used pages (index of the next available page) */
  std::size_t top_index_;
  /** Page reference count */
  std::vector<std::uint64_t> page_counts_;
  /** Index of available pages before the top */
  std::vector<std::size_t> free_pages_;
  /** Index from page hash to page index */
  pages_map_type hash_index_;

private: // Methods
  void resize(std::size_t size);
  std::size_t alloc_page();
  void remove_page(std::size_t pageno);

public: // Constructors
  PageStore(PageStore const&) = delete;
  PageStore& operator=(PageStore const&) = delete;
  explicit PageStore(std::size_t size);
  ~PageStore();

public: // Methods

  /** @brief Decrement the reference count for a given page
   *
   * Decrement the reference count of this page. Used when a snapshot is
   * destroyed.
   *
   * If the reference count reaches zero, the page is recycled:
   * it is added to the `free_pages_` list and removed from the `hash_index_`.
   *
   * */
  void unref_page(std::size_t pageno);

  /** @brief Increment the refcount for a given page
   *
   * This method used to increase a reference count of a page if we know
   * that the content of a page is the same as a page already in the page
   * store.
   *
   * This will be the case if a page if soft clean: we know that is has not
   * changed since the previous cnapshot/restoration and we can avoid
   * hashing the page, comparing byte-per-byte to candidates.
   * */
  void ref_page(size_t pageno);

  /** @brief Store a page in the page store */
  std::size_t store_page(void* page);

  /** @brief Get a page from its page number
   *
   *  @param Number of the memory page in the store
   *  @return Start of the page
   */
  const void* get_page(std::size_t pageno) const;

public: // Debug/test methods

  /** @brief Get the number of references for a page */
  std::size_t get_ref(std::size_t pageno);

  /** @brief Get the number of used pages */
  std::size_t size();

  /** @brief Get the capacity of the page store
   *
   *  The capacity is expanded by a system call (mremap).
   * */
  std::size_t capacity();

};

inline __attribute__((always_inline))
void PageStore::unref_page(std::size_t pageno) {
  if ((--this->page_counts_[pageno]) == 0)
    this->remove_page(pageno);
}

inline __attribute__((always_inline))
void PageStore::ref_page(size_t pageno)
{
  ++this->page_counts_[pageno];
}

inline __attribute__((always_inline))
const void* PageStore::get_page(std::size_t pageno) const
{
  return (void*) simgrid::mc::mmu::join(pageno, (std::uintptr_t) this->memory_);
}

inline __attribute__((always_inline))
std::size_t PageStore::get_ref(std::size_t pageno)
{
  return this->page_counts_[pageno];
}

inline __attribute__((always_inline))
std::size_t PageStore::size() {
  return this->top_index_ - this->free_pages_.size();
}

inline __attribute__((always_inline))
std::size_t PageStore::capacity()
{
  return this->capacity_;
}

}
}

#endif
