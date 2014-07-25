/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdbool.h>

#include "mc_private.h"
#include "mc_mmu.h"
#include "mc_page_store.h"

/** @brief Find the snapshoted region from a pointer
 *
 *  @param addr     Pointer
 *  @param snapshot Snapshot
 *  @param Snapshot region in the snapshot this pointer belongs to
 *         (or NULL if it does not belong to any snapshot region)
 * */
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
  // Last byte of the memory area:
  void* end = (char*) addr + size - 1;

  // Page of the last byte of the memory area:
  size_t page_end = mc_page_number(NULL, end);

  void* dest = target;

  if (dest==NULL) {
    xbt_die("Missing destination buffer for fragmented memory access");
  }

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
 *  @return Pointer where the data is located (target buffer or original location)
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
  void* buffer1a = (region1==NULL || region1->data) ? NULL : stack_alloc ? alloca(size) : malloc(size);
  void* buffer2a = (region2==NULL || region2->data) ? NULL : stack_alloc ? alloca(size) : malloc(size);
  void* buffer1 = mc_snapshot_read_region(addr1, region1, buffer1a, size);
  void* buffer2 = mc_snapshot_read_region(addr2, region2, buffer2a, size);
  int res;
  if (buffer1 == buffer2) {
    res = 0;
  } else {
    res = memcmp(buffer1, buffer2, size);
  }
  if (!stack_alloc) {
    free(buffer1a);
    free(buffer2a);
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
int mc_snapshot_memcmp(
  void* addr1, mc_snapshot_t snapshot1,
  void* addr2, mc_snapshot_t snapshot2, size_t size)
{
  mc_mem_region_t region1 = mc_get_snapshot_region(addr1, snapshot1);
  mc_mem_region_t region2 = mc_get_snapshot_region(addr2, snapshot2);
  return mc_snapshot_region_memcmp(addr1, region1, addr2, region2, size);
}

#ifdef SIMGRID_TEST

#include <string.h>
#include <stdlib.h>

#include <sys/mman.h>

#include "mc/mc_private.h"

XBT_TEST_SUITE("mc_snapshot", "Snapshots");

static inline void init_memory(void* mem, size_t size)
{
  char* dest = (char*) mem;
  for (int i=0; i!=size; ++i) {
    dest[i] = rand() & 255;
  }
}

static void test_snapshot(bool sparse_checkpoint);

XBT_TEST_UNIT("page_snapshots", test_per_snpashots, "Test per-page snapshots")
{
  test_snapshot(1);
}


XBT_TEST_UNIT("flat_snapshot", test_flat_snapshots, "Test flat snapshots")
{
  test_snapshot(0);
}


static void test_snapshot(bool sparse_checkpoint) {

  xbt_test_add("Initialisation");
  _sg_mc_soft_dirty = 0;
  _sg_mc_sparse_checkpoint = sparse_checkpoint;
  xbt_assert(xbt_pagesize == getpagesize());
  xbt_assert(1 << xbt_pagebits == xbt_pagesize);
  mc_model_checker = xbt_new0(s_mc_model_checker_t, 1);
  mc_model_checker->pages = mc_pages_store_new();

  for(int n=1; n!=256; ++n) {

    // Store region page(s):
    size_t byte_size = n * xbt_pagesize;
    void* source = mmap(NULL, byte_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    xbt_assert(source!=MAP_FAILED, "Could not allocate source memory");

    // Init memory and take snapshots:
    init_memory(source, byte_size);
    mc_mem_region_t region0 = mc_region_new_sparse(0, source, byte_size, NULL);
    for(int i=0; i<n; i+=2) {
      init_memory((char*) source + i*xbt_pagesize, xbt_pagesize);
    }
    mc_mem_region_t region = mc_region_new_sparse(0, source, byte_size, NULL);

    void* destination = mmap(NULL, byte_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    xbt_assert(source!=MAP_FAILED, "Could not allocate destination memory");

    xbt_test_add("Reading whole region data for %i page(s)", n);
    void* read = mc_snapshot_read_region(source, region, destination, byte_size);
    xbt_test_assert(!memcmp(source, read, byte_size), "Mismatch in mc_snapshot_read_region()");

    xbt_test_add("Reading parts of region data for %i page(s)", n);
    for(int j=0; j!=100; ++j) {
      size_t offset = rand() % byte_size;
      size_t size = rand() % (byte_size - offset);
      void* read = mc_snapshot_read_region((char*) source+offset, region, destination, size);
      xbt_test_assert(!memcmp((char*) source+offset, read, size),
        "Mismatch in mc_snapshot_read_region()");
    }

    xbt_test_add("Compare whole region data for %i page(s)", n);
    xbt_test_assert(!mc_snapshot_region_memcmp(source, NULL, source, region, byte_size),
      "Mismatch in mc_snapshot_region_memcmp() for the whole region");
    xbt_test_assert(mc_snapshot_region_memcmp(source, region0, source, region, byte_size),
      "Unexpected match in mc_snapshot_region_memcmp() with previous snapshot");

    xbt_test_add("Compare parts of region data for %i page(s) with current value", n);
    for(int j=0; j!=100; ++j) {
      size_t offset = rand() % byte_size;
      size_t size = rand() % (byte_size - offset);
      xbt_test_assert(!mc_snapshot_region_memcmp((char*) source+offset, NULL, (char*) source+offset, region, size),
        "Mismatch in mc_snapshot_region_memcmp()");
    }

    xbt_test_add("Compare parts of region data for %i page(s) with itself", n);
    for(int j=0; j!=100; ++j) {
      size_t offset = rand() % byte_size;
      size_t size = rand() % (byte_size - offset);
      xbt_test_assert(!mc_snapshot_region_memcmp((char*) source+offset, region, (char*) source+offset, region, size),
        "Mismatch in mc_snapshot_region_memcmp()");
    }

    if (n==1) {
      xbt_test_add("Read pointer for %i page(s)", n);
      memcpy(source, &mc_model_checker, sizeof(void*));
      mc_mem_region_t region2 = mc_region_new_sparse(0, source, byte_size, NULL);
      xbt_test_assert(mc_snapshot_read_pointer_region(source, region2) == mc_model_checker,
        "Mismtach in mc_snapshot_read_pointer_region()");
      MC_region_destroy(region2);
    }

    MC_region_destroy(region);
    MC_region_destroy(region0);
    munmap(destination, byte_size);
    munmap(source, byte_size);
  }

  mc_pages_store_delete(mc_model_checker->pages);
  xbt_free(mc_model_checker);
  mc_model_checker = NULL;
}

#endif /* SIMGRID_TEST */

