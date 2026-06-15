/* Copyright (c) 2026-2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/3rd-party/catch.hpp"
#include <cstdint>
#include <cstring>
#include <vector>

#define private public
#include "src/mc/smemory/MemoryAccessTrace.hpp"
#include "src/mc/smemory/SlabTrackState.hpp"
#include "src/mc/smemory/VectorClockPool.hpp"

using namespace simgrid::mc;
using namespace simgrid::mc::smemory;

// ============================================================================
// Test Utilities
// ============================================================================

// Creates a basic vector clock for testing
static VectorClock make_vc(uint32_t my_aid, uint32_t my_clock)
{
  VectorClock vc;
  vc.clocks[my_aid] = Epoch(Aid{my_aid}, Clock{my_clock});
  return vc;
}

// Creates a synchronized vector clock (T2 knows T1's history)
static VectorClock make_sync_vc(uint32_t my_aid, uint32_t my_clock, uint32_t other_aid, uint32_t other_clock)
{
  VectorClock vc;
  vc.clocks[my_aid]    = Epoch(Aid{my_aid}, Clock{my_clock});
  vc.clocks[other_aid] = Epoch(Aid{other_aid}, Clock{other_clock});
  return vc;
}

// Aligned memory to simulate a page
alignas(4096) static char memory_page[4096];
const auto memory_page_addr = reinterpret_cast<uintptr_t>(memory_page);

// ============================================================================
// Fusion Algorithm Tests (In-Place & Out-Of-Place)
// ============================================================================

TEST_CASE("SlabTrackState: Initial state", "[SlabTrackState]")
{
  Epoch init_epoch(Aid{1}, Clock{0});
  SlabTrackState state(init_epoch);

  SlabTrackState::Page& page = state.get_or_create_page(reinterpret_cast<uintptr_t>(memory_page));
  const auto& intervals      = page.slabs_;

  REQUIRE(intervals.size() == 1);
  REQUIRE(intervals[0].start_ == 0);
  REQUIRE(intervals[0].end_ == 4096);
}
TEST_CASE("SlabTrackState: First write triggers scratchpad/out-of-place", "[SlabTrackState]")
{
  Epoch init_epoch(Aid{1}, Clock{0});
  SlabTrackState state(init_epoch);
  MemoryAccessTrace tracker;

  tracker.create_memory_access(MemOpType::Write, memory_page_addr + 1024, 16);
  state.apply_transition(tracker, Aid{2}, Clock{1}, make_vc(2, 1));

  SlabTrackState::Page& page = state.get_or_create_page(reinterpret_cast<uintptr_t>(memory_page));
  const auto& intervals      = page.slabs_;

  uint16_t expected_end =
      MemoryAccessTrace::is_coalescing() ? (1024 + 16) : (1024 + MemoryAccessTrace::get_bucket_size());

  REQUIRE(intervals.size() == 3);
  REQUIRE(intervals[1].start_ == 1024);
  REQUIRE(intervals[1].end_ == expected_end);
  REQUIRE(intervals[1].write_ == Epoch(Aid{2}, Clock{1}));
  REQUIRE(intervals[2].start_ == expected_end);
}

TEST_CASE("SlabTrackState: Contiguous writes coalesce", "[SlabTrackState]")
{
  SlabTrackState state(Epoch(Aid{1}, Clock{0}));

  // Write 1
  MemoryAccessTrace tracker1;
  tracker1.create_memory_access(MemOpType::Write, memory_page_addr + 100, 16);
  state.apply_transition(tracker1, Aid{2}, Clock{1}, make_vc(2, 1));

  // Adjacently write
  MemoryAccessTrace tracker2;
  tracker2.create_memory_access(MemOpType::Write, memory_page_addr + 116, 16);
  state.apply_transition(tracker2, Aid{2}, Clock{1}, make_vc(2, 1));

  SlabTrackState::Page& page = state.get_or_create_page(reinterpret_cast<uintptr_t>(memory_page));
  const auto& intervals      = page.slabs_;

  if (MemoryAccessTrace::is_coalescing()) { // SlabTrack mode
    REQUIRE(intervals.size() == 3);
    REQUIRE(intervals[1].start_ == 100);
    REQUIRE(intervals[1].end_ == 132); // 100 + 16 + 16
  } else {                             // orignal FastTrack mode: isolated writes
    uint16_t bucket_size = MemoryAccessTrace::get_bucket_size();

    // Page: [0-100), [100-101), [101-116), [116-117), [117-4096)
    REQUIRE(intervals.size() == 5);

    REQUIRE(intervals[1].start_ == 100);
    REQUIRE(intervals[1].end_ == 100 + bucket_size);
    REQUIRE(intervals[1].write_ == Epoch(Aid{2}, Clock{1}));

    REQUIRE(intervals[3].start_ == 116);
    REQUIRE(intervals[3].end_ == 116 + bucket_size);
    REQUIRE(intervals[3].write_ == Epoch(Aid{2}, Clock{1}));
  }
}
// ============================================================================
// Data Race Detection Tests
// ============================================================================

TEST_CASE("SlabTrackState: Write-Write Data Race detection", "[SlabTrackState]")
{
  SlabTrackState state(Epoch(Aid{1}, Clock{0}));

  MemoryAccessTrace tracker1;
  tracker1.create_memory_access(MemOpType::Write, memory_page_addr + 100, 16);
  state.apply_transition(tracker1, Aid{2}, Clock{1}, make_vc(2, 1));

  MemoryAccessTrace tracker2;
  tracker2.create_memory_access(MemOpType::Write, memory_page_addr + 100, 4);

  // Works for both FastTrack and SlabTrack as both access are on the exact same address
  REQUIRE_THROWS_AS(state.apply_transition(tracker2, Aid{3}, Clock{1}, make_vc(3, 1)), DataRaceException);
}

TEST_CASE("SlabTrackState: Synchronization prevents race", "[SlabTrackState]")
{
  SlabTrackState state(Epoch(Aid{1}, Clock{0}));

  MemoryAccessTrace tracker1;
  tracker1.create_memory_access(MemOpType::Write, memory_page_addr + 500, 16);
  state.apply_transition(tracker1, Aid{2}, Clock{1}, make_vc(2, 1));

  MemoryAccessTrace tracker2;
  tracker2.create_memory_access(MemOpType::Read, memory_page_addr + 500, 16);

  // No exception expected due to synchronized VectorClock
  REQUIRE_NOTHROW(state.apply_transition(tracker2, Aid{3}, Clock{1}, make_sync_vc(3, 1, 2, 1)));
}
// ============================================================================
// Test Utilities (additional helpers)
// ============================================================================

// Creates a VectorClock where thread `t` is at clock `c` and knows the
// history of an arbitrary number of other threads (variadic)
static VectorClock make_full_sync_vc(std::initializer_list<std::pair<uint32_t, uint32_t>> entries)
{
  VectorClock vc;
  for (auto [aid, clk] : entries)
    vc.clocks[aid] = Epoch(Aid{aid}, Clock{clk});
  return vc;
}

// Finds the Slab covering `offset` in a page. Asserts it exists.
static const SlabTrackState::Slab& find_interval(const SlabTrackState::Page& page, uint16_t offset)
{
  for (const auto& inv : page.slabs_)
    if (inv.start_ <= offset && offset < inv.end_)
      return inv;
  FAIL("No interval covers offset " << offset);
  // unreachable but required for compilation
  return page.slabs_[0];
}

// ============================================================================
// Write-Read Race Detection Tests
// ============================================================================

TEST_CASE("SlabTrackState: Write-Read race detection (unsynced read after write)", "[SlabTrackState]")
{
  SlabTrackState state(Epoch(Aid{1}, Clock{0}));

  // T1 writes
  MemoryAccessTrace tracker1;
  tracker1.create_memory_access(MemOpType::Write, memory_page_addr + 300, 8);
  state.apply_transition(tracker1, Aid{2}, Clock{1}, make_vc(2, 1));

  // T2 reads the same location without knowing about T1's write
  MemoryAccessTrace tracker2;
  tracker2.create_memory_access(MemOpType::Read, memory_page_addr + 300, 8);

  REQUIRE_THROWS_AS(state.apply_transition(tracker2, Aid{3}, Clock{1}, make_vc(3, 1)), DataRaceException);
}

TEST_CASE("SlabTrackState: Write-Read race is partial-overlap-sensitive in SlabTrack", "[SlabTrackState]")
{
  SlabTrackState state(Epoch(Aid{1}, Clock{0}));

  // T1 writes a large range
  MemoryAccessTrace tracker1;
  tracker1.create_memory_access(MemOpType::Write, memory_page_addr + 100, 32);
  state.apply_transition(tracker1, Aid{2}, Clock{1}, make_vc(2, 1));

  // T2 reads only the middle, without sync — still a race
  MemoryAccessTrace tracker2;
  tracker2.create_memory_access(MemOpType::Read, memory_page_addr + 112, 8);

  if (MemoryAccessTrace::is_coalescing()) {
    // SlabTrack detects the problem despite the partial overlap!
    REQUIRE_THROWS_AS(state.apply_transition(tracker2, Aid{3}, Clock{1}, make_vc(3, 1)), DataRaceException);
  } else {
    // FastTrack sees two distinct variables (100 & 112) and misses the datarace
    REQUIRE_NOTHROW(state.apply_transition(tracker2, Aid{3}, Clock{1}, make_vc(3, 1)));
  }
}

// ============================================================================
// Shared Read Path Tests
// ============================================================================

TEST_CASE("SlabTrackState: Two independent reads promote reads_ to pool index", "[SlabTrackState][SharedRead]")
{
  SlabTrackState state(Epoch(Aid{1}, Clock{0}));

  // T1 reads
  MemoryAccessTrace tracker1;
  tracker1.create_memory_access(MemOpType::Read, memory_page_addr + 200, 4);
  state.apply_transition(tracker1, Aid{2}, Clock{1}, make_vc(2, 1));

  // T2 reads the same location without synchronizing with T1
  MemoryAccessTrace tracker2;
  tracker2.create_memory_access(MemOpType::Read, memory_page_addr + 200, 4);
  state.apply_transition(tracker2, Aid{3}, Clock{1}, make_vc(3, 1));

  // The interval covering offset 200 must now hold a pool index, not a pure epoch
  SlabTrackState::Page& page = state.get_or_create_page(memory_page_addr);
  const auto& inv            = find_interval(page, 200);

  REQUIRE(inv.reads_.is_pool_index());
}

TEST_CASE("SlabTrackState: Pool index encodes both readers' clocks", "[SlabTrackState][SharedRead]")
{
  SlabTrackState state(Epoch(Aid{1}, Clock{0}));

  MemoryAccessTrace tracker1;
  tracker1.create_memory_access(MemOpType::Read, memory_page_addr + 200, 4);
  state.apply_transition(tracker1, Aid{2}, Clock{3}, make_vc(2, 3));

  MemoryAccessTrace tracker2;
  tracker2.create_memory_access(MemOpType::Read, memory_page_addr + 200, 4);
  state.apply_transition(tracker2, Aid{3}, Clock{5}, make_vc(3, 5));

  SlabTrackState::Page& page = state.get_or_create_page(memory_page_addr);
  const auto& inv            = find_interval(page, 200);

  REQUIRE(inv.reads_.is_pool_index());

  // The pool must contain a VC recording both T1@3 and T2@5
  const VectorClock& rx_vc = state.pool_.get(inv.reads_.get_pool_index());
  REQUIRE(rx_vc.clocks[2].get_clock().value() == 3);
  REQUIRE(rx_vc.clocks[3].get_clock().value() == 5);
}

TEST_CASE("SlabTrackState: Third reader merges into existing pool index", "[SlabTrackState][SharedRead]")
{
  SlabTrackState state(Epoch(Aid{1}, Clock{0}));

  MemoryAccessTrace tracker1;
  tracker1.create_memory_access(MemOpType::Read, memory_page_addr + 200, 4);
  state.apply_transition(tracker1, Aid{2}, Clock{1}, make_vc(2, 1));

  MemoryAccessTrace tracker2;
  tracker2.create_memory_access(MemOpType::Read, memory_page_addr + 200, 4);
  state.apply_transition(tracker2, Aid{3}, Clock{1}, make_vc(3, 1));

  // T3 also reads without sync — must update the existing pool index, not create garbage
  MemoryAccessTrace tracker3;
  tracker3.create_memory_access(MemOpType::Read, memory_page_addr + 200, 4);
  REQUIRE_NOTHROW(state.apply_transition(tracker3, Aid{4}, Clock{1}, make_vc(4, 1)));

  SlabTrackState::Page& page = state.get_or_create_page(memory_page_addr);
  const auto& inv            = find_interval(page, 200);

  REQUIRE(inv.reads_.is_pool_index());

  const VectorClock& rx_vc = state.pool_.get(inv.reads_.get_pool_index());
  REQUIRE(rx_vc.clocks[2].get_clock().value() == 1);
  REQUIRE(rx_vc.clocks[3].get_clock().value() == 1);
  REQUIRE(rx_vc.clocks[4].get_clock().value() == 1);
}

TEST_CASE("SlabTrackState: Write after shared read without sync throws", "[SlabTrackState][SharedRead]")
{
  SlabTrackState state(Epoch(Aid{1}, Clock{0}));

  MemoryAccessTrace tracker1;
  tracker1.create_memory_access(MemOpType::Read, memory_page_addr + 200, 4);
  state.apply_transition(tracker1, Aid{2}, Clock{1}, make_vc(2, 1));

  MemoryAccessTrace tracker2;
  tracker2.create_memory_access(MemOpType::Read, memory_page_addr + 200, 4);
  state.apply_transition(tracker2, Aid{3}, Clock{1}, make_vc(3, 1));

  // T3 writes without knowing about either reader
  MemoryAccessTrace tracker3;
  tracker3.create_memory_access(MemOpType::Write, memory_page_addr + 200, 4);

  REQUIRE_THROWS_AS(state.apply_transition(tracker3, Aid{4}, Clock{1}, make_vc(4, 1)), DataRaceException);
}

TEST_CASE("SlabTrackState: Write after shared read with full sync does not throw", "[SlabTrackState][SharedRead]")
{
  SlabTrackState state(Epoch(Aid{1}, Clock{0}));

  MemoryAccessTrace tracker1;
  tracker1.create_memory_access(MemOpType::Read, memory_page_addr + 200, 4);
  state.apply_transition(tracker1, Aid{2}, Clock{1}, make_vc(2, 1));

  MemoryAccessTrace tracker2;
  tracker2.create_memory_access(MemOpType::Read, memory_page_addr + 200, 4);
  state.apply_transition(tracker2, Aid{3}, Clock{1}, make_vc(3, 1));

  // T3 writes knowing about both T1 and T2
  MemoryAccessTrace tracker3;
  tracker3.create_memory_access(MemOpType::Write, memory_page_addr + 200, 4);

  REQUIRE_NOTHROW(state.apply_transition(tracker3, Aid{4}, Clock{1}, make_full_sync_vc({{4, 1}, {2, 1}, {3, 1}})));
}

TEST_CASE("SlabTrackState: Write after shared read with partial sync throws", "[SlabTrackState][SharedRead]")
{
  // T3 knows about T1 but not T2 — the race with T2 must still be detected
  SlabTrackState state(Epoch(Aid{1}, Clock{0}));

  MemoryAccessTrace tracker1;
  tracker1.create_memory_access(MemOpType::Read, memory_page_addr + 200, 4);
  state.apply_transition(tracker1, Aid{2}, Clock{1}, make_vc(2, 1));

  MemoryAccessTrace tracker2;
  tracker2.create_memory_access(MemOpType::Read, memory_page_addr + 200, 4);
  state.apply_transition(tracker2, Aid{3}, Clock{1}, make_vc(3, 1));

  MemoryAccessTrace tracker3;
  tracker3.create_memory_access(MemOpType::Write, memory_page_addr + 200, 4);

  // T3 synchronized only with T1, not T2
  REQUIRE_THROWS_AS(state.apply_transition(tracker3, Aid{4}, Clock{1}, make_full_sync_vc({{4, 1}, {2, 1}})),
                    DataRaceException);
}

// ============================================================================
// Interval Invariant Tests
// ============================================================================

TEST_CASE("SlabTrackState: Split preserves epochs of untouched segments", "[SlabTrackState]")
{
  // Verify that a write to the middle of a page produces exactly 3 intervals
  // and that the untouched prefix and suffix keep the initial epoch
  Epoch init_epoch(Aid{1}, Clock{0});
  SlabTrackState state(init_epoch);

  MemoryAccessTrace tracker;
  tracker.create_memory_access(MemOpType::Write, memory_page_addr + 1000, 24);
  state.apply_transition(tracker, Aid{2}, Clock{1}, make_vc(2, 1));

  SlabTrackState::Page& page = state.get_or_create_page(memory_page_addr);
  const auto& intervals      = page.slabs_;

  uint16_t expected_end =
      MemoryAccessTrace::is_coalescing() ? (1000 + 24) : (1000 + MemoryAccessTrace::get_bucket_size());

  REQUIRE(intervals.size() == 3);

  // Prefix [0, 1000) — untouched, must keep init_epoch
  REQUIRE(intervals[0].start_ == 0);
  REQUIRE(intervals[0].end_ == 1000);
  REQUIRE(intervals[0].write_ == init_epoch);

  // Written segment [1000, expected_end)
  REQUIRE(intervals[1].start_ == 1000);
  REQUIRE(intervals[1].end_ == expected_end);
  REQUIRE(intervals[1].write_ == Epoch(Aid{2}, Clock{1}));

  // Suffix [expected_end, 4096) — untouched, must keep init_epoch
  REQUIRE(intervals[2].start_ == expected_end);
  REQUIRE(intervals[2].end_ == 4096);
  REQUIRE(intervals[2].write_ == init_epoch);
}

TEST_CASE("SlabTrackState: Write-Write race is detected on partial overlap", "[SlabTrackState]")
{
  SlabTrackState state(Epoch(Aid{1}, Clock{0}));

  // T1 writes [100, 132)
  MemoryAccessTrace tracker1;
  tracker1.create_memory_access(MemOpType::Write, memory_page_addr + 100, 32);
  state.apply_transition(tracker1, Aid{2}, Clock{1}, make_vc(2, 1));

  // T2 writes [120, 152) — overlaps T1 without sync
  MemoryAccessTrace tracker2;
  tracker2.create_memory_access(MemOpType::Write, memory_page_addr + 120, 32);

  if (MemoryAccessTrace::is_coalescing()) {
    // SlabTrack detects the problem
    REQUIRE_THROWS_AS(state.apply_transition(tracker2, Aid{3}, Clock{1}, make_vc(3, 1)), DataRaceException);
  } else {
    // FastTrack sees 2 variables and misses the datarace
    REQUIRE_NOTHROW(state.apply_transition(tracker2, Aid{3}, Clock{1}, make_vc(3, 1)));
  }
}

TEST_CASE("SlabTrackState: Read on initial epoch never throws", "[SlabTrackState]")
{
  // No matter which thread reads from a freshly initialized page,
  // it must never detect a race because the initial epoch is neutral
  SlabTrackState state(Epoch(Aid{1}, Clock{0}));

  MemoryAccessTrace tracker;
  tracker.create_memory_access(MemOpType::Read, memory_page_addr + 0, 4096);

  REQUIRE_NOTHROW(state.apply_transition(tracker, Aid{2}, Clock{1}, make_vc(2, 1)));
}

TEST_CASE("SlabTrackState: Successive writes by same thread do not race", "[SlabTrackState]")
{
  // A thread always synchronizes with its own past — same-thread writes must never throw
  SlabTrackState state(Epoch(Aid{1}, Clock{0}));

  MemoryAccessTrace tracker1;
  tracker1.create_memory_access(MemOpType::Write, memory_page_addr + 400, 16);
  state.apply_transition(tracker1, Aid{2}, Clock{1}, make_vc(2, 1));

  MemoryAccessTrace tracker2;
  tracker2.create_memory_access(MemOpType::Write, memory_page_addr + 400, 16);

  REQUIRE_NOTHROW(state.apply_transition(tracker2, Aid{2}, Clock{2}, make_vc(2, 2)));
}
