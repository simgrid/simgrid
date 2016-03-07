/* Copyright (c) 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_MMU_H
#define SIMGRID_MC_MMU_H

#include <cstdint>
#include <cstddef>

#include <xbt/asserts.h>
#include <xbt/base.h> // xbt_pagesize...
#include <xbt/misc.h>

#include <simgrid_config.h>

SG_BEGIN_DECL()

/** @brief How many memory pages are necessary to store size bytes?
 *
 *  @param size Byte size
 *  @return Number of memory pages
 */
static inline __attribute__ ((always_inline))
size_t mc_page_count(size_t size)
{
  size_t page_count = size >> xbt_pagebits;
  if (size & (xbt_pagesize-1))
    page_count ++;
  return page_count;
}

/** @brief Get the virtual memory page number of a given address
 *
 *  @param address Address
 *  @return Virtual memory page number of the given address
 */
static inline __attribute__ ((always_inline))
size_t mc_page_number(const void* base, const void* address)
{
  xbt_assert(address>=base, "The address is not in the range");
  return ((std::uintptr_t) address - (std::uintptr_t) base) >> xbt_pagebits;
}

/** @brief Get the offset of an address within a memory page
 *
 *  @param address Address
 *  @return Offset within the memory page
 */
static inline __attribute__ ((always_inline))
size_t mc_page_offset(const void* address)
{
  return ((std::uintptr_t) address) & (xbt_pagesize-1);
}

/** @brief Get the virtual address of a virtual memory page
 *
 *  @param base Address of the first page
 *  @param page Index of the page
 */
static inline __attribute__ ((always_inline))
void* mc_page_from_number(const void* base, size_t page)
{
  return (void*) ((char*)base + (page << xbt_pagebits));
}

static inline __attribute__ ((always_inline))
bool mc_same_page(const void* a, const void* b)
{
  return ((std::uintptr_t) a >> xbt_pagebits)
    == ((std::uintptr_t) b >> xbt_pagebits);
}

SG_END_DECL()

#endif
