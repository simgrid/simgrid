/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdint.h>

#include <vector>

#include <boost/utility.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

#include <xbt.h>

#include "mc_private.h"
#include "mc_mmu.h"

#ifndef MC_PAGE_SNAPSHOT_H
#define MC_PAGE_SNAPSHOT_H

/** @brief Manager for snapshot pages
 *
 *  Page management: the free pages are stored as a simple linked-list.
 *  The number of the first page if stored in `free_pages`.
 *  Each free page store the number of the next free page.
 */
struct s_mc_pages_store {
private: // Types
  typedef uint64_t hash_type;
  typedef boost ::unordered_set<size_t> page_set_type;
  typedef boost::unordered_map<hash_type, page_set_type> pages_map_type;

private: // Fields:
  /** First page */
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

  /** @brief Decrement the refcount for a given page */
  void unref_page(size_t pageno) {
    if ((--this->page_counts_[pageno]) == 0) {
      this->remove_page(pageno);
    }
  }

  /** @brief Increment the refcount for a given page */
  void ref_page(size_t pageno) {
    ++this->page_counts_[pageno];
  }

  /** @brief Store a page
   *
   *  Either allocate a new page in the store or reuse
   *  a shared page if is is already in the page store.
   */
  size_t store_page(void* page);

  /** @brief Get a page from its page number
   *
   *  @param Number of the memory page in the store
   *  @return Start of the page
   */
  const void* get_page(size_t pageno) const {
    return mc_page_from_number(this->memory_, pageno);
  }

public: // Debug/test methods

  /** @brief Get the number of references for a page */
  size_t get_ref(size_t pageno) {
    return this->page_counts_[pageno];
  }

  /** @brief Get the number of used pages */
  size_t size() {
    return this->top_index_ - this->free_pages_.size();
  }

  /** @brief Get the capacity of the page store
   *
   *  The capacity is expanded by a system call (mremap).
   * */
  size_t capacity() {
    return this->capacity_;
  }

};

#endif

