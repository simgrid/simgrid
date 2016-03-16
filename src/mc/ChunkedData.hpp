/* Copyright (c) 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_CHUNKED_DATA_HPP
#define SIMGRID_MC_CHUNKED_DATA_HPP

#include <cstddef>
#include <cstdint>

#include <utility>
#include <vector>

#include "src/mc/mc_forward.hpp"
#include "src/mc/PageStore.hpp"

namespace simgrid {
namespace mc {

/** A byte-string represented as a sequence of chunks from a PageStor */
class ChunkedData {
  PageStore* store_ = nullptr;
  /** Indices of the chunks */
  std::vector<std::size_t> pagenos_;
public:

  ChunkedData() {}
  void clear()
  {
    for (std::size_t pageno : pagenos_)
      store_->unref_page(pageno);
    pagenos_.clear();
  }
  ~ChunkedData()
  {
    clear();
  }

  // Copy and move
  ChunkedData(ChunkedData const& that)
  {
    store_ = that.store_;
    pagenos_ = that.pagenos_;
    for (std::size_t pageno : pagenos_)
      store_->ref_page(pageno);
  }
  ChunkedData(ChunkedData&& that)
  {
    store_ = that.store_;
    that.store_ = nullptr;
    pagenos_ = std::move(that.pagenos_);
    that.pagenos_.clear();
  }
  ChunkedData& operator=(ChunkedData const& that)
  {
    this->clear();
    store_ = that.store_;
    pagenos_ = that.pagenos_;
    for (std::size_t pageno : pagenos_)
      store_->ref_page(pageno);
    return *this;
  }
  ChunkedData& operator=(ChunkedData && that)
  {
    this->clear();
    store_ = that.store_;
    that.store_ = nullptr;
    pagenos_ = std::move(that.pagenos_);
    that.pagenos_.clear();
    return *this;
  }

  std::size_t page_count()          const { return pagenos_.size(); }
  std::size_t pageno(std::size_t i) const { return pagenos_[i]; }
  const std::size_t* pagenos()      const { return pagenos_.data(); }

  const void* page(std::size_t i) const
  {
    return store_->get_page(pagenos_[i]);
  }

  ChunkedData(PageStore& store, AddressSpace& as,
    RemotePtr<void> addr, std::size_t page_count,
    const std::size_t* ref_page_numbers, const std::uint64_t* pagemap);
};

}
}

#endif
