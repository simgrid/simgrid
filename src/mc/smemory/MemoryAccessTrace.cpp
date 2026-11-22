/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/smemory/MemoryAccessTrace.hpp"
#include "xbt/asserts.h"
#include "xbt/log.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <inttypes.h>

XBT_LOG_NEW_SUBCATEGORY(smemory, mc, "Tracking memory accesses");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smem_mark, smemory, "Tracking memory accesses");

namespace simgrid::mc::smemory {

void MemoryAccessTrace::create_memory_access(MemOpType type, uintptr_t where, size_t size)
{
  if (size == 0) // e.g., a memset of 0 bytes
    return;

  XBT_DEBUG("Marking %zu bytes at %p as %s", size, (void*)where, to_c_str(type));
  uintptr_t addr      = where;
  const uintptr_t end = where + size;
  while (addr < end) {
    const uintptr_t page_index = addr >> page_shift_;
    const uintptr_t page_base  = page_index << page_shift_;
    Page& page                 = pages_.try_emplace(page_index).first->second;
    auto& bits                 = type == MemOpType::Write ? page.write_bits : page.read_bits;

    // Mark the buckets of this page, one word of the bitfield at a time
    const uintptr_t limit = std::min(end, page_base + static_config::page_size);
    while (addr < limit) {
      const uintptr_t offset   = addr - page_base;
      const uintptr_t word_end = page_base + (((offset >> word_shift_) + 1) << word_shift_);
      const uintptr_t chunk    = std::min(word_end, limit) - addr;

      const uintptr_t first_bucket = offset >> bucket_shift_;
      const uintptr_t last_bucket  = (offset + chunk - 1) >> bucket_shift_;
      const uintptr_t bit_count    = last_bucket - first_bucket + 1; // up to 64
      const uint64_t mask          = (bit_count == 64 ? ~0ULL : (1ULL << bit_count) - 1) << (first_bucket & 63);
      bits[first_bucket >> 6] |= mask;

      addr += chunk;
    }
  }
}

bool MemoryAccessTrace::was_marked(uintptr_t where, MemOpType type) const
{
  uintptr_t addr       = reinterpret_cast<uintptr_t>(where);
  uintptr_t page_index = addr >> page_shift_;

  auto it = pages_.find(page_index);
  if (it == pages_.end())
    return false;

  uintptr_t offset = addr & page_mask_;
  uintptr_t bucket = offset >> bucket_shift_;
  uintptr_t word   = bucket >> 6;
  uintptr_t bit    = bucket & 63;

  const auto& word_ctn = type == MemOpType::Read ? it->second.read_bits[word] : it->second.write_bits[word];
  return word_ctn & (1ULL << bit);
}
void MemoryAccessTrace::serialize(Channel& channel)
{
  unsigned int page_count = pages_.size();
  channel.pack<unsigned int>(page_count);

  if (XBT_LOG_ISENABLED(smem_mark, xbt_log_priority_verbose)) {
    unsigned long hit_count = 0;
    for (const auto& page_accesses : *this)
      hit_count += std::distance(page_accesses.begin(), page_accesses.end());
    XBT_VERB("Serialize a transition with %u pages, each of size %" PRIxPTR " (%lu accesses in this tracker)",
             page_count, words_per_page_ * sizeof(uint64_t) * 2, hit_count);
  }
  for (auto& [addr, page] : pages_) {
    channel.pack<uintptr_t>(addr);
    for (unsigned wit = 0; wit < words_per_page_; wit++)
      channel.pack<uint64_t>(page.write_bits[wit]);
    for (unsigned rit = 0; rit < words_per_page_; rit++)
      channel.pack<uint64_t>(page.read_bits[rit]);
  }
  channel.pack<unsigned int>(allocations_.size());
  for (auto const& [addr, size] : allocations_) {
    channel.pack<uintptr_t>(addr);
    channel.pack<size_t>(size);
  }
  clear();
}
void MemoryAccessTrace::deserialize(Channel& channel)
{
  xbt_assert(empty(), "Cannot deserialize a memory tracker when the receiver is not empty");

  auto page_count = channel.unpack<unsigned int>();
  XBT_DEBUG("Deserialize a transition with %u pages", page_count);

  for (unsigned pit = 0; pit < page_count; pit++) {
    uintptr_t addr = channel.unpack<uintptr_t>();
    Page& page     = pages_.try_emplace(addr).first->second;

    for (unsigned wit = 0; wit < words_per_page_; wit++)
      page.write_bits[wit] = channel.unpack<uint64_t>();
    for (unsigned rit = 0; rit < words_per_page_; rit++)
      page.read_bits[rit] = channel.unpack<uint64_t>();
  }
  auto allocation_count = channel.unpack<unsigned int>();
  for (unsigned ait = 0; ait < allocation_count; ait++) {
    auto addr = channel.unpack<uintptr_t>();
    allocations_.emplace_back(addr, channel.unpack<size_t>());
  }
}

// ============================================================
//                 Two-level Online Iterator logic
// ============================================================

void MemoryAccessTrace::PageIterator::compute_current_view() const
{
  if (current_view_.has_value())
    return;
  if (current_page_it_ == end_page_it_)
    return;

  uintptr_t page_index = current_page_it_->first;
  uintptr_t page_base  = page_index << page_shift_;
  const Page& page     = current_page_it_->second;

  current_view_ = PageAccessView{page_base, &page, AccessIterator(&page, false), AccessIterator(&page, true)};
}

void MemoryAccessTrace::AccessIterator::advance()
{
  if (is_end_ || not page_) {
    is_end_ = true;
    return;
  }

  // Find the next marked bucket, skipping the empty words at once
  while (bucket_pos_ < buckets_per_page_) {
    const uintptr_t word   = bucket_pos_ >> 6;
    const uint64_t pending = (page_->write_bits[word] | page_->read_bits[word]) >> (bucket_pos_ & 63);
    if (pending == 0) {
      bucket_pos_ = (word + 1) << 6;
      continue;
    }
    bucket_pos_ += std::countr_zero(pending);

    // A bucket both read and written is reported as written: the checks of a write include the ones of a read
    const uintptr_t start_bucket = bucket_pos_;
    const MemOpType type = (page_->write_bits[word] >> (bucket_pos_ & 63)) & 1 ? MemOpType::Write : MemOpType::Read;
    bucket_pos_++;

    // Extend the interval to the following buckets accessed the same way
    if (static_config::coalescing) {
      while (bucket_pos_ < buckets_per_page_) {
        const uintptr_t next_word = bucket_pos_ >> 6;
        const uint64_t same  = type == MemOpType::Write ? page_->write_bits[next_word]
                                                        : page_->read_bits[next_word] & ~page_->write_bits[next_word];
        const unsigned shift = bucket_pos_ & 63;
        const unsigned run   = std::countr_one(same >> shift);
        bucket_pos_ += std::min(run, 64 - shift);
        if (run < 64 - shift)
          break;
      }
    }

    current_interval_ = {static_cast<uint16_t>(start_bucket * static_config::granularity),
                         static_cast<uint16_t>(bucket_pos_ * static_config::granularity), type};
    return;
  }

  is_end_ = true;
}
} // namespace simgrid::mc::smemory