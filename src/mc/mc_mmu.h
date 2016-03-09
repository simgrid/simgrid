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


namespace simgrid {
namespace mc {
// TODO, do not depend on xbt_pagesize/xbt_pagebits but our own chunk size
namespace mmu {

static int chunkSize()
{
  return xbt_pagesize;
}

/** @brief How many memory pages are necessary to store size bytes?
 *
 *  @param size Byte size
 *  @return Number of memory pages
 */
static inline __attribute__ ((always_inline))
std::size_t chunkCount(std::size_t size)
{
  size_t page_count = size >> xbt_pagebits;
  if (size & (xbt_pagesize-1))
    page_count ++;
  return page_count;
}

/** @brief Split into chunk number and remaining offset */
static inline __attribute__ ((always_inline))
std::pair<std::size_t, std::uintptr_t> split(std::uintptr_t offset)
{
  return {
    offset >> xbt_pagebits,
    offset & (xbt_pagesize-1)
  };
}

/** Merge chunk number and remaining offset info a global offset */
static inline __attribute__ ((always_inline))
std::uintptr_t join(std::size_t page, std::uintptr_t offset)
{
  return ((std::uintptr_t) page << xbt_pagebits) + offset;
}

static inline __attribute__ ((always_inline))
std::uintptr_t join(std::pair<std::size_t,std::uintptr_t> value)
{
  return join(value.first, value.second);
}

static inline __attribute__ ((always_inline))
bool sameChunk(std::uintptr_t a, std::uintptr_t b)
{
  return (a >> xbt_pagebits) == (b >> xbt_pagebits);
}

}
}
}

#endif
