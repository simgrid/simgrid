/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdint.h>

#ifdef __cplusplus
#include <vector>

#include <boost/array.hpp>
#include <boost/utility.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#endif

#include <xbt.h>

#include "mc_mmu.h"

#ifndef MC_PAGE_STORE_H
#define MC_PAGE_STORE_H

struct s_mc_pages_store;

#ifdef __cplusplus

/** @brief Storage for snapshot memory pages
 *
 * The first (lower) layer of the per-page snapshot mechanism is a page
 * store: it's responsibility is to store immutable shareable
 * reference-counted memory pages independently of the snapshoting
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
struct s_mc_pages_store {
public: // Types
#ifdef MC_PAGE_STORE_MD4
  typedef boost::array<uint64_t,2> hash_type;
#else
  typedef uint64_t hash_type;
#endif
private: // Types
#ifdef MC_PAGE_STORE_MD4
  // We are using a secure hash to identify a page.
  // We assume there will not be any collision: we need to map a hash
  // to a single page index.
  typedef boost::unordered_map<hash_type, size_t> pages_map_type;
#else
  // We are using a cheap hash to index a page.
  // We should expect collision and we need to associate multiple page indices
  // to the same hash.
  typedef boost::unordered_set<size_t> page_set_type;
  typedef boost::unordered_map<hash_type, page_set_type> pages_map_type;
#endif

private: // Fields:
  /** First page
   *
   *  mc_page_store_get_page expects that this is the first field.
   * */
  void* memory_;
  /** Number of available pages in virtual memory */
  size_t capacity_;
  /** Top of the used pages (index of the next available page) */
  size_t top_index_;
  /** Page reference count */
  std::vector<uint64_t> page_counts_;
  /** Index of available pages before the top */
  std::vector<size_t> free_pages_;
  /** Index from page hash to page index */
  pages_map_type hash_index_;

private: // Methods
  void resize(size_t size);
  size_t alloc_page();
  void remove_page(size_t pageno);

public: // Constructors
  explicit s_mc_pages_store(size_t size);
  ~s_mc_pages_store();

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
  void unref_page(size_t pageno);

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
  size_t store_page(void* page);

  /** @brief Get a page from its page number
   *
   *  @param Number of the memory page in the store
   *  @return Start of the page
   */
  const void* get_page(size_t pageno) const;

public: // Debug/test methods

  /** @brief Get the number of references for a page */
  size_t get_ref(size_t pageno);

  /** @brief Get the number of used pages */
  size_t size();

  /** @brief Get the capacity of the page store
   *
   *  The capacity is expanded by a system call (mremap).
   * */
  size_t capacity();

};

inline __attribute__((always_inline))
void s_mc_pages_store::unref_page(size_t pageno) {
  if ((--this->page_counts_[pageno]) == 0) {
    this->remove_page(pageno);
  }
}

inline __attribute__((always_inline))
void s_mc_pages_store::ref_page(size_t pageno) {
  ++this->page_counts_[pageno];
}

inline __attribute__((always_inline))
const void* s_mc_pages_store::get_page(size_t pageno) const {
  return mc_page_from_number(this->memory_, pageno);
}

inline __attribute__((always_inline))
size_t s_mc_pages_store::get_ref(size_t pageno)  {
  return this->page_counts_[pageno];
}

inline __attribute__((always_inline))
size_t s_mc_pages_store::size() {
  return this->top_index_ - this->free_pages_.size();
}

inline __attribute__((always_inline))
size_t s_mc_pages_store::capacity() {
  return this->capacity_;
}

#endif

SG_BEGIN_DECL()

typedef struct s_mc_pages_store s_mc_pages_store_t, * mc_pages_store_t;
mc_pages_store_t mc_pages_store_new(void);
void mc_pages_store_delete(mc_pages_store_t store);

/**
 */
static inline __attribute__((always_inline))
const void* mc_page_store_get_page(mc_pages_store_t page_store, size_t pageno)
{
  // This is page_store->memory_:
  void* memory = *(void**)page_store;
  return mc_page_from_number(memory, pageno);
}

SG_END_DECL()

#endif
