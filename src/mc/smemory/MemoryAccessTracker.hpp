/* Copyright (c) 2026-2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SMEMORY_MEMORYACCESSTRACKER_HPP
#define SIMGRID_MC_SMEMORY_MEMORYACCESSTRACKER_HPP

#include "src/mc/remote/Channel.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <vector>

/* This structure tracks the read and write accesses to the memory during execution of a given transition, to detect
 * race conditions. We need to list all memory areas that were written to during this transition, and the ones that were
 * read from. If a memory address was both read and written, we need to report that it was written to.
 *
 * It uses a `struct Page` per page of memory (actually, per chunk of 4kb regardless of the page size). That
 * substructure has two bitfields each with one bit per byte in the page. The first bitfield is used to mark whether the
 * byte was read during this transition while the other bitfield marks whether it was writen. The data structure is
 * optimized both in time and space, as a transition may touch megabytes or even gigabytes of data.
 *
 * To further optimize, one can change the constexpr `granularity` parameter. When its value is 4, we use one bit per
 * set of 4 bytes in memory (these 4 bytes are said to form a bucklet), reducing the amount of memory by a factor of 4,
 * but possibly creating false shared accesses in memory. 1, 2, 4, 8, 16 are possible granularity values.
 *
 * This structure comes with an iterator that traverses the list of all contiguous memory areas (i.e., two adjacent
 * reads are merged by this iterator into a single read). For each item, a tuple {base address, size in bytes,
 * MemOpType::Read or Write} is returned.
 */

namespace simgrid::mc::smemory {

XBT_DECLARE_ENUM_CLASS(MemOpType, Read, Write);

class MemoryAccessTracker {
private:
  static constexpr uintptr_t page_shift_ = 12;                  // 12 gives 4k per page
  static constexpr uintptr_t PAGE_SIZE   = 1ULL << page_shift_; /* 4096 bytes */
  static constexpr uintptr_t PAGE_MASK   = PAGE_SIZE - 1;

  constexpr static uintptr_t granularity_ = 4;
  static_assert(granularity_ != 0 && ((granularity_ & (granularity_ - 1)) == 0),
                "MemoryAccessTracker::granularity must be power of two.");

  constexpr static uintptr_t bucket_shift_     = __builtin_ctz(granularity_); // log2 (number of trailing 0-bits in x)
  constexpr static uintptr_t word_shift_       = bucket_shift_ + 6;
  constexpr static uintptr_t buckets_per_page_ = PAGE_SIZE >> bucket_shift_;
  constexpr static uintptr_t words_per_page_   = buckets_per_page_ / 64;

  static_assert(buckets_per_page_ % 64 == 0,
                "4096/granularity must be multiple of 64 (because words of the bitfield are uint64_t)");

  struct Page {
    std::vector<uint64_t> read_bits  = std::vector<uint64_t>(words_per_page_, 0);
    std::vector<uint64_t> write_bits = std::vector<uint64_t>(words_per_page_, 0);

    bool has_mark(MemOpType type) const
    {
      auto& bits = type == MemOpType::Read ? read_bits : write_bits;
      for (uintptr_t i = 0; i < words_per_page_; ++i)
        if (bits[i] != 0)
          return true;
      return false;
    };
    void mark_bucket(uintptr_t bucket, MemOpType type);
    std::optional<uintptr_t> find_prev_marked_bucket(uintptr_t start_bucket, MemOpType type) const;
    std::optional<uintptr_t> find_next_marked_bucket(uintptr_t start_bucket, MemOpType type) const;
  };

  std::unordered_map<uintptr_t, Page> pages_;
  std::vector<uintptr_t> sorted_pages_;
  bool sorted_pages_dirty_ = true;
  void sort_pages()
  {
    sorted_pages_.clear();
    for (auto& kv : pages_)
      sorted_pages_.push_back(kv.first);
    std::sort(sorted_pages_.begin(), sorted_pages_.end());
    sorted_pages_dirty_ = false;
  }

  bool was_marked(void* where, MemOpType type) const;

public:
  void create_memory_access(MemOpType type, void* where, unsigned char size);
  bool was_written(void* where) const { return was_marked(where, MemOpType::Write); }
  bool was_read(void* where) const
  {
    return (not was_marked(where, MemOpType::Write)) && was_marked(where, MemOpType::Read);
  }
  constexpr static size_t get_bucket_size() { return granularity_; }
  // Search begin, the maximal value so that begin is unmarked and begin-1 is marked
  // Respectively, end is the minimal value so that end is unmarked and end+1 is marked
  void unused_interval_around(uintptr_t addr, uintptr_t& begin, uintptr_t& end, MemOpType kind);

  // ============================================================
  //                    Serialization
  // ============================================================
  void serialize(Channel& chan);
  void deserialize(Channel& chan);

  // ============================================================
  //                      Iterator
  // ============================================================
  class iterator {
  private:
    using OuterIt = typename std::unordered_map<uintptr_t, Page>::const_iterator;

    MemoryAccessTracker* parent_;
    size_t page_pos_      = 0;
    uintptr_t bucket_pos_ = 0;
    std::tuple<void*, size_t, MemOpType> current_;

    void advance();

  public:
    using value_type        = std::tuple<void*, size_t, MemOpType>;
    using reference         = value_type;
    using pointer           = void;
    using difference_type   = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    iterator(MemoryAccessTracker* p, bool end) : parent_(end ? nullptr : p)
    {
      if (not end) {
        if (parent_->sorted_pages_dirty_)
          parent_->sort_pages();
        advance();
      }
    }

    reference operator*() const { return current_; }

    iterator& operator++()
    {
      advance();
      return *this;
    }

    bool operator==(const iterator& other) const
    {
      if (parent_ == nullptr && other.parent_ == nullptr) // Disregard the positions when we reached the end
        return true;
      return parent_ == other.parent_ && page_pos_ == other.page_pos_ && bucket_pos_ == other.bucket_pos_;
    }

    bool operator!=(const iterator& other) const { return not(*this == other); }
  };

  iterator begin() { return iterator(this, false); }
  iterator end() { return iterator(this, true); }
};

} // namespace simgrid::mc::smemory
#endif