/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstddef>
#include <cstdint>

#include <vector>

#include <xbt/misc.h> // xbt_pagesize and friends
#include <xbt/asserts.h>

#include "src/mc/AddressSpace.hpp"
#include "src/mc/ChunkedData.hpp"
#include "src/mc/PageStore.hpp"

#define SOFT_DIRTY_BIT_NUMBER 55
#define SOFT_DIRTY (((uint64_t)1) << SOFT_DIRTY_BIT_NUMBER)

namespace simgrid {
namespace mc {

/** Take a per-page snapshot of a region
 *
 *  @param data            The start of the region (must be at the beginning of a page)
 *  @param pag_count       Number of pages of the region
 *  @return                Snapshot page numbers of this new snapshot
 */
ChunkedData::ChunkedData(PageStore& store, AddressSpace& as,
    RemotePtr<void> addr, std::size_t page_count,
    const std::size_t* ref_page_numbers, const std::uint64_t* pagemap)
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

      RemotePtr<void> page = remote((void*)
        simgrid::mc::mmu::join(i, addr.address()));
      xbt_assert(simgrid::mc::mmu::split(page.address()).second == 0,
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
