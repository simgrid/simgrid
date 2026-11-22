/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Unit tests of SlabTrackState, the data race detector of smemory. Run them with `unit-tests "[SlabTrackState]"`.
 *
 * Hand-written scenarios, each checking one rule on a few accesses:
 *  - Slab management: initial state of a page, split of slabs around a new access (in place or through the
 *    scratchpad), coalescing of adjacent accesses, untouched segments keeping their epochs.
 *  - Race detection: write-write, write-read and read-write races (on full or partial overlaps), absence of race
 *    when the accesses are synchronized or done by the same actor, content of the reported McDataRace.
 *  - Shared reads (tag [SharedRead]): concurrent reads turn the read epoch into a VectorClockPool index, later readers
 *    are merged into it, and a write must know every reader (full, partial or no synchronization).
 *  - reset() forgets every access.
 *  - Garbage collection (tag [GC]): clean_before() forgets the accesses in the past of global_min, merges the slabs,
 *    drops the empty pages and frees the VCs, while keeping the accesses that can still race.
 *
 * Exhaustive comparison with a naive detector (tag [exhaustive]):
 *  - Every scenario of 3 steps with up to 3 actors is run, skipping the scenarios that only differ by the naming of the
 *    actors or by synchronizations with actors that did not run yet (3672 scenarios). Each step is a read or a write
 *    on [0,4), [2,6) or [8,12), possibly after the last step of another actor.
 *  - The reference is a naive detector written in this file. It keeps every access to every byte and applies the
 *    definition of a race: different actors, at least one write, and the earlier access does not happen before the
 *    current one. It works on the ranges recorded by MemoryAccessTrace, so it does not depend on the coalescing mode.
 *  - At each step, both detectors must agree on the presence of a race. When there is one, the second access of the
 *    report must be the current one, and the first access must race on every byte of the reported range. The scenario
 *    stops at the first race, as FastTrack only guarantees the detection of the first race.
 *  - Each scenario runs twice: once without garbage collection, and once with clean_before(global_min) after each step,
 *    where global_min is the minimum of the vector clocks of the remaining steps (valid by construction).
 *  - On failure, the test prints the scenario, e.g. "Without clean_before: T1 R[0,4); T2 R[0,4); T2 W[0,4); SlabTrack
 *    misses the race of step 3". It can then be turned into a hand-written scenario.
 *  - Not covered: granularity above 1 byte, the instrumentation, and the computation of the vector clocks by the
 *    explorers.
 */

#include "src/3rd-party/catch.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
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
  SlabTrackState state;

  SlabTrackState::Page& page = state.get_or_create_page(reinterpret_cast<uintptr_t>(memory_page));
  const auto& intervals      = page.slabs_;

  REQUIRE(intervals.size() == 1);
  REQUIRE(intervals[0].start_ == 0);
  REQUIRE(intervals[0].end_ == 4096);
}
TEST_CASE("SlabTrackState: First write triggers scratchpad/out-of-place", "[SlabTrackState]")
{
  SlabTrackState state;
  MemoryAccessTrace tracker;

  tracker.create_memory_access(MemOpType::Write, memory_page_addr + 1024, 16);
  state.apply_transition(tracker, Aid{2}, make_vc(2, 1));

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
  SlabTrackState state;

  // Write 1
  MemoryAccessTrace tracker1;
  tracker1.create_memory_access(MemOpType::Write, memory_page_addr + 100, 16);
  state.apply_transition(tracker1, Aid{2}, make_vc(2, 1));

  // Adjacently write
  MemoryAccessTrace tracker2;
  tracker2.create_memory_access(MemOpType::Write, memory_page_addr + 116, 16);
  state.apply_transition(tracker2, Aid{2}, make_vc(2, 1));

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
  SlabTrackState state;

  MemoryAccessTrace tracker1;
  tracker1.create_memory_access(MemOpType::Write, memory_page_addr + 100, 16);
  state.apply_transition(tracker1, Aid{2}, make_vc(2, 1));

  MemoryAccessTrace tracker2;
  tracker2.create_memory_access(MemOpType::Write, memory_page_addr + 100, 4);

  // Works for both FastTrack and SlabTrack as both access are on the exact same address
  try {
    state.apply_transition(tracker2, Aid{3}, make_vc(3, 1));
    FAIL("The race was not detected");
  } catch (const McDataRace& race) {
    REQUIRE(race.first_mem_op_ == Epoch(Aid{2}, Clock{1}));
    REQUIRE(race.first_mem_type_ == MemOpType::Write);
    REQUIRE(race.second_mem_op_ == Epoch(Aid{3}, Clock{1}));
    REQUIRE(race.second_mem_type_ == MemOpType::Write);
    REQUIRE(race.location_ == memory_page_addr + 100);
    REQUIRE(race.sizes_[0] == (MemoryAccessTrace::is_coalescing() ? 4 : MemoryAccessTrace::get_bucket_size()));
  }
}

TEST_CASE("SlabTrackState: Synchronization prevents race", "[SlabTrackState]")
{
  SlabTrackState state;

  MemoryAccessTrace tracker1;
  tracker1.create_memory_access(MemOpType::Write, memory_page_addr + 500, 16);
  state.apply_transition(tracker1, Aid{2}, make_vc(2, 1));

  MemoryAccessTrace tracker2;
  tracker2.create_memory_access(MemOpType::Read, memory_page_addr + 500, 16);

  // No exception expected due to synchronized VectorClock
  REQUIRE_NOTHROW(state.apply_transition(tracker2, Aid{3}, make_sync_vc(3, 1, 2, 1)));
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
  SlabTrackState state;

  // T1 writes
  MemoryAccessTrace tracker1;
  tracker1.create_memory_access(MemOpType::Write, memory_page_addr + 300, 8);
  state.apply_transition(tracker1, Aid{2}, make_vc(2, 1));

  // T2 reads the same location without knowing about T1's write
  MemoryAccessTrace tracker2;
  tracker2.create_memory_access(MemOpType::Read, memory_page_addr + 300, 8);

  REQUIRE_THROWS_AS(state.apply_transition(tracker2, Aid{3}, make_vc(3, 1)), McDataRace);
}

TEST_CASE("SlabTrackState: Write-Read race is partial-overlap-sensitive in SlabTrack", "[SlabTrackState]")
{
  SlabTrackState state;

  // T1 writes a large range
  MemoryAccessTrace tracker1;
  tracker1.create_memory_access(MemOpType::Write, memory_page_addr + 100, 32);
  state.apply_transition(tracker1, Aid{2}, make_vc(2, 1));

  // T2 reads only the middle, without sync — still a race
  MemoryAccessTrace tracker2;
  tracker2.create_memory_access(MemOpType::Read, memory_page_addr + 112, 8);

  if (MemoryAccessTrace::is_coalescing()) {
    // SlabTrack detects the problem despite the partial overlap!
    REQUIRE_THROWS_AS(state.apply_transition(tracker2, Aid{3}, make_vc(3, 1)), McDataRace);
  } else {
    // FastTrack sees two distinct variables (100 & 112) and misses the datarace
    REQUIRE_NOTHROW(state.apply_transition(tracker2, Aid{3}, make_vc(3, 1)));
  }
}

// ============================================================================
// Shared Read Path Tests
// ============================================================================

TEST_CASE("SlabTrackState: Two independent reads promote reads_ to pool index", "[SlabTrackState][SharedRead]")
{
  SlabTrackState state;

  // T1 reads
  MemoryAccessTrace tracker1;
  tracker1.create_memory_access(MemOpType::Read, memory_page_addr + 200, 4);
  state.apply_transition(tracker1, Aid{2}, make_vc(2, 1));

  // T2 reads the same location without synchronizing with T1
  MemoryAccessTrace tracker2;
  tracker2.create_memory_access(MemOpType::Read, memory_page_addr + 200, 4);
  state.apply_transition(tracker2, Aid{3}, make_vc(3, 1));

  // The interval covering offset 200 must now hold a pool index, not a pure epoch
  SlabTrackState::Page& page = state.get_or_create_page(memory_page_addr);
  const auto& inv            = find_interval(page, 200);

  REQUIRE(inv.reads_.is_pool_index());
}

TEST_CASE("SlabTrackState: Pool index encodes both readers' clocks", "[SlabTrackState][SharedRead]")
{
  SlabTrackState state;

  MemoryAccessTrace tracker1;
  tracker1.create_memory_access(MemOpType::Read, memory_page_addr + 200, 4);
  state.apply_transition(tracker1, Aid{2}, make_vc(2, 3));

  MemoryAccessTrace tracker2;
  tracker2.create_memory_access(MemOpType::Read, memory_page_addr + 200, 4);
  state.apply_transition(tracker2, Aid{3}, make_vc(3, 5));

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
  SlabTrackState state;

  MemoryAccessTrace tracker1;
  tracker1.create_memory_access(MemOpType::Read, memory_page_addr + 200, 4);
  state.apply_transition(tracker1, Aid{2}, make_vc(2, 1));

  MemoryAccessTrace tracker2;
  tracker2.create_memory_access(MemOpType::Read, memory_page_addr + 200, 4);
  state.apply_transition(tracker2, Aid{3}, make_vc(3, 1));

  // T3 also reads without sync — must update the existing pool index, not create garbage
  MemoryAccessTrace tracker3;
  tracker3.create_memory_access(MemOpType::Read, memory_page_addr + 200, 4);
  REQUIRE_NOTHROW(state.apply_transition(tracker3, Aid{4}, make_vc(4, 1)));

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
  SlabTrackState state;

  MemoryAccessTrace tracker1;
  tracker1.create_memory_access(MemOpType::Read, memory_page_addr + 200, 4);
  state.apply_transition(tracker1, Aid{2}, make_vc(2, 1));

  MemoryAccessTrace tracker2;
  tracker2.create_memory_access(MemOpType::Read, memory_page_addr + 200, 4);
  state.apply_transition(tracker2, Aid{3}, make_vc(3, 1));

  // T3 writes without knowing about either reader
  MemoryAccessTrace tracker3;
  tracker3.create_memory_access(MemOpType::Write, memory_page_addr + 200, 4);

  REQUIRE_THROWS_AS(state.apply_transition(tracker3, Aid{4}, make_vc(4, 1)), McDataRace);
}

TEST_CASE("SlabTrackState: Write after shared read with full sync does not throw", "[SlabTrackState][SharedRead]")
{
  SlabTrackState state;

  MemoryAccessTrace tracker1;
  tracker1.create_memory_access(MemOpType::Read, memory_page_addr + 200, 4);
  state.apply_transition(tracker1, Aid{2}, make_vc(2, 1));

  MemoryAccessTrace tracker2;
  tracker2.create_memory_access(MemOpType::Read, memory_page_addr + 200, 4);
  state.apply_transition(tracker2, Aid{3}, make_vc(3, 1));

  // T3 writes knowing about both T1 and T2
  MemoryAccessTrace tracker3;
  tracker3.create_memory_access(MemOpType::Write, memory_page_addr + 200, 4);

  REQUIRE_NOTHROW(state.apply_transition(tracker3, Aid{4}, make_full_sync_vc({{4, 1}, {2, 1}, {3, 1}})));
}

TEST_CASE("SlabTrackState: Write after shared read with partial sync throws", "[SlabTrackState][SharedRead]")
{
  // T3 knows about T1 but not T2 — the race with T2 must still be detected
  SlabTrackState state;

  MemoryAccessTrace tracker1;
  tracker1.create_memory_access(MemOpType::Read, memory_page_addr + 200, 4);
  state.apply_transition(tracker1, Aid{2}, make_vc(2, 1));

  MemoryAccessTrace tracker2;
  tracker2.create_memory_access(MemOpType::Read, memory_page_addr + 200, 4);
  state.apply_transition(tracker2, Aid{3}, make_vc(3, 1));

  MemoryAccessTrace tracker3;
  tracker3.create_memory_access(MemOpType::Write, memory_page_addr + 200, 4);

  // T3 synchronized only with T1, not T2
  try {
    state.apply_transition(tracker3, Aid{4}, make_full_sync_vc({{4, 1}, {2, 1}}));
    FAIL("The race was not detected");
  } catch (const McDataRace& race) {
    REQUIRE(race.first_mem_op_ == Epoch(Aid{3}, Clock{1})); // The reader that T4 does not know
    REQUIRE(race.first_mem_type_ == MemOpType::Read);
    REQUIRE(race.second_mem_op_ == Epoch(Aid{4}, Clock{1}));
    REQUIRE(race.second_mem_type_ == MemOpType::Write);
    REQUIRE(race.location_ == memory_page_addr + 200);
  }
}

TEST_CASE("SlabTrackState: reset forgets every access", "[SlabTrackState]")
{
  SlabTrackState state;

  MemoryAccessTrace tracker1;
  tracker1.create_memory_access(MemOpType::Read, memory_page_addr + 200, 4);
  state.apply_transition(tracker1, Aid{2}, make_vc(2, 1));
  MemoryAccessTrace tracker2;
  tracker2.create_memory_access(MemOpType::Read, memory_page_addr + 200, 4);
  state.apply_transition(tracker2, Aid{3}, make_vc(3, 1));
  REQUIRE(state.pool_.size() == 2);

  state.reset();
  REQUIRE(state.pages_.empty());
  REQUIRE(state.pool_.size() == 1);

  // Without the reset, this write would race with both reads
  MemoryAccessTrace tracker3;
  tracker3.create_memory_access(MemOpType::Write, memory_page_addr + 200, 4);
  REQUIRE_NOTHROW(state.apply_transition(tracker3, Aid{4}, make_vc(4, 1)));
}

// ============================================================================
// Interval Invariant Tests
// ============================================================================

TEST_CASE("SlabTrackState: Split preserves epochs of untouched segments", "[SlabTrackState]")
{
  // Verify that a write to the middle of a page produces exactly 3 intervals
  // and that the untouched prefix and suffix keep BOTTOM
  SlabTrackState state;

  MemoryAccessTrace tracker;
  tracker.create_memory_access(MemOpType::Write, memory_page_addr + 1000, 24);
  state.apply_transition(tracker, Aid{2}, make_vc(2, 1));

  SlabTrackState::Page& page = state.get_or_create_page(memory_page_addr);
  const auto& intervals      = page.slabs_;

  uint16_t expected_end =
      MemoryAccessTrace::is_coalescing() ? (1000 + 24) : (1000 + MemoryAccessTrace::get_bucket_size());

  REQUIRE(intervals.size() == 3);

  // Prefix [0, 1000) — untouched, must keep BOTTOM
  REQUIRE(intervals[0].start_ == 0);
  REQUIRE(intervals[0].end_ == 1000);
  REQUIRE(intervals[0].write_ == SlabTrackState::BOTTOM);

  // Written segment [1000, expected_end)
  REQUIRE(intervals[1].start_ == 1000);
  REQUIRE(intervals[1].end_ == expected_end);
  REQUIRE(intervals[1].write_ == Epoch(Aid{2}, Clock{1}));

  // Suffix [expected_end, 4096) — untouched, must keep BOTTOM
  REQUIRE(intervals[2].start_ == expected_end);
  REQUIRE(intervals[2].end_ == 4096);
  REQUIRE(intervals[2].write_ == SlabTrackState::BOTTOM);
}

TEST_CASE("SlabTrackState: Write-Write race is detected on partial overlap", "[SlabTrackState]")
{
  SlabTrackState state;

  // T1 writes [100, 132)
  MemoryAccessTrace tracker1;
  tracker1.create_memory_access(MemOpType::Write, memory_page_addr + 100, 32);
  state.apply_transition(tracker1, Aid{2}, make_vc(2, 1));

  // T2 writes [120, 152) — overlaps T1 without sync
  MemoryAccessTrace tracker2;
  tracker2.create_memory_access(MemOpType::Write, memory_page_addr + 120, 32);

  if (MemoryAccessTrace::is_coalescing()) {
    // SlabTrack detects the problem
    REQUIRE_THROWS_AS(state.apply_transition(tracker2, Aid{3}, make_vc(3, 1)), McDataRace);
  } else {
    // FastTrack sees 2 variables and misses the datarace
    REQUIRE_NOTHROW(state.apply_transition(tracker2, Aid{3}, make_vc(3, 1)));
  }
}

TEST_CASE("SlabTrackState: Read on initial epoch never throws", "[SlabTrackState]")
{
  // No matter which thread reads from a freshly initialized page,
  // it must never detect a race because the initial epoch is neutral
  SlabTrackState state;

  MemoryAccessTrace tracker;
  tracker.create_memory_access(MemOpType::Read, memory_page_addr + 0, 4096);

  REQUIRE_NOTHROW(state.apply_transition(tracker, Aid{2}, make_vc(2, 1)));
}

TEST_CASE("SlabTrackState: Successive writes by same thread do not race", "[SlabTrackState]")
{
  // A thread always synchronizes with its own past — same-thread writes must never throw
  SlabTrackState state;

  MemoryAccessTrace tracker1;
  tracker1.create_memory_access(MemOpType::Write, memory_page_addr + 400, 16);
  state.apply_transition(tracker1, Aid{2}, make_vc(2, 1));

  MemoryAccessTrace tracker2;
  tracker2.create_memory_access(MemOpType::Write, memory_page_addr + 400, 16);

  REQUIRE_NOTHROW(state.apply_transition(tracker2, Aid{2}, make_vc(2, 2)));
}

// ============================================================================
// Garbage Collection Tests
// ============================================================================

TEST_CASE("SlabTrackState: clean_before forgets the past accesses only", "[SlabTrackState][GC]")
{
  SlabTrackState state;

  // T2 writes [100, 104), then T3 and T4 read it concurrently, creating a shared read VC
  MemoryAccessTrace write2;
  write2.create_memory_access(MemOpType::Write, memory_page_addr + 100, 4);
  state.apply_transition(write2, Aid{2}, make_vc(2, 1));
  MemoryAccessTrace read3;
  read3.create_memory_access(MemOpType::Read, memory_page_addr + 100, 4);
  state.apply_transition(read3, Aid{3}, make_sync_vc(3, 1, 2, 1));
  MemoryAccessTrace read4;
  read4.create_memory_access(MemOpType::Read, memory_page_addr + 100, 4);
  state.apply_transition(read4, Aid{4}, make_sync_vc(4, 1, 2, 1));
  REQUIRE(state.pool_.size() == 2); // The neutral VC, and the one of the shared read

  // T5 writes [2000, 2004)
  MemoryAccessTrace write5;
  write5.create_memory_access(MemOpType::Write, memory_page_addr + 2000, 4);
  state.apply_transition(write5, Aid{5}, make_vc(5, 1));

  SECTION("When every access is in the past, the page and the VC are freed")
  {
    state.clean_before(make_full_sync_vc({{2, 1}, {3, 1}, {4, 1}, {5, 1}}));
    REQUIRE(state.pages_.empty());
    REQUIRE(state.pool_.free_list_.size() == 1);

    // A new shared read reuses the freed VC index
    MemoryAccessTrace read6;
    read6.create_memory_access(MemOpType::Read, memory_page_addr + 3000, 4);
    state.apply_transition(read6, Aid{6}, make_vc(6, 1));
    MemoryAccessTrace read7;
    read7.create_memory_access(MemOpType::Read, memory_page_addr + 3000, 4);
    state.apply_transition(read7, Aid{7}, make_vc(7, 1));
    REQUIRE(state.pool_.free_list_.empty());
  }

  SECTION("The accesses that are not in the past are kept")
  {
    state.clean_before(make_full_sync_vc({{2, 1}, {3, 1}, {4, 1}}));
    REQUIRE(state.pool_.free_list_.size() == 1);

    const auto& slabs = state.get_or_create_page(memory_page_addr).slabs_;
    REQUIRE(slabs.size() == 3); // [0, 2000) and the end of the page are back to BOTTOM, and merged
    REQUIRE(slabs[0].start_ == 0);
    REQUIRE(slabs[0].write_ == SlabTrackState::BOTTOM);
    REQUIRE(slabs[0].reads_ == SlabTrackState::BOTTOM);
    REQUIRE(slabs[1].start_ == 2000);
    REQUIRE(slabs[1].write_ == Epoch(Aid{5}, Clock{1}));

    // T6 knows global_min but not the write of T5
    MemoryAccessTrace write6;
    write6.create_memory_access(MemOpType::Write, memory_page_addr + 100, 4);
    REQUIRE_NOTHROW(state.apply_transition(write6, Aid{6}, make_full_sync_vc({{6, 1}, {2, 1}, {3, 1}, {4, 1}})));
    MemoryAccessTrace write6b;
    write6b.create_memory_access(MemOpType::Write, memory_page_addr + 2000, 4);
    REQUIRE_THROWS_AS(state.apply_transition(write6b, Aid{6}, make_full_sync_vc({{6, 2}, {2, 1}, {3, 1}, {4, 1}})),
                      McDataRace);
  }
}

// ============================================================================
// Exhaustive comparison with a naive detector
// ============================================================================

namespace {
// One step of a scenario: a single access by an actor, possibly after synchronizing with the last step of another actor
struct Step {
  unsigned actor; // 0, 1 or 2, whose aid is actor + 1
  unsigned sync;  // 0 for no synchronization, or 1 + the actor whose last step happens before this step
  MemOpType type;
  unsigned range; // Index in step_ranges
};
constexpr uint16_t step_ranges[3][2] = {{0, 4}, {2, 6}, {8, 12}};

std::string describe(const std::vector<Step>& steps, bool gc)
{
  std::string res = gc ? "With clean_before:" : "Without clean_before:";
  for (auto const& step : steps) {
    res += " T" + std::to_string(step.actor + 1);
    if (step.sync != 0)
      res += "(after T" + std::to_string(step.sync) + ")";
    res += (step.type == MemOpType::Write ? " W[" : " R[") + std::to_string(step_ranges[step.range][0]) + "," +
           std::to_string(step_ranges[step.range][1]) + ");";
  }
  return res;
}

struct NaiveAccess {
  Epoch epoch;
  MemOpType type;
};

// Keeps every access to every byte, and checks the definition of a race against each of them
class NaiveDetector {
  std::map<uint16_t, std::vector<NaiveAccess>> accesses_; // Offset -> every access to that byte

public:
  // The accesses that race with an access of the given epoch and type on the given byte
  std::vector<NaiveAccess> racers(uint16_t offset, Epoch current, MemOpType type, const VectorClock& vc) const
  {
    std::vector<NaiveAccess> res;
    if (auto it = accesses_.find(offset); it != accesses_.end())
      for (auto const& other : it->second)
        if (other.epoch.get_aid() != current.get_aid() &&
            (other.type == MemOpType::Write || type == MemOpType::Write) &&
            other.epoch.get_clock() > vc.clocks[other.epoch.get_aid().value()].get_clock())
          res.push_back(other);
    return res;
  }
  void record(uint16_t offset, Epoch epoch, MemOpType type) { accesses_[offset].push_back({epoch, type}); }
};

// Runs the scenario with both detectors, and returns a description of their first disagreement (if any).
// With gc, clean_before() is called after each step with the minimum of the vector clocks of the remaining steps.
std::string compare_detectors(const std::vector<Step>& steps, bool gc)
{
  // Compute the vector clock of each step
  std::vector<VectorClock> vcs;
  VectorClock last_vc[3];
  for (auto const& step : steps) {
    VectorClock& vc = last_vc[step.actor];
    if (step.sync != 0)
      for (unsigned i = 0; i < static_config::max_threads; i++)
        vc.clocks[i] = std::max(vc.clocks[i], last_vc[step.sync - 1].clocks[i]);
    Aid aid(step.actor + 1);
    vc.clocks[aid.value()] = Epoch(aid, Clock(vc.clocks[aid.value()].get_clock().value() + 1));
    vcs.push_back(vc);
  }

  SlabTrackState state;
  NaiveDetector naive;
  for (unsigned i = 0; i < steps.size(); i++) {
    const Aid aid(steps[i].actor + 1);
    const Epoch current = vcs[i].clocks[aid.value()];
    MemoryAccessTrace trace;
    trace.create_memory_access(steps[i].type, memory_page_addr + step_ranges[steps[i].range][0],
                               step_ranges[steps[i].range][1] - step_ranges[steps[i].range][0]);

    // Search the races with the naive detector, on the bytes that the trace actually recorded
    std::map<uint16_t, std::vector<NaiveAccess>> racers; // Offset -> racing accesses
    for (auto const& page : trace)
      for (auto const& access : page)
        for (uint16_t offset = access.start_offset; offset < access.end_offset; offset++)
          if (auto r = naive.racers(offset, current, access.type, vcs[i]); not r.empty())
            racers[offset] = r;

    try {
      state.apply_transition(trace, aid, vcs[i]);
    } catch (const McDataRace& race) {
      if (racers.empty())
        return describe(steps, gc) + " SlabTrack reports a race at step " + std::to_string(i + 1) +
               ", but not the naive detector";
      if (race.second_mem_op_ != current || race.second_mem_type_ != steps[i].type)
        return describe(steps, gc) + " SlabTrack reports a wrong second access at step " + std::to_string(i + 1);
      // The first reported access must race on every reported byte
      for (uintptr_t addr = race.location_; addr < race.location_ + race.sizes_[0]; addr++) {
        auto it          = racers.find(static_cast<uint16_t>(addr - memory_page_addr));
        auto same_access = [&race](const NaiveAccess& a) {
          return a.epoch == race.first_mem_op_ && a.type == race.first_mem_type_;
        };
        if (it == racers.end() || std::none_of(it->second.begin(), it->second.end(), same_access))
          return describe(steps, gc) + " SlabTrack reports an access that does not race at offset " +
                 std::to_string(addr - memory_page_addr);
      }
      return ""; // Both detectors agree on the first race
    }
    if (not racers.empty())
      return describe(steps, gc) + " SlabTrack misses the race of step " + std::to_string(i + 1);

    for (auto const& page : trace)
      for (auto const& access : page)
        for (uint16_t offset = access.start_offset; offset < access.end_offset; offset++)
          naive.record(offset, current, access.type);

    if (gc && i + 1 < steps.size()) {
      VectorClock global_min = vcs[i + 1];
      for (unsigned j = i + 2; j < steps.size(); j++)
        for (unsigned k = 0; k < static_config::max_threads; k++)
          global_min.clocks[k] = std::min(global_min.clocks[k], vcs[j].clocks[k]);
      state.clean_before(global_min);
    }
  }
  return "";
}
} // namespace

TEST_CASE("SlabTrackState: exhaustive comparison with a naive detector", "[SlabTrackState][exhaustive]")
{
  // Every possible step. A step cannot synchronize with its own actor.
  std::vector<Step> all_steps;
  for (unsigned actor = 0; actor < 3; actor++)
    for (unsigned sync = 0; sync < 4; sync++)
      for (auto type : {MemOpType::Read, MemOpType::Write})
        for (unsigned range = 0; range < 3; range++)
          if (sync != actor + 1)
            all_steps.push_back({actor, sync, type, range});

  unsigned scenarios = 0;
  for (auto const& s1 : all_steps)
    for (auto const& s2 : all_steps)
      for (auto const& s3 : all_steps) {
        const std::vector<Step> steps = {s1, s2, s3};

        // Skip the scenarios that only differ by the naming of the actors (actors appear in order), and the
        // synchronizations with actors that did not run yet
        bool skip         = false;
        unsigned next_new = 0;
        bool seen[3]      = {false, false, false};
        for (auto const& step : steps) {
          if (not seen[step.actor] && step.actor != next_new++)
            skip = true;
          if (step.sync != 0 && not seen[step.sync - 1])
            skip = true;
          seen[step.actor] = true;
        }
        if (skip)
          continue;

        scenarios++;
        for (bool gc : {false, true})
          if (std::string error = compare_detectors(steps, gc); not error.empty())
            FAIL(error);
      }
  REQUIRE(scenarios == 3672);
}
