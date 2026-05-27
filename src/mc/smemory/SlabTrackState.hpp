/* Copyright (c) 2026-2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* SlabTrackState is the accumulator storing all information gathered so far by our FastTrack variant. The variations to
 * the original FastTrack algorithm come from the following insights:
 *
 * # Typical memory accesses exhibit some level of locality, where adjacent memory chunks are accessed together.
 *
 * SlabTrackState::Slab is thus the equivalent of an VarState in the Flanagan'09 article, but with a location start and
 * end rather than a discrete address.
 *
 * # Memory is not a huge array of cells for the OS, but a set of continuous pages with unaddressable holes between them
 *
 * Each SlabTrackState::Page gathers info about a given page while SlabTrackState maps the any location to the right
 * page. This layered data structure leaves room for algorithmic optimization to leverage memory access locality that
 * most application exhibit.
 *
 * # The memory accesses are observed in the App process, while our FastTrack algorithm lives in the Checker.
 *
 * Data is collected in MemoryAccessTrace during a transition of the system. Two bit fields are used (one for read and
 * one for write) to prune dupplicate accesses and aggregate adjacent accesses into a single one. In addition, if a
 * given location is accessed both in read and write mode, only the write is retained. The read access is not relevant
 * to FastTrack anyway (the [FT WRITE] rule of the original article resets Rx to the bottom value).
 *
 * Another optimization is that several adjacent bytes of the observed memory can be compacted into a single bit of each
 * bit fields. If  MemoryAccessTrace::granularity == 32 for example, we save only one bit pair for 32 observed bytes.
 * This can induce false sharings, but it greatly decrease the memory compaction (this optimization naturally propagates
 * to the SlabTrackState). granularity == sizeof(int) is probably a good compromise.
 *
 * Finally, the  MemoryAccessTrace data structure is also layered as a sparse collection of continous pages, helping
 * with memory locality when integrating the  MemoryAccessTrace of a given system transition into the current
 * SlabTrackState.
 *
 * The code is highly optimized. The first optimization is about memory compaction. The original FastTrack maintains 3
 * fields per Slab: a write epoch, a read epoch and a read vector clock (VC). The read epoch is used only for private
 * reads while the read vector clock is used only for shared reads, so these values are never used simultaneously. In
 * addition, VCs are rather large and often similar between locations, but only used in 20% of the read operations
 * (according to Flanagan's paper). As a solution, VCs are interned in an external data structure called
 * VectorClockPool, i.e. VectorClockPool stores each VC only once in big vector, and the index to that cell is stored in
 * the Slab instead of the full structure.
 *
 * Each Slab keeps only two epochs, each packed into a uint32_t. For shared reads, the reads_ "epoch" stores the VC
 * index in the VectorClockPool container instead. One bit of the read epoch is a selector:
 *
 *   - if the selector bit is 0, then the data is a real Epoch with 5 bits for the process ID (up to 32 processes) and
 * 26 bits for the clock (over 67M transitions, which is more than enough)
 *   - if the selector bit is 1, then the other 31 bits are an index that uniquely represent a given VectorClock in the
 *     VectorClockPool (which boils down to a vector of VectorClock).
 *
 * One could argue that 32 processes is too few while 67M transitions is too much, but this setting (which can be
 * adjusted through the max_threads constant) ensure that each VC is only 128 bytes (one Epoch per thread, i.e. one
 * uint32 each). This enables the use of superfast AVX2 operations on these vector clocks. The VC stores Epochs rather
 * than Clocks even if the process ID is already known from the position in the VC vector, but this small data redudancy
 * greatly reduces the amount of data conversion along the path.
 */

#ifndef SIMGRID_MC_SHADOW_INTERVAL_MAP_HPP
#define SIMGRID_MC_SHADOW_INTERVAL_MAP_HPP

#include "src/mc/smemory/MemoryAccessTrace.hpp"
#include "src/mc/smemory/VectorClockPool.hpp"

#include <unordered_map>

namespace simgrid::mc::smemory {

class DataRaceException : public std::runtime_error {
public:
  explicit DataRaceException(const char* msg) : std::runtime_error(msg) {}
};

class SlabTrackState {
public:
  struct Slab {
    Epoch write_; /* Epoch of the last write on that interval. This must be a pure epoch in any case. */
    Epoch reads_; /* Information about the last read(s) on that interval. This can be a pure epoch for private reads,
                     but can be an index to a VectorClockPool cell in case of shared reads. */

    /* start and end offsets of the concerned interval of observed memory */
    uint16_t start_;
    uint16_t end_;

    constexpr Slab(uint16_t start, uint16_t end, Epoch w, Epoch r) noexcept
        : write_(w), reads_(r), start_(start), end_(end)
    {
    }

    constexpr bool has_same_epochs(const Slab& other) const noexcept
    {
      return write_ == other.write_ && reads_ == other.reads_;
    }
    constexpr bool operator==(const Slab& rhs) const noexcept = default; // Default comparison does so on each element
    constexpr bool operator<(const Slab& rhs) const noexcept             // Comparing the beginning is enough
    {
      return start_ < rhs.start_;
    }
  };

  class Page {
  private:
    std::vector<Slab> slabs_;

    // Isolated FastTrack rules to keep the merge algorithm readable
    void check_read_race(const Slab& node, Aid t, const smemory::VectorClock& thread_vc) const;
    void check_write_race(const Slab& node, Aid t, const smemory::VectorClock& thread_vc,
                          const smemory::VectorClockPool& pool) const;

    void update_read_epoch(Slab& node, Aid t, Clock c, const smemory::VectorClock& thread_vc,
                           smemory::VectorClockPool& pool) const;

  public:
    /** @brief Applies batch memory operations on a page.
     *
     * This method tries to merge the operations in-place, in the already existing data structure. If it would overwrite
     * a segment that is not read yet, it switches to an out-of-place algorithm using a scratch vector provided
     * elsewhere (to keep it between pages and reduce the amount of mallocs). The out-of-place data is swapped with the
     * page data on need.
     */
    void apply_batch_operations(const smemory::MemoryAccessTrace::PageAccessView& accesses, Aid t, Clock c,
                                const smemory::VectorClock& thread_vc, smemory::VectorClockPool& pool,
                                std::vector<Slab>& scratchpad);

    Page(Epoch init_W, Epoch init_R)
    {
      slabs_.emplace_back(0, static_cast<uint16_t>(smemory::config::page_size), init_W, init_R);
    }
  };

private:
  std::unordered_map<uintptr_t, Page> pages_;
  smemory::VectorClockPool pool_; // Where the VC are interned
  Epoch initial_epoch_;

  Page& get_or_create_page(uintptr_t page_base)
  {
    auto it = pages_.find(page_base);
    if (it == pages_.end()) {
      it = pages_.emplace(page_base, Page(initial_epoch_, initial_epoch_)).first;
    }
    return it->second;
  }

public:
  SlabTrackState(Epoch init_epoch) : initial_epoch_(init_epoch) {}

  // Apply every accesses contained in the record to the current SlabTrackState, detecting the dataraces as we go
  void apply_transition(const smemory::MemoryAccessTrace& tracker, Aid t, Clock c,
                        const smemory::VectorClock& thread_vc);
};

} // namespace simgrid::mc::smemory
#endif