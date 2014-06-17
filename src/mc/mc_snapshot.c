/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc_private.h"
#include "mc_mmu.h"

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

void* mc_translate_address_region(uintptr_t addr, mc_mem_region_t region)
{
  if (!region) {
    return (void *) addr;
  }

  // Flat snapshot:
  else if (region->data) {
    uintptr_t offset = addr - (uintptr_t) region->start_addr;
    return (void *) ((uintptr_t) region->data + offset);
  }

  // Per-page snapshot:
  else if (region->page_numbers) {
    size_t pageno = mc_page_number(region->data, (void*) addr);
    return (char*) region->page_numbers[pageno] + mc_page_offset((void*) addr);
  }

  else {
    xbt_die("No data for this memory region");
  }
}

void* mc_translate_address(uintptr_t addr, mc_snapshot_t snapshot)
{

  // If not in a process state/clone:
  if (!snapshot) {
    return (uintptr_t *) addr;
  }

  mc_mem_region_t region = mc_get_snapshot_region((void*) addr, snapshot);
  return mc_translate_address_region(addr, region);

}

uintptr_t mc_untranslate_address(void *addr, mc_snapshot_t snapshot)
{
  if (!snapshot) {
    return (uintptr_t) addr;
  }

  for (size_t i = 0; i != NB_REGIONS; ++i) {
    mc_mem_region_t region = snapshot->regions[i];
    if (region->page_numbers) {
      xbt_die("Does not work for per-page snapshot.");
    }
    if (addr >= region->data
        && addr <= (void *) (((char *) region->data) + region->size)) {
      size_t offset = (size_t) ((char *) addr - (char *) region->data);
      return ((uintptr_t) region->start_addr) + offset;
    }
  }

  return (uintptr_t) addr;
}

/** @brief Read memory from a snapshot region broken across fragmented pages
 *
 *  @param addr    Process (non-snapshot) address of the data
 *  @param region  Snapshot memory region where the data is located
 *  @param target  Buffer to store the value
 *  @param size    Size of the data to read in bytes
 *  @return Pointer where the data is located (target buffer of original location)
 */
static void* mc_snapshot_read_fragmented(void* addr, mc_mem_region_t region, void* target, size_t size)
{
  void* end = (char*) addr + size - 1;
  size_t page_end = mc_page_number(NULL, end);

  // Read each page:
  while (mc_page_number(NULL, addr) != page_end) {
    void* snapshot_addr = mc_translate_address_region((uintptr_t) addr, region);
    void* next_page = mc_page_from_number(NULL, mc_page_number(NULL, addr) + 1);
    size_t readable = (char*) next_page - (char*) addr;
    memcpy(target, snapshot_addr, readable);
    target = (char*) target + readable;
    size -= readable;
  }

  // Read the end:
  void* snapshot_addr = mc_translate_address_region((uintptr_t)addr, region);
  memcpy(target, snapshot_addr, size);

  return target;
}

/** @brief Read memory from a snapshot region
 *
 *  @param addr    Process (non-snapshot) address of the data
 *  @param region  Snapshot memory region where the data is located
 *  @param target  Buffer to store the value
 *  @param size    Size of the data to read in bytes
 *  @return Pointer where the data is located (target buffer of original location)
 */
void* mc_snapshot_read_region(void* addr, mc_mem_region_t region, void* target, size_t size)
{
  uintptr_t offset = (uintptr_t) addr - (uintptr_t) region->start_addr;

  if (addr < region->start_addr || (char*) addr+size >= (char*)region->start_addr+region->size) {
    xbt_die("Trying to read out of the region boundary.");
  }

  // Linear memory region:
  if (region->data) {
    return (void*) ((uintptr_t) region->data + offset);
  }

  // Fragmented memory region:
  else if (region->page_numbers) {
    void* end = (char*) addr + size - 1;
    if( mc_same_page(addr, end) ) {
      // The memory is contained in a single page:
      return mc_translate_address_region((uintptr_t) addr, region);
    } else {
      // The memory spans several pages:
      return mc_snapshot_read_fragmented(addr, region, target, size);
    }
  }

  else {
    xbt_die("No data available for this region");
  }
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
