/* MC interface: definitions that non-MC modules must see, but not the user */

/* Copyright (c) 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <unistd.h> // pread, pwrite

#include "PageStore.hpp"
#include "mc_mmu.h"
#include "mc_private.h"
#include "mc_snapshot.h"

#include <xbt/mmalloc.h>

#define SOFT_DIRTY_BIT_NUMBER 55
#define SOFT_DIRTY (((uint64_t)1) << SOFT_DIRTY_BIT_NUMBER)

using simgrid::mc::remote;

namespace simgrid {
namespace mc {

/** @brief Take a per-page snapshot of a region
 *
 *  @param data            The start of the region (must be at the beginning of a page)
 *  @param pag_count       Number of pages of the region
 *  @return                Snapshot page numbers of this new snapshot
 */
PerPageCopy::PerPageCopy(PageStore& store, AddressSpace& as,
    remote_ptr<void> addr, std::size_t page_count,
    const size_t* ref_page_numbers, const std::uint64_t* pagemap)
{
  store_ = &store;
  this->pagenos_.resize(page_count);
  std::vector<char> buffer(xbt_pagesize);

  for (size_t i = 0; i != page_count; ++i) {

    // We don't have to compare soft-clean pages:
    if (ref_page_numbers && pagemap && !(pagemap[i] & SOFT_DIRTY)) {
      pagenos_[i] = ref_page_numbers[i];
      store_->ref_page(ref_page_numbers[i]);
      continue;
    }

      remote_ptr<void> page = remote(addr.address() + (i << xbt_pagebits));
      xbt_assert(mc_page_offset((void*)page.address())==0,
        "Not at the beginning of a page");

        /* Adding another copy (and a syscall) will probably slow things a lot.
           TODO, optimize this somehow (at least by grouping the syscalls)
           if needed. Either:
            - reduce the number of syscalls;
            - let the application snapshot itself;
            - move the segments in shared memory (this will break `fork` however).
        */

        as.read_bytes(
          buffer.data(), xbt_pagesize, page,
          simgrid::mc::ProcessIndexDisabled);

      pagenos_[i] = store_->store_page(buffer.data());

  }
}

}
}

extern "C" {

/** @brief Restore a snapshot of a region
 *
 *  If possible, the restoration will be incremental
 *  (the modified pages will not be touched).
 *
 *  @param start_addr
 *  @param page_count       Number of pages of the region
 *  @param pagenos
 */
void mc_restore_page_snapshot_region(simgrid::mc::Process* process,
  void* start_addr, simgrid::mc::PerPageCopy const& pages_copy)
{
  for (size_t i = 0; i != pages_copy.page_count(); ++i) {
    // Otherwise, copy the page:
    void* target_page = mc_page_from_number(start_addr, i);
    const void* source_page = pages_copy.page(i);
    process->write_bytes(source_page, xbt_pagesize, remote(target_page));
  }
}

// ***** High level API

void mc_region_restore_sparse(simgrid::mc::Process* process, mc_mem_region_t reg)
{
  xbt_assert(((reg->permanent_address().address()) & (xbt_pagesize-1)) == 0,
    "Not at the beginning of a page");
  xbt_assert(mc_page_count(reg->size()) == reg->page_data().page_count());
  mc_restore_page_snapshot_region(process,
    (void*) reg->permanent_address().address(), reg->page_data());
}

}
