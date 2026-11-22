/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/smemory/SlabTrackState.hpp"

#include <algorithm>
#include <iterator>

namespace simgrid::mc::smemory {
static_assert(sizeof(SlabTrackState::Slab) == 12,
              "SlabTrackState::Slab must be exactly 12 bytes for optimal cache packing");

/* ********************** Page implementation ********************** */
/* ***************************************************************** */

/* ********************** FastTrack Rules ********************** */
void SlabTrackState::Page::check_read_race(const Slab& node, uintptr_t page_base, Epoch current,
                                           const smemory::VectorClock& thread_vc) const
{
  const unsigned size = node.end_ - node.start_;
  if (node.write_.get_aid() != current.get_aid() &&
      node.write_.get_clock() > thread_vc.clocks[node.write_.get_aid().value()].get_clock()) [[unlikely]]
    throw McDataRace(node.write_, MemOpType::Write, current, MemOpType::Read, page_base + node.start_, size, size);
}

void SlabTrackState::Page::check_write_race(const Slab& node, uintptr_t page_base, Epoch current,
                                            const smemory::VectorClock& thread_vc,
                                            const smemory::VectorClockPool& pool) const
{
  const unsigned size = node.end_ - node.start_;
  if (node.write_.get_aid() != current.get_aid() &&
      node.write_.get_clock() > thread_vc.clocks[node.write_.get_aid().value()].get_clock()) [[unlikely]]
    throw McDataRace(node.write_, MemOpType::Write, current, MemOpType::Write, page_base + node.start_, size, size);

  if (node.reads_.is_pure_epoch()) {
    if (node.reads_.get_aid() != current.get_aid() &&
        node.reads_.get_clock() > thread_vc.clocks[node.reads_.get_aid().value()].get_clock()) [[unlikely]]
      throw McDataRace(node.reads_, MemOpType::Read, current, MemOpType::Write, page_base + node.start_, size, size);
  } else if (not pool.is_past(node.reads_.get_pool_index(), thread_vc)) [[unlikely]] {
    // Report one of the readers that are not in the past of the current access
    const smemory::VectorClock& readers = pool.get(node.reads_.get_pool_index());
    for (std::size_t i = 0; i < static_config::max_threads; ++i)
      if (thread_vc.clocks[i] < readers.clocks[i])
        throw McDataRace(readers.clocks[i], MemOpType::Read, current, MemOpType::Write, page_base + node.start_, size,
                         size);
    xbt_die("is_past() found a racy reader that cannot be found anymore");
  }
}

void SlabTrackState::Page::update_read_epoch(Slab& node, Aid t, Clock c, const smemory::VectorClock& thread_vc,
                                             smemory::VectorClockPool& pool) const
{
  Epoch current_epoch(t, c);
  if (node.reads_ == current_epoch)
    return;

  if (node.reads_.is_pure_epoch()) {
    if (node.reads_.get_clock() <= thread_vc.clocks[node.reads_.get_aid().value()].get_clock()) {
      node.reads_ = current_epoch;
    } else {
      smemory::VectorClock new_vc;
      new_vc.clocks[node.reads_.get_aid().value()] = node.reads_;
      new_vc.clocks[t.value()]                     = current_epoch;
      node.reads_                                  = Epoch(pool.get_or_insert(new_vc));
    }
  } else {
    const smemory::VectorClock& rx_vc = pool.get(node.reads_.get_pool_index());
    smemory::VectorClock new_vc       = rx_vc;
    new_vc.clocks[t.value()]          = current_epoch;
    node.reads_                       = Epoch(pool.get_or_insert(new_vc));
  }
}

/* ********************** In-Place Batch Application ********************** */

void SlabTrackState::Page::apply_batch_operations(const smemory::MemoryAccessTrace::PageAccessView& accesses, Aid t,
                                                  Clock c, const smemory::VectorClock& thread_vc,
                                                  smemory::VectorClockPool& pool, std::vector<Slab>& scratchpad)
{
  Epoch current_epoch(t, c);

  bool out_of_place = false;
  size_t write_idx  = 0;
  size_t read_idx   = 0;

  // This lambda pushes one slab to the vector that we built so far, possibly, coalescing it to the last slab of the
  // vector (unless the coalescing is disabled, to get the original FastTrack)
  auto push_and_coalesce = [&](const Slab& inv) {
    if (out_of_place) {
      if (static_config::coalescing && not scratchpad.empty() && scratchpad.back().end_ == inv.start_ &&
          scratchpad.back().has_same_epochs(inv))
        scratchpad.back().end_ = inv.end_;
      else
        scratchpad.push_back(inv);

    } else {
      if (static_config::coalescing && write_idx > 0 && slabs_[write_idx - 1].end_ == inv.start_ &&
          slabs_[write_idx - 1].has_same_epochs(inv)) {
        slabs_[write_idx - 1].end_ = inv.end_;
      } else {
        if (write_idx <= read_idx) { // Safe in-place write: we are behind or at the read cursor
          slabs_[write_idx++] = inv;
        } else { // COLLISION! We would overwrite an interval we haven't read yet.
          out_of_place = true;

          scratchpad.clear();
          scratchpad.insert(scratchpad.end(), slabs_.begin(), slabs_.begin() + write_idx);
          scratchpad.push_back(inv);
        }
      }
    }
  };

  // Traverse at the same time the accesses that need to be pushed and the existing slabs in our page
  auto access_it  = accesses.begin();
  auto access_end = accesses.end();

  for (read_idx = 0; read_idx < slabs_.size(); read_idx++) {
    Slab curr_slab = slabs_[read_idx];

    // Process the current slab (we may need to split it, thus the unusual while loop)
    while (curr_slab.start_ < curr_slab.end_) {

      // If no more accesses to see, or next access does not intersect with current slab
      if (access_it == access_end || access_it->start_offset >= curr_slab.end_) {
        push_and_coalesce(curr_slab);
        break; // Advance to the next slab
      }

      // Skip accesses that are completely behind (should not happen with sorted input)
      if (access_it->end_offset <= curr_slab.start_) {
        ++access_it;
        continue; // Next slab, please
      }

      // We have an overlap!
      uint16_t overlap_start = std::max(curr_slab.start_, access_it->start_offset);
      uint16_t overlap_end   = std::min(curr_slab.end_, access_it->end_offset);

      // Emit the untouched prefix (if any)
      if (curr_slab.start_ < overlap_start) {
        Slab prefix = curr_slab;
        prefix.end_ = overlap_start;
        push_and_coalesce(prefix);
        curr_slab.start_ = overlap_start;
      }

      // Process the overlapping segment
      Slab overlap   = curr_slab;
      overlap.start_ = overlap_start;
      overlap.end_   = overlap_end;

      if (access_it->type == smemory::MemOpType::Write) {
        check_write_race(overlap, accesses.page_base_addr, current_epoch, thread_vc, pool);
        overlap.write_ = current_epoch;
        overlap.reads_ = BOTTOM; // Wipe reads on successful write
      } else {
        check_read_race(overlap, accesses.page_base_addr, current_epoch, thread_vc);
        update_read_epoch(overlap, t, c, thread_vc, pool);
      }

      push_and_coalesce(overlap);
      curr_slab.start_ = overlap_end;

      if (access_it->end_offset <= overlap_end) // If the access is exhausted, move to the next access
        ++access_it;
    }
  }

  // Finalize the container size
  if (out_of_place) {
    slabs_.swap(scratchpad);
  } else {
    // Simply shrink the logical size, keeping memory capacity for next operations.
    if (write_idx < slabs_.size())
      slabs_.erase(slabs_.begin() + write_idx, slabs_.end());
  }
}

bool SlabTrackState::Page::clean_before(const smemory::VectorClock& global_min, const smemory::VectorClockPool& pool)
{
  auto is_past = [&global_min](Epoch e) { return e.get_clock() <= global_min.clocks[e.get_aid().value()].get_clock(); };

  // The slabs are contiguous, so merging a slab into the previous one only requires to extend the previous one
  size_t kept = 0;
  for (Slab slab : slabs_) {
    if (is_past(slab.write_))
      slab.write_ = BOTTOM;
    if (slab.reads_.is_pure_epoch() ? is_past(slab.reads_) : pool.is_past(slab.reads_.get_pool_index(), global_min))
      slab.reads_ = BOTTOM;

    if (static_config::coalescing && kept > 0 && slabs_[kept - 1].has_same_epochs(slab))
      slabs_[kept - 1].end_ = slab.end_;
    else
      slabs_[kept++] = slab;
  }
  slabs_.erase(slabs_.begin() + kept, slabs_.end());

  return std::ranges::all_of(slabs_, [](const Slab& slab) { return slab.has_same_epochs(Slab(0, 0, BOTTOM, BOTTOM)); });
}

void SlabTrackState::Page::reset(uint16_t start, uint16_t end, std::vector<Slab>& scratchpad)
{
  scratchpad.clear();
  auto push = [&scratchpad](const Slab& slab) {
    if (slab.start_ >= slab.end_)
      return;
    if (static_config::coalescing && not scratchpad.empty() && scratchpad.back().has_same_epochs(slab))
      scratchpad.back().end_ = slab.end_;
    else
      scratchpad.push_back(slab);
  };

  // The slabs cover the whole page in order, so the reset slab is pushed when meeting the first slab that overlaps it
  bool reset_pushed = false;
  for (const Slab& slab : slabs_) {
    if (slab.end_ <= start || slab.start_ >= end) {
      push(slab);
      continue;
    }
    push(Slab(slab.start_, start, slab.write_, slab.reads_)); // Part before the reset range (if any)
    if (not reset_pushed) {
      push(Slab(start, end, BOTTOM, BOTTOM));
      reset_pushed = true;
    }
    push(Slab(end, slab.end_, slab.write_, slab.reads_)); // Part after the reset range (if any)
  }
  slabs_.swap(scratchpad);
}

/* ********************** Container implementation ********************** */
/* ********************************************************************** */

void SlabTrackState::apply_transition(const smemory::MemoryAccessTrace& record, Aid t,
                                      const smemory::VectorClock& thread_vc)
{
  const Epoch current = thread_vc.clocks[t.value()];
  xbt_assert(current.get_aid() == t, "Component %d of the vector clock belongs to actor %d", t.c_val(),
             current.get_aid().c_val());
  const Clock c = current.get_clock();
  xbt_assert(c.value() >= 1, "Clock 0 is reserved for BOTTOM, it cannot be used for the accesses of actor %d",
             t.c_val());

  // Forget the previous accesses to the blocks allocated during the segment. The segment does not keep the order of its
  // events, so its accesses are all considered to be done after its allocations.
  for (auto const& [addr, size] : record.get_allocations())
    for (uintptr_t page_base = addr & ~(static_config::page_size - 1); page_base < addr + size;
         page_base += static_config::page_size)
      if (auto it = pages_.find(page_base); it != pages_.end())
        it->second.reset(static_cast<uint16_t>(std::max(addr, page_base) - page_base),
                         static_cast<uint16_t>(std::min(addr + size, page_base + static_config::page_size) - page_base),
                         scratchpad_);

  // For each page of the MemoryAccessTrace, merge it to the corresponding page of the SlabTrackState
  for (const auto& access_it : record) {

    Page& page = get_or_create_page(access_it.page_base_addr);

    page.apply_batch_operations(access_it, t, c, thread_vc, pool_, scratchpad_);
  }
}

void SlabTrackState::clean_before(const smemory::VectorClock& global_min)
{
  // The slabs must stop referencing the obsolete VCs before the pool frees them. This modifies the pages, which is not
  // allowed in the predicate of std::erase_if (some versions of libstdc++ pass the elements as const).
  for (auto it = pages_.begin(); it != pages_.end();)
    it = it->second.clean_before(global_min, pool_) ? pages_.erase(it) : std::next(it);
  pool_.clean_before(global_min);
}

} // namespace simgrid::mc::smemory