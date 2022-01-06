/* Copyright (c) 2014-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_CHUNKED_DATA_HPP
#define SIMGRID_MC_CHUNKED_DATA_HPP

#include <vector>

#include "src/mc/mc_forward.hpp"
#include "src/mc/remote/RemotePtr.hpp"
#include "src/mc/sosp/PageStore.hpp"

namespace simgrid {
namespace mc {

/** A byte-string represented as a sequence of chunks from a PageStore
 *
 *  In order to save memory when taking memory snapshots, a given byte-string
 *  is split in fixed-size chunks. Identical chunks (either from the same
 *  snapshot or more probably from different snapshots) share the same memory
 *  storage.
 *
 *  Thus a chunked is represented as a sequence of indices of each chunk.
 */
class ChunkedData {
  /** This is where we store the chunks */
  PageStore* store_ = nullptr;
  /** Indices of the chunks in the `PageStore` */
  std::vector<std::size_t> pagenos_;

public:
  ChunkedData() = default;
  void clear()
  {
    for (std::size_t const& pageno : pagenos_)
      store_->unref_page(pageno);
    pagenos_.clear();
  }
  ~ChunkedData() { clear(); }

  // Copy and move
  ChunkedData(ChunkedData const& that) : store_(that.store_), pagenos_(that.pagenos_)
  {
    for (std::size_t const& pageno : pagenos_)
      store_->ref_page(pageno);
  }
  ChunkedData(ChunkedData&& that) noexcept : pagenos_(std::move(that.pagenos_))
  {
    std::swap(store_, that.store_);
    that.pagenos_.clear();
  }
  ChunkedData& operator=(ChunkedData const& that)
  {
    if (this != &that) {
      this->clear();
      store_   = that.store_;
      pagenos_ = that.pagenos_;
      for (std::size_t const& pageno : pagenos_)
        store_->ref_page(pageno);
    }
    return *this;
  }
  ChunkedData& operator=(ChunkedData&& that) noexcept
  {
    if (this != &that) {
      this->clear();
      store_      = that.store_;
      that.store_ = nullptr;
      pagenos_    = std::move(that.pagenos_);
      that.pagenos_.clear();
    }
    return *this;
  }

  /** How many pages are used */
  std::size_t page_count() const { return pagenos_.size(); }

  /** Get a chunk index */
  std::size_t pageno(std::size_t i) const { return pagenos_[i]; }

  /** Get a view of the chunk indices */
  const std::size_t* pagenos() const { return pagenos_.data(); }

  /** Get a pointer to a chunk */
  void* page(std::size_t i) const { return store_->get_page(pagenos_[i]); }

  ChunkedData(PageStore& store, const AddressSpace& as, RemotePtr<void> addr, std::size_t page_count);
};

} // namespace mc
} // namespace simgrid

#endif
