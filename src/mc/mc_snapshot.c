/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdbool.h>

#include "mc_private.h"
#include "mc_mmu.h"
#include "mc_page_store.h"

mc_mem_region_t mc_get_snapshot_region(void* addr, mc_snapshot_t snapshot)
{
  for (size_t i = 0; i != NB_REGIONS; ++i) {
    mc_mem_region_t region = snapshot->regions[i];
    void* start = region->start_addr;
    void* end = (char*) start + region->size;

    if (addr >= start && addr < end) {
      return region;
    }
  }

  return NULL;
}

/** @brief Read memory from a snapshot region broken across fragmented pages
 *
 *  @param addr    Process (non-snapshot) address of the data
 *  @param region  Snapshot memory region where the data is located
 *  @param target  Buffer to store the value
 *  @param size    Size of the data to read in bytes
 *  @return Pointer where the data is located (target buffer of original location)
 */
void* mc_snapshot_read_fragmented(void* addr, mc_mem_region_t region, void* target, size_t size)
{
  void* end = (char*) addr + size - 1;
  size_t page_end = mc_page_number(NULL, end);
  void* dest = target;

  // Read each page:
  while (mc_page_number(NULL, addr) != page_end) {
    void* snapshot_addr = mc_translate_address_region((uintptr_t) addr, region);
    void* next_page = mc_page_from_number(NULL, mc_page_number(NULL, addr) + 1);
    size_t readable = (char*) next_page - (char*) addr;
    memcpy(dest, snapshot_addr, readable);
    addr = (char*) addr + readable;
    dest = (char*) dest + readable;
    size -= readable;
  }

  // Read the end:
  void* snapshot_addr = mc_translate_address_region((uintptr_t)addr, region);
  memcpy(dest, snapshot_addr, size);

  return target;
}

/** @brief Read memory from a snapshot
 *
 *  @param addr     Process (non-snapshot) address of the data
 *  @param snapshot Snapshot (or NULL is no snapshot)
 *  @param target   Buffer to store the value
 *  @param size     Size of the data to read in bytes
 *  @return Pointer where the data is located (target buffer of original location)
 */
void* mc_snapshot_read(void* addr, mc_snapshot_t snapshot, void* target, size_t size)
{
  if (snapshot) {
    mc_mem_region_t region = mc_get_snapshot_region(addr, snapshot);
    return mc_snapshot_read_region(addr, region, target, size);
  } else {
    return addr;
  }
}

/** Compare memory between snapshots (with known regions)
 *
 * @param addr1 Address in the first snapshot
 * @param snapshot2 Region of the address in the first snapshot
 * @param addr2 Address in the second snapshot
 * @param snapshot2 Region of the address in the second snapshot
 * @return same as memcmp
 * */
int mc_snapshot_region_memcmp(
  void* addr1, mc_mem_region_t region1,
  void* addr2, mc_mem_region_t region2, size_t size)
{
  // Using alloca() for large allocations may trigger stack overflow:
  // use malloc if the buffer is too big.

  bool stack_alloc = size < 64;
  void* buffer = stack_alloc ? alloca(2*size) : malloc(2*size);
  void* buffer1 = mc_snapshot_read_region(addr1, region1, buffer, size);
  void* buffer2 = mc_snapshot_read_region(addr2, region2, (char*) buffer + size, size);
  int res;
  if (buffer1 == buffer2) {
    res =  0;
  } else {
    res = memcmp(buffer1, buffer2, size);
  }
  if (!stack_alloc) {
    free(buffer);
  }
  return res;
}

/** Compare memory between snapshots
 *
 * @param addr1 Address in the first snapshot
 * @param snapshot1 First snapshot
 * @param addr2 Address in the second snapshot
 * @param snapshot2 Second snapshot
 * @return same as memcmp
 * */
int mc_snapshot_memcp(
  void* addr1, mc_snapshot_t snapshot1,
  void* addr2, mc_snapshot_t snapshot2, size_t size)
{
  mc_mem_region_t region1 = mc_get_snapshot_region(addr1, snapshot1);
  mc_mem_region_t region2 = mc_get_snapshot_region(addr2, snapshot2);
  return mc_snapshot_region_memcmp(addr1, region1, addr2, region2, size);
}
