/* Copyright (c) 2026-2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/smemory/MemoryAccessTracker.hpp"
#include "xbt/asserts.h"
#include "xbt/log.h"

#include <cstdint>
#include <limits>

XBT_LOG_NEW_SUBCATEGORY(smemory, mc, "Tracking memory accesses");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smem_mark, smemory, "Tracking memory accesses");

namespace simgrid::mc::smemory {

void MemoryAccessTracker::create_memory_access(MemOpType type, void* where, unsigned char size)
{
  // Save the memory access to the bitmap
  uintptr_t addr = reinterpret_cast<uintptr_t>(where);
  uintptr_t end  = addr + size; // ignored when coalescing

  if (not coalescing_) {
    XBT_DEBUG("Marking the first byte of memory at %p as %s", where, to_c_str(type));
    uintptr_t page_index = addr >> page_shift_;
    uintptr_t page_base  = page_index << page_shift_;
    uintptr_t offset     = addr - page_base;
    if (pages_.find(page_index) == pages_.end())
      sorted_pages_dirty_ = true;
    Page& page = pages_.try_emplace(page_index).first->second;

    uintptr_t first_bucket = offset >> bucket_shift_;
    uintptr_t word_index   = first_bucket >> 6;
    uintptr_t first_bit    = first_bucket & 63;
    uint64_t mask          = 1ULL << first_bit;

    if (type == MemOpType::Write)
      page.write_bits[word_index] |= mask;
    else
      page.read_bits[word_index] |= mask;

    return;
  }

  XBT_DEBUG("Marking %u bytes at %p as %s", size, where, to_c_str(type));

  while (addr < end) {

    uintptr_t page_index = addr >> page_shift_;

    uintptr_t page_base = page_index << page_shift_;
    uintptr_t page_end  = page_base + page_size_;

    // The end of what can be written to a single page from what needs to be marked
    uintptr_t limit = (end < page_end) ? end : page_end;

    // We call mark_bytes() for each word of the underlying bitmap
    while (addr < limit) {
      uintptr_t offset = addr - page_base;

      // End of the current word (as an address, in bytes)
      uintptr_t word_end = page_base + (((offset >> word_shift_) + 1) << word_shift_);
      // End of what needs to be marked as read (resp. written)
      uintptr_t chunk_end = (word_end < limit) ? word_end : limit;

      //      mark_bytes(page_index, offset, chunk_end - addr, type);
      /* Go and mark the corresponding word */
      if (pages_.find(page_index) == pages_.end())
        sorted_pages_dirty_ = true;
      Page& page = pages_.try_emplace(page_index).first->second;

      uintptr_t first_bucket = offset >> bucket_shift_;
      uintptr_t last_bucket  = (offset + (chunk_end - addr) - 1) >> bucket_shift_;

      uintptr_t word_index = first_bucket >> 6;

      uintptr_t first_bit = first_bucket & 63;
      uintptr_t last_bit  = last_bucket & 63;

      uint64_t mask = ((1ULL << (last_bit - first_bit + 1)) - 1) << first_bit;

      if (type == MemOpType::Write)
        page.write_bits[word_index] |= mask;
      else
        page.read_bits[word_index] |= mask;

      /* Advance to after this word */
      addr = chunk_end;
    }
  }
}

bool MemoryAccessTracker::was_marked(void* where, MemOpType type) const
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
void MemoryAccessTracker::Page::mark_bucket(uintptr_t bucket, MemOpType type)
{
  uintptr_t word_index = bucket >> 6;
  uintptr_t bit_index  = bucket & 63;

  auto& word = type == MemOpType::Read ? read_bits[word_index] : write_bits[word_index];
  word |= 1ULL << bit_index;
}

// Search the marked bucket at or before start_bucket (including start_bucket)
//
// Returns std::nullopt if no suck bucket exists in this page
std::optional<uintptr_t> MemoryAccessTracker::Page::find_prev_marked_bucket(uintptr_t start_bucket,
                                                                            MemOpType type) const
{
  xbt_assert(start_bucket < buckets_per_page_);

  const auto& bitfield = MemOpType::Write == type ? write_bits : read_bits;
  // --- Find the bucket in the bitfield ---
  const uintptr_t start_word = start_bucket >> 6;
  const uintptr_t bit_index  = start_bucket & 63; // bit in word

  // --- First look in the current word (fast path)
  // Mask of all bits of index <= start_bucket, to compare in the first word where we want to ignore some bits
  const uint64_t word = bitfield[start_word];
  // In masked_first_word, we handle separately the case where bit_index==63 to avoid an overflow at (1ULL << 64)
  uint64_t masked_first_word = bit_index == 63 ? word : (word & ((1ULL << (bit_index + 1)) - 1));
  if (masked_first_word != 0) {
    // index of most significant bit set in this mask using _builtin_clzll (number of leading 0-bits in x \in uint64_t)
    unsigned highest_bit = 63 - __builtin_clzll(masked_first_word);
    return (start_word << 6) + highest_bit;
  }

  // --- If not found, look the other words iteratively
  for (uintptr_t wi = start_word; wi > 0;) {
    wi--;
    uint64_t word_it = bitfield[wi];
    if (word_it != 0) {
      unsigned highest_bit = 63 - __builtin_clzll(word_it);
      return (wi << 6) + highest_bit;
    }
  }

  // Not found
  return std::nullopt;
}

// Search the marked bucket at or after start_bucket (including start_bucket)
//
// Returns std::nullopt if no such bucket exists in this page
std::optional<uintptr_t> MemoryAccessTracker::Page::find_next_marked_bucket(uintptr_t start_bucket,
                                                                            MemOpType type) const
{
  xbt_assert(start_bucket < buckets_per_page_);

  const auto& bitfield = (MemOpType::Write == type) ? write_bits : read_bits;

  const uintptr_t start_word = start_bucket >> 6;
  const uintptr_t bit_index  = start_bucket & 63;

  // --- First look in the current word (fast path)
  const uint64_t word = bitfield[start_word];

  // Keep only bits >= bit_index
  const uint64_t masked_first_word = word & (~0ULL << bit_index);

  if (masked_first_word != 0) {
    // index of least significant bit set
    unsigned lowest_bit = __builtin_ctzll(masked_first_word);
    return (start_word << 6) + lowest_bit;
  }

  // --- If not found, look the other words iteratively
  for (uintptr_t wi = start_word + 1; wi < words_per_page_; ++wi) {
    const uint64_t word_it = bitfield[wi];
    if (word_it != 0) {
      unsigned lowest_bit = __builtin_ctzll(word_it);
      return (wi << 6) + lowest_bit;
    }
  }

  return std::nullopt;
}

std::pair<uintptr_t, uintptr_t> MemoryAccessTracker::expand_around_memory_chunk(uintptr_t addr, size_t size,
                                                                                MemOpType kind)
{
  xbt_assert((addr & (granularity_ - 1)) == 0, "addr must be on a bucket boundary");
  xbt_assert(((addr + size) & (granularity_ - 1)) == 0, "addr+size must be on a bucket boundary");
  xbt_assert(coalescing_, "expand_around_memory_chunk() does not work when the MemoryAccessTracker is not coalescing "
                          "adjacent memory accesses");

  if (sorted_pages_dirty_)
    sort_pages();

  const uintptr_t first_page   = addr >> page_shift_;
  const uintptr_t first_bucket = (addr & page_mask_) >> bucket_shift_;
  const uintptr_t last_addr    = addr + size - 1;
  const uintptr_t last_page    = last_addr >> page_shift_;
  const uintptr_t last_bucket  = (last_addr & page_mask_) >> bucket_shift_;

  uintptr_t begin = 0;
  uintptr_t end   = std::numeric_limits<uintptr_t>::max();

  auto it_first = std::lower_bound(sorted_pages_.begin(), sorted_pages_.end(), first_page);
  auto it_last  = (first_page == last_page) ? it_first : std::lower_bound(it_first, sorted_pages_.end(), last_page);

  // ── SEARCH LEFT ─────────────────────────────────────────────────────────────

  auto left_it = it_first;

  // 1) inside the current page, to the left of first_bucket

  if (left_it != sorted_pages_.end() && *left_it == first_page) { // Only search the first page if it exists
    if (first_bucket > 0) {                                       // Something to search

      if (auto found = pages_.at(first_page).find_prev_marked_bucket(first_bucket - 1, kind)) {
        // begin must be the left boundary of the marked bucket
        begin = (first_page << page_shift_) + ((*found + 1) << bucket_shift_);
      }
    }
  }

  // 2) across the previous pages, starting at the page end
  if (begin == 0) {
    auto left_it = it_first;
    while (left_it != sorted_pages_.begin()) {
      --left_it;
      auto last_marked = pages_.at(*left_it).find_prev_marked_bucket(buckets_per_page_ - 1, kind);
      if (last_marked.has_value()) {
        begin = (*left_it << page_shift_) + ((last_marked.value() + 1) << bucket_shift_);
        break;
      }
    }
  }

  // ── SEARCH RIGHT ────────────────────────────────────────────────────────────
  // 1) inside the current page, to the right of last_bucklet
  if (last_bucket + 1 < buckets_per_page_ && it_last != sorted_pages_.end() && *it_last == last_page) {
    auto next = pages_.at(last_page).find_next_marked_bucket(last_bucket + 1, kind);
    if (next.has_value())
      end = (last_page << page_shift_) + (next.value() << bucket_shift_) - 1;
  }

  // 2) across the following pages, starting at the page beginning
  if (end == std::numeric_limits<uintptr_t>::max()) {
    auto right_it = (it_last != sorted_pages_.end() && *it_last == last_page) ? std::next(it_last) : it_last;
    while (right_it != sorted_pages_.end()) {
      auto first_marked = pages_.at(*right_it).find_next_marked_bucket(0, kind);
      if (first_marked.has_value()) {
        end = (*right_it << page_shift_) + (first_marked.value() << bucket_shift_) - 1;
        break;
      }
      ++right_it;
    }
  }

  return {begin, end};
}

void MemoryAccessTracker::serialize(Channel& channel)
{
  unsigned int page_count = pages_.size();
  channel.pack<unsigned int>(page_count);

  XBT_DEBUG("Serialize a transition with %u pages", page_count);
  for (auto& [addr, page] : pages_) {
    channel.pack<uintptr_t>(addr);
    for (unsigned wit = 0; wit < words_per_page_; wit++)
      channel.pack<uint64_t>(page.write_bits[wit]);
    for (unsigned rit = 0; rit < words_per_page_; rit++)
      channel.pack<uint64_t>(page.read_bits[rit]);
  }
}
void MemoryAccessTracker::deserialize(Channel& channel)
{
  xbt_assert(pages_.empty(), "Cannot deserialize a memory tracker when the receiver is not empty");

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
}

void MemoryAccessTracker::iterator::advance()
{
  if (parent_ == nullptr) // don't advance beyond end()
    return;

  while (page_pos_ < parent_->sorted_pages_.size()) {

    uintptr_t page_index = parent_->sorted_pages_[page_pos_];
    const Page& page     = parent_->pages_.at(page_index);

    // Search the next enabled bit in the bitfields
    while (bucket_pos_ < parent_->buckets_per_page_) {

      uintptr_t word = bucket_pos_ >> 6;
      uintptr_t bit  = bucket_pos_ & 63;
      uint64_t mask  = 1ULL << bit;

      bool is_write = page.write_bits[word] & mask;
      bool is_read  = (is_write) || (page.read_bits[word] & mask); // Don't even compute is_read if is_write is true

      if (is_write || is_read) { // Found an enabled bit. Finalize the answer we will do

        MemOpType type = is_write ? MemOpType::Write : MemOpType::Read;

        uintptr_t start_bucket = bucket_pos_;
        uintptr_t base_addr    = (page_index << page_shift_) + (start_bucket << parent_->bucket_shift_);

        uintptr_t count = 0;
        if (coalescing_) {
          // Merge contiguous bits together in our answer
          while (bucket_pos_ < parent_->buckets_per_page_) {

            word = bucket_pos_ >> 6;
            bit  = bucket_pos_ & 63;
            mask = 1ULL << bit;

            bool w = page.write_bits[word] & mask;
            bool r = page.read_bits[word] & mask;

            if ((type == MemOpType::Write && w) || (type == MemOpType::Read && r && not w)) {
              // If we're still within the same chunk, advance the bucket.
              bucket_pos_++;
              count++;
            } else
              // Found the end of the chunk; bucket_pos is now one step too far, ready for the next call to advance()
              break;
          }
        } else {
          bucket_pos_++;
          count = 1;
        }

        current_ = std::make_tuple(reinterpret_cast<void*>(base_addr), count * parent_->granularity_, type);
        return;
      }

      // That bit was not enabled. Search further
      bucket_pos_++;
    }

    // We're done with the last bucket of that page, iterate to the next one
    page_pos_++;
    bucket_pos_ = 0;
  }

  parent_ = nullptr; // end() reached
}
} // namespace simgrid::mc::smemory