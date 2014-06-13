/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_MMU_H
#define MC_MMU_H

#include <stdint.h>
#include <stdbool.h>

#include "mc_private.h"

/** @brief How many memory pages are necessary to store size bytes?
 *
 *  @param size Byte size
 *  @return Number of memory pages
 */
static inline size_t mc_page_count(size_t size)
{
  size_t page_count = size >> xbt_pagebits;
  if (size & (xbt_pagesize-1)) {
    page_count ++;
  }
  return page_count;
}

/** @brief Get the virtual memory page number of a given address
 *
 *  @param address Address
 *  @return Virtual memory page number of the given address
 */
static inline size_t mc_page_number(void* base, void* address)
{
  return ((uintptr_t) address - (uintptr_t) base) >> xbt_pagebits;
}

/** @brief Get the offset of an address within a memory page
 *
 *  @param address Address
 *  @return Offset within the memory page
 */
static inline size_t mc_page_offset(void* address)
{
  return ((uintptr_t) address) & (xbt_pagesize-1);
}

/** @brief Get the virtual address of a virtual memory page
 *
 *  @param base Address of the first page
 *  @param page Index of the page
 */
static inline void* mc_page_from_number(void* base, size_t page)
{
  return (void*) ((char*)base + (page << xbt_pagebits));
}

static inline bool mc_same_page(void* a, void* b)
{
  return mc_page_number(NULL, a) == mc_page_number(NULL, b);
}

#endif
