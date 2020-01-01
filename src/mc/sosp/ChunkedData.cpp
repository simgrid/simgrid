/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/AddressSpace.hpp"
#include "src/mc/sosp/ChunkedData.hpp"

namespace simgrid {
namespace mc {

/** Take a per-page snapshot of a region
 *
 *  @param addr            The start of the region (must be at the beginning of a page)
 *  @param page_count      Number of pages of the region
 *  @return                Snapshot page numbers of this new snapshot
 */
ChunkedData::ChunkedData(PageStore& store, const AddressSpace& as, RemotePtr<void> addr, std::size_t page_count)
    : store_(&store)
{
  this->pagenos_.resize(page_count);
  std::vector<char> buffer(xbt_pagesize);

  for (size_t i = 0; i != page_count; ++i) {

    RemotePtr<void> page = remote((void*)simgrid::mc::mmu::join(i, addr.address()));
    xbt_assert(simgrid::mc::mmu::split(page.address()).second == 0, "Not at the beginning of a page");

    /* Adding another copy (and a syscall) will probably slow things a lot.
       TODO, optimize this somehow (at least by grouping the syscalls)
       if needed. Either:
       - reduce the number of syscalls
       - let the application snapshot itself
       - move the segments in shared memory (this will break `fork` however)
    */

    as.read_bytes(buffer.data(), xbt_pagesize, page);

    pagenos_[i] = store_->store_page(buffer.data());
  }
}

} // namespace mc
} // namespace simgrid
