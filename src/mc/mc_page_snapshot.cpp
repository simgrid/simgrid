/* MC interface: definitions that non-MC modules must see, but not the user */

/* Copyright (c) 2014-2015. The SimGrid Team.  All rights reserved.         */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <unistd.h> // pread, pwrite

#include "PageStore.hpp"
#include "mc_mmu.h"
#include "mc_private.h"
#include "mc_snapshot.h"

#include <xbt/mmalloc.h>

extern "C" {

// ***** Region management:

/** @brief Take a per-page snapshot of a region
 *
 *  @param data            The start of the region (must be at the beginning of a page)
 *  @param pag_count       Number of pages of the region
 *  @return                Snapshot page numbers of this new snapshot
 */
size_t* mc_take_page_snapshot_region(mc_process_t process,
  void* data, size_t page_count)
{
  size_t* pagenos = (size_t*) malloc(page_count * sizeof(size_t));

  const bool is_self = MC_process_is_self(process);

  void* temp = NULL;
  if (!is_self)
    temp = malloc(xbt_pagesize);

  for (size_t i=0; i!=page_count; ++i) {

      // Otherwise, we need to store the page the hard way
      // (by reading its content):
      void* page = (char*) data + (i << xbt_pagebits);
      xbt_assert(mc_page_offset(page)==0, "Not at the beginning of a page");
      void* page_data;
      if (is_self) {
        page_data = page;
      } else {
        /* Adding another copy (and a syscall) will probably slow things a lot.
           TODO, optimize this somehow (at least by grouping the syscalls)
           if needed. Either:
            - reduce the number of syscalls;
            - let the application snapshot itself;
            - move the segments in shared memory (this will break `fork` however).
        */
        page_data = temp;
        MC_process_read(process, MC_ADDRESS_SPACE_READ_FLAGS_NONE,
          temp, page, xbt_pagesize, MC_PROCESS_INDEX_DISABLED);
      }
      pagenos[i] = mc_model_checker->page_store().store_page(page_data);

  }

  free(temp);
  return pagenos;
}

void mc_free_page_snapshot_region(size_t* pagenos, size_t page_count)
{
  for (size_t i=0; i!=page_count; ++i) {
    mc_model_checker->page_store().unref_page(pagenos[i]);
  }
}

/** @brief Restore a snapshot of a region
 *
 *  If possible, the restoration will be incremental
 *  (the modified pages will not be touched).
 *
 *  @param start_addr
 *  @param page_count       Number of pages of the region
 *  @param pagenos
 */
void mc_restore_page_snapshot_region(mc_process_t process,
  void* start_addr, size_t page_count, size_t* pagenos)
{
  for (size_t i=0; i!=page_count; ++i) {
    // Otherwise, copy the page:
    void* target_page = mc_page_from_number(start_addr, i);
    const void* source_page = mc_model_checker->page_store().get_page(pagenos[i]);
    MC_process_write(process, source_page, target_page, xbt_pagesize);
  }
}

// ***** High level API

mc_mem_region_t mc_region_new_sparse(mc_region_type_t region_type,
  void *start_addr, void* permanent_addr, size_t size)
{
  mc_process_t process = &mc_model_checker->process();

  mc_mem_region_t region = xbt_new(s_mc_mem_region_t, 1);
  region->region_type = region_type;
  region->storage_type = MC_REGION_STORAGE_TYPE_CHUNKED;
  region->start_addr = start_addr;
  region->permanent_addr = permanent_addr;
  region->size = size;

  xbt_assert((((uintptr_t)start_addr) & (xbt_pagesize-1)) == 0,
    "Not at the beginning of a page");
  xbt_assert((((uintptr_t)permanent_addr) & (xbt_pagesize-1)) == 0,
    "Not at the beginning of a page");
  size_t page_count = mc_page_count(size);

  // Take incremental snapshot:
  region->chunked.page_numbers = mc_take_page_snapshot_region(process,
    permanent_addr, page_count);

  return region;
}

void mc_region_restore_sparse(mc_process_t process, mc_mem_region_t reg)
{
  xbt_assert((((uintptr_t)reg->permanent_addr) & (xbt_pagesize-1)) == 0,
    "Not at the beginning of a page");
  size_t page_count = mc_page_count(reg->size);

  mc_restore_page_snapshot_region(process,
    reg->permanent_addr, page_count, reg->chunked.page_numbers);
}

}
