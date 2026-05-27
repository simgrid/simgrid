/* Copyright (c) 2026-2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/smemory/SlabTrackState.hpp"

namespace simgrid::mc::smemory {
static_assert(sizeof(SlabTrackState::Slab) == 12,
              "SlabTrackState::Slab must be exactly 12 bytes for optimal cache packing");

/* ********************** Page implementation ********************** */
/* ***************************************************************** */

/* ********************** FastTrack Rules ********************** */
void SlabTrackState::Page::check_read_race(const Slab& node, Aid t, const smemory::VectorClock& thread_vc) const
{
  if (node.write_.get_aid() != t &&
      node.write_.get_clock() > thread_vc.clocks[node.write_.get_aid().value()].get_clock()) [[unlikely]] {
    throw DataRaceException("FastTrack: Read DataRace detected (Write-Read conflict)");
  }
}

void SlabTrackState::Page::check_write_race(const Slab& node, Aid t, const smemory::VectorClock& thread_vc,
                                            const smemory::VectorClockPool& pool) const
{
  if (node.write_.get_aid() != t &&
      node.write_.get_clock() > thread_vc.clocks[node.write_.get_aid().value()].get_clock()) [[unlikely]] {
    throw DataRaceException("FastTrack: Write DataRace detected (Write-Write conflict)");
  }

  if (node.reads_.is_pure_epoch()) {
    if (node.reads_.get_aid() != t &&
        node.reads_.get_clock() > thread_vc.clocks[node.reads_.get_aid().value()].get_clock()) [[unlikely]] {
      throw DataRaceException("FastTrack: Write DataRace detected (Read-Write conflict on Exclusive Read)");
    }
  } else {
    if (not pool.is_past(node.reads_.get_pool_index(), thread_vc)) [[unlikely]] {
      throw DataRaceException("FastTrack: Write DataRace detected (Read-Write conflict on Shared Read)");
    }
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
  // vector
  auto push_and_coalesce = [&](const Slab& inv) {
    if (out_of_place) {
      if (not scratchpad.empty() && scratchpad.back().end_ == inv.start_ && scratchpad.back().has_same_epochs(inv))
        scratchpad.back().end_ = inv.end_;
      else
        scratchpad.push_back(inv);

    } else {
      if (write_idx > 0 && slabs_[write_idx - 1].end_ == inv.start_ && slabs_[write_idx - 1].has_same_epochs(inv)) {
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
        check_write_race(overlap, t, thread_vc, pool);
        overlap.write_ = current_epoch;
        overlap.reads_ = Epoch(Aid{0}, Clock{0}); // Wipe reads on successful write
      } else {
        check_read_race(overlap, t, thread_vc);
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

/* ********************** Container implementation ********************** */
/* ********************************************************************** */

void SlabTrackState::apply_transition(const smemory::MemoryAccessTrace& record, Aid t, Clock c,
                                      const smemory::VectorClock& thread_vc)
{
  std::vector<Slab> scratchpad;

  // For each page of the MemoryAccessTrace, merge it to the corresponding page of the SlabTrackState
  for (const auto& access_it : record) {

    Page& page = get_or_create_page(access_it.page_base_addr);

    page.apply_batch_operations(access_it, t, c, thread_vc, pool_, scratchpad);
  }
}

} // namespace simgrid::mc::smemory