/* Copyright (c) 2026-2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SMEMORY_MEMORYACCESSTRACKER_HPP
#define SIMGRID_MC_SMEMORY_MEMORYACCESSTRACKER_HPP

#include "src/mc/remote/Channel.hpp"
#include "src/mc/smemory/smemory_config.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <vector>

/* This structure keeps track the read and write accesses to the memory during execution of a given transition, to
 * detect race conditions in SlabTrackState (see this file for an overview). We need to list all memory areas that were
 * written to during this transition, and the ones that were read from. If a memory address was both read and written,
 * only the write needs to be reported.
 *
 * It uses a `struct Page` per page of memory (actually, per chunk of 4kb regardless of the page size). That
 * substructure has two bitfields each with one bit per byte in the page. The first bitfield is used to mark whether the
 * byte was read during this transition while the other bitfield marks whether it was writen. The data structure is
 * optimized both in time and space, as a transition may touch megabytes or even gigabytes of data.
 *
 * To further optimize, one can change the constexpr `granularity` parameter in smemory_config. When its value is 4, we
 * use one bit per set of 4 bytes in memory (these 4 bytes are said to form a bucket), reducing the amount of memory by
 * a factor of 4, but possibly creating false shared accesses in memory. 1, 2, 4, 8, 16 are possible granularity values.
 *
 * This structure comes with a two-level online iterator. The outer iterator traverses the memory pages touched during
 * the transition. The inner iterator yields `PageInterval`s on the fly, merging contiguous bits together into a single
 * interval without heap allocations. This hierarchical design enables O(1) FastPaths in the SlabTrackState.
 */

namespace simgrid::mc::smemory {

XBT_DECLARE_ENUM_CLASS(MemOpType, Read, Write);

class MemoryAccessTrace {
private:
  // __builtin_ctzll: long long log2 (number of trailing 0-bits in x) uintptr_t is a long long
  static constexpr uintptr_t page_shift_ = __builtin_ctzll(smemory::config::page_size);
  static constexpr uintptr_t page_mask_  = smemory::config::page_size - 1;

  constexpr static uintptr_t bucket_shift_     = __builtin_ctzll(smemory::config::granularity);
  constexpr static uintptr_t word_shift_       = bucket_shift_ + 6;
  constexpr static uintptr_t buckets_per_page_ = smemory::config::page_size >> bucket_shift_;
  constexpr static uintptr_t words_per_page_   = buckets_per_page_ / 64;

  static_assert(buckets_per_page_ % 64 == 0,
                "4096/granularity must be multiple of 64 (because words of the bitfield are uint64_t)");

  // Whether all bytes of a memory area shall be marked and whether adjacent bytes shall be considered as a single
  // (larger) access
  static constexpr bool coalescing_ = false;

  struct Page {
    std::vector<uint64_t> read_bits  = std::vector<uint64_t>(words_per_page_, 0);
    std::vector<uint64_t> write_bits = std::vector<uint64_t>(words_per_page_, 0);

    void mark_bucket(uintptr_t bucket, MemOpType type);
    std::optional<uintptr_t> find_prev_marked_bucket(uintptr_t start_bucket, MemOpType type) const;
    std::optional<uintptr_t> find_next_marked_bucket(uintptr_t start_bucket, MemOpType type) const;
  };
  std::unordered_map<uintptr_t, Page> pages_;

  bool was_marked(uintptr_t where, MemOpType type) const;

public:
  /* Main input method. The observation uses this method only */
  void create_memory_access(MemOpType type, uintptr_t where, unsigned short size);

  bool was_written(uintptr_t where) const { return was_marked(where, MemOpType::Write); }
  bool was_read(uintptr_t where) const
  {
    return (not was_marked(where, MemOpType::Write)) && was_marked(where, MemOpType::Read);
  }
  constexpr static size_t get_bucket_size() { return smemory::config::granularity; }
  constexpr static bool is_coalescing() { return coalescing_; }

  // ============================================================
  //                    Serialization
  // ============================================================
  void serialize(Channel& chan);
  void deserialize(Channel& chan);

  // ============================================================
  //                 Two-level Online Iterator
  // ============================================================
  // Lightweight interval yielded by the inner iterator
  struct PageInterval {
    uint16_t start_offset;
    uint16_t end_offset;
    MemOpType type;
  };

  // Inner Iterator: parses bits of a page on the fly
  class AccessIterator {
  private:
    const Page* page_              = nullptr;
    uintptr_t bucket_pos_          = 0;
    PageInterval current_interval_ = {};
    bool is_end_                   = true;

    void advance();

  public:
    using value_type        = PageInterval;
    using reference         = const PageInterval&;
    using pointer           = const PageInterval*;
    using difference_type   = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    AccessIterator() = default;
    AccessIterator(const Page* page, bool is_end) : page_(page), is_end_(is_end)
    {
      if (not is_end)
        advance();
    }

    reference operator*() const { return current_interval_; }
    pointer operator->() const { return &current_interval_; }
    AccessIterator& operator++()
    {
      advance();
      return *this;
    }
    bool operator==(const AccessIterator& other) const
    {
      if (is_end_ && other.is_end_)
        return true;
      return is_end_ == other.is_end_ && page_ == other.page_ && bucket_pos_ == other.bucket_pos_;
    }
    bool operator!=(const AccessIterator& other) const { return not(*this == other); }
  };

  // View over a page, yielded by the outer iterator
  struct PageAccessView {
    uintptr_t page_base_addr;
    const Page* page_ptr;
    AccessIterator begin_it;
    AccessIterator end_it;

    AccessIterator begin() const { return begin_it; }
    AccessIterator end() const { return end_it; }
  };

  // Outer Iterator: iterates over touched pages
  class PageIterator {
  private:
    using OuterIt = typename std::unordered_map<uintptr_t, Page>::const_iterator;
    OuterIt current_page_it_;
    OuterIt end_page_it_;
    mutable std::optional<PageAccessView> current_view_;

    void compute_current_view() const;

  public:
    using value_type        = PageAccessView;
    using reference         = const PageAccessView&;
    using pointer           = const PageAccessView*;
    using difference_type   = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    PageIterator(OuterIt it, OuterIt end_it) : current_page_it_(it), end_page_it_(end_it) {}

    reference operator*() const
    {
      compute_current_view();
      return *current_view_;
    }
    pointer operator->() const
    {
      compute_current_view();
      return &*current_view_;
    }

    PageIterator& operator++()
    {
      current_view_.reset();
      ++current_page_it_;
      return *this;
    }

    bool operator==(const PageIterator& other) const { return current_page_it_ == other.current_page_it_; }
    bool operator!=(const PageIterator& other) const { return not(*this == other); }
  };

  PageIterator begin() const { return PageIterator(pages_.begin(), pages_.end()); }
  PageIterator end() const { return PageIterator(pages_.end(), pages_.end()); }
};

} // namespace simgrid::mc::smemory
#endif