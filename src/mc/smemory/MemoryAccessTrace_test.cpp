/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/3rd-party/catch.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <tuple>
#include <vector>

#define private public
#include "MemoryAccessTrace.hpp"

using namespace simgrid::mc::smemory;

TEST_CASE("MemoryAccessTrace: one read", "[MemoryAccessTrace]")
{
  MemoryAccessTrace tracker;
  std::vector<char> buffer(32, 0); // Addresses on the stack are ignored
  auto addr = reinterpret_cast<uintptr_t>(buffer.data());

  tracker.create_memory_access(MemOpType::Read, addr, 1);

  REQUIRE(tracker.was_read(addr) == true);
  REQUIRE(tracker.was_written(addr) == false);

  // not touched (+tracker.get_bucket_size() because of the granularity simplification)
  REQUIRE(tracker.was_read(addr + tracker.get_bucket_size() + 1) == false);
  REQUIRE(tracker.was_written(addr + tracker.get_bucket_size() + 1) == false);
}

TEST_CASE("MemoryAccessTrace: one write", "[MemoryAccessTrace]")
{
  MemoryAccessTrace tracker;
  std::vector<char> buffer(32, 0);
  auto addr = reinterpret_cast<uintptr_t>(buffer.data());

  tracker.create_memory_access(MemOpType::Write, addr, 1);

  REQUIRE(tracker.was_read(addr) == false);
  REQUIRE(tracker.was_written(addr) == true);

  // not touched
  REQUIRE(tracker.was_read(addr + tracker.get_bucket_size() + 1) == false);
  REQUIRE(tracker.was_written(addr + tracker.get_bucket_size() + 1) == false);
}

TEST_CASE("MemoryAccessTrace: write + read", "[MemoryAccessTrace]")
{
  MemoryAccessTrace tracker;
  std::vector<char> buffer(32, 0);
  auto addr = reinterpret_cast<uintptr_t>(buffer.data());

  tracker.create_memory_access(MemOpType::Read, addr, 1);
  tracker.create_memory_access(MemOpType::Write, addr, 1);

  REQUIRE(tracker.was_read(addr) == false); // write dominates
  REQUIRE(tracker.was_written(addr) == true);
}

TEST_CASE("MemoryAccessTrace: several bytes of the same bucket", "[MemoryAccessTrace]")
{
  MemoryAccessTrace tracker;
  std::vector<char> buffer(32, 0);
  auto addr = reinterpret_cast<uintptr_t>(buffer.data());

  tracker.create_memory_access(MemOpType::Read, addr, MemoryAccessTrace::get_bucket_size());
  for (unsigned int i = 0; i < MemoryAccessTrace::get_bucket_size(); i++)
    REQUIRE(tracker.was_read(addr + i) == true);
  REQUIRE(tracker.was_read(addr + MemoryAccessTrace::get_bucket_size()) == false); // next bucket

  // next bucket
  tracker.create_memory_access(MemOpType::Write, addr + MemoryAccessTrace::get_bucket_size(),
                               MemoryAccessTrace::get_bucket_size());
  for (unsigned int i = 0; i < MemoryAccessTrace::get_bucket_size(); i++)
    REQUIRE(tracker.was_written(addr + MemoryAccessTrace::get_bucket_size() + i) == true);
  REQUIRE(tracker.was_written(addr + 2 * MemoryAccessTrace::get_bucket_size()) == false);
  REQUIRE(tracker.was_read(addr + 2 * MemoryAccessTrace::get_bucket_size()) == false);
}

TEST_CASE("MemoryAccessTrace: access accross two words", "[MemoryAccessTrace]")
{
  MemoryAccessTrace tracker;
  constexpr size_t word_bytes = 64 * MemoryAccessTrace::get_bucket_size(); // 64 buckets * 4 bytes

  std::vector<char> buffer(word_bytes * 2, 0);
  auto addr = reinterpret_cast<uintptr_t>(buffer.data());

  // Crossing the border
  tracker.create_memory_access(MemOpType::Read, addr + word_bytes - 8, 16);

  // Check word 0
  REQUIRE(tracker.was_read(addr + word_bytes - 9) == false);
  REQUIRE(tracker.was_read(addr + word_bytes - 8) == true);
  REQUIRE(tracker.was_read(addr + word_bytes - 1) == true);

  // check word 1
  REQUIRE(tracker.was_read(addr + word_bytes) == true);
  REQUIRE(tracker.was_read(addr + word_bytes + 7) == true);
  REQUIRE(tracker.was_read(addr + word_bytes + 8) == false);
}

TEST_CASE("MemoryAccessTrace: accross page boundary", "[MemoryAccessTrace]")
{
  MemoryAccessTrace tracker;

  constexpr size_t page_size = 4096;
  std::vector<char> buffer(page_size * 2, 0);
  auto addr = reinterpret_cast<uintptr_t>(buffer.data());

  // This write ends exactly at the end of the page
  tracker.create_memory_access(MemOpType::Write, addr + page_size - 8, 8);

  REQUIRE(tracker.was_written(addr + page_size - 1) == true);
  REQUIRE(tracker.was_written(addr + page_size) == false);

  // Accross the page boundary
  tracker.create_memory_access(MemOpType::Write, addr + page_size - 4, 8);

  REQUIRE(tracker.was_written(addr + page_size - 1) == true);
  REQUIRE(tracker.was_written(addr + page_size) == true);
  REQUIRE(tracker.was_written(addr + page_size + 3) == true);
  REQUIRE(tracker.was_written(addr + page_size + 4) == false);
}

static std::vector<bool>& make_result(MemoryAccessTrace& tracker, uintptr_t base, size_t size, MemOpType kind)
{
  static std::vector<bool> result;
  result.clear();
  for (unsigned i = 0; i < size; i++)
    result.push_back(kind == MemOpType::Read ? tracker.was_read(base + i) : tracker.was_written(base + i));
  return result;
}

TEST_CASE("MemoryAccessTrace: writes overlapping on previous reads", "[MemoryAccessTrace]")
{
  MemoryAccessTrace tracker;
  std::vector<char> buffer(32, 0);
  auto addr = reinterpret_cast<uintptr_t>(buffer.data());
  {
    std::vector<bool> reads;
    std::vector<bool> writes;
    std::vector<bool> expected;

    for (unsigned i = 0; i < 8; i++) {
      UNSCOPED_INFO("Before, value " << i << " is: read=" << tracker.was_read(addr + i)
                                     << " ; write=" << tracker.was_written(addr + i));
      reads.push_back(tracker.was_read(addr + i));
      writes.push_back(tracker.was_written(addr + i));
      expected.push_back(false);
    }

    REQUIRE_THAT(reads, Catch::Matchers::Equals(expected));
    REQUIRE_THAT(writes, Catch::Matchers::Equals(expected));
  }

  // read 8 bytes
  tracker.create_memory_access(MemOpType::Read, addr, 8);
  {
    std::vector<bool> result;
    std::vector<bool> expected;

    for (unsigned i = 0; i < 8; i++) {
      UNSCOPED_INFO("Value " << i << " is: read=" << tracker.was_read(addr + i)
                             << " ; write=" << tracker.was_written(addr + i));
      result.push_back(tracker.was_read(addr + i));
      expected.push_back(true);
    }

    REQUIRE_THAT(result, Catch::Matchers::Equals(expected));
  }

  // write 4 bytes, overlaps the previous read
  tracker.create_memory_access(MemOpType::Write, addr + 4, 4);
  {
    std::vector<bool> result_reads;
    std::vector<bool> result_writes;
    std::vector<bool> expected_reads;
    std::vector<bool> expected_writes;

    for (unsigned i = 0; i < 8; i++) {
      UNSCOPED_INFO("After write " << i << " is: read=" << tracker.was_read(addr + i)
                                   << " ; write=" << tracker.was_written(addr + i));
      result_reads.push_back(tracker.was_read(addr + i));
      result_writes.push_back(tracker.was_written(addr + i));
      expected_reads.push_back(i < 4);   // write dominates
      expected_writes.push_back(i >= 4); // Only wrote on the second part
    }
    REQUIRE_THAT(expected_reads, Catch::Matchers::Equals(result_reads));
    REQUIRE_THAT(expected_writes, Catch::Matchers::Equals(result_writes));
  }
}

// ============ Two-Level Online Iterator ================

using Interval = std::tuple<uintptr_t, uintptr_t, MemOpType>; // Absolute start and end addresses, and type

// The intervals yielded for an access to [start, end): a single one with coalescing, one per bucket without
static std::vector<Interval> expected_intervals(uintptr_t start, uintptr_t end, MemOpType type)
{
  const uintptr_t bucket = MemoryAccessTrace::get_bucket_size();
  start                  = start / bucket * bucket;
  end                    = (end + bucket - 1) / bucket * bucket;
  if (MemoryAccessTrace::is_coalescing())
    return {{start, end, type}};
  std::vector<Interval> res;
  for (uintptr_t b = start; b < end; b += bucket)
    res.emplace_back(b, b + bucket, type);
  return res;
}

// All the intervals yielded by the iterator, page after page
static std::vector<Interval> yielded_intervals(const MemoryAccessTrace& tracker)
{
  std::vector<Interval> res;
  for (auto const& page : tracker)
    for (auto const& interval : page)
      res.emplace_back(page.page_base_addr + interval.start_offset, page.page_base_addr + interval.end_offset,
                       interval.type);
  std::ranges::sort(res);
  return res;
}

// Simulated addresses, to avoid ASLR and alignment issues. 0x4000 is the start of a page.
constexpr uintptr_t base = 0x4000;

TEST_CASE("Iterator: reads in a single page", "[MemoryAccessTrace]")
{
  MemoryAccessTrace tracker;
  tracker.create_memory_access(MemOpType::Read, base, 8);
  REQUIRE(yielded_intervals(tracker) == expected_intervals(base, base + 8, MemOpType::Read));
}

TEST_CASE("Iterator: writes in a single page", "[MemoryAccessTrace]")
{
  MemoryAccessTrace tracker;
  tracker.create_memory_access(MemOpType::Write, base, 8);
  REQUIRE(yielded_intervals(tracker) == expected_intervals(base, base + 8, MemOpType::Write));
}

TEST_CASE("Iterator: reads and then writes", "[MemoryAccessTrace]")
{
  MemoryAccessTrace tracker;
  tracker.create_memory_access(MemOpType::Read, base, 8);
  tracker.create_memory_access(MemOpType::Write, base + 4, 4);

  auto expected = expected_intervals(base, base + 4, MemOpType::Read);
  std::ranges::copy(expected_intervals(base + 4, base + 8, MemOpType::Write), std::back_inserter(expected));
  REQUIRE(yielded_intervals(tracker) == expected);
}

TEST_CASE("Iterator: contiguous accesses are merged", "[MemoryAccessTrace]")
{
  MemoryAccessTrace tracker;
  tracker.create_memory_access(MemOpType::Write, base, 4);     // An integer
  tracker.create_memory_access(MemOpType::Write, base + 4, 1); // Two chars
  tracker.create_memory_access(MemOpType::Write, base + 5, 1);
  REQUIRE(yielded_intervals(tracker) == expected_intervals(base, base + 6, MemOpType::Write));
}

TEST_CASE("Iterator: accesses covering whole words of the bitfield", "[MemoryAccessTrace]")
{
  const uintptr_t word_bytes = 64 * MemoryAccessTrace::get_bucket_size();

  SECTION("Exactly one word")
  {
    MemoryAccessTrace tracker;
    tracker.create_memory_access(MemOpType::Write, base + word_bytes, word_bytes);
    REQUIRE(yielded_intervals(tracker) ==
            expected_intervals(base + word_bytes, base + 2 * word_bytes, MemOpType::Write));
  }
  SECTION("Across three words")
  {
    MemoryAccessTrace tracker;
    tracker.create_memory_access(MemOpType::Read, base + word_bytes / 2, 2 * word_bytes);
    REQUIRE(yielded_intervals(tracker) ==
            expected_intervals(base + word_bytes / 2, base + word_bytes / 2 + 2 * word_bytes, MemOpType::Read));
  }
  SECTION("The whole page, then the beginning of the next one")
  {
    MemoryAccessTrace tracker;
    tracker.create_memory_access(MemOpType::Write, base, 4096 + 8);
    auto expected = expected_intervals(base, base + 4096, MemOpType::Write);
    std::ranges::copy(expected_intervals(base + 4096, base + 4096 + 8, MemOpType::Write), std::back_inserter(expected));
    REQUIRE(yielded_intervals(tracker) == expected);
  }
}

TEST_CASE("Iterator: isolated buckets at the edges of the words and of the page", "[MemoryAccessTrace]")
{
  const uintptr_t bucket = MemoryAccessTrace::get_bucket_size();
  for (uintptr_t index : {0UL, 1UL, 62UL, 63UL, 64UL, 65UL, 127UL, 128UL, 4096 / bucket - 1}) {
    INFO("Bucket " << index);
    MemoryAccessTrace tracker;
    tracker.create_memory_access(MemOpType::Write, base + index * bucket, bucket);
    REQUIRE(yielded_intervals(tracker) ==
            expected_intervals(base + index * bucket, base + (index + 1) * bucket, MemOpType::Write));
  }
}

TEST_CASE("Iterator: separated accesses are not merged", "[MemoryAccessTrace]")
{
  const uintptr_t bucket = MemoryAccessTrace::get_bucket_size();
  MemoryAccessTrace tracker;
  // On both sides of a word boundary, with a gap of one bucket after each of them
  tracker.create_memory_access(MemOpType::Read, base + 61 * bucket, 2 * bucket);
  tracker.create_memory_access(MemOpType::Read, base + 64 * bucket, 2 * bucket);

  auto expected = expected_intervals(base + 61 * bucket, base + 63 * bucket, MemOpType::Read);
  std::ranges::copy(expected_intervals(base + 64 * bucket, base + 66 * bucket, MemOpType::Read),
                    std::back_inserter(expected));
  REQUIRE(yielded_intervals(tracker) == expected);
}

TEST_CASE("Iterator: reads and writes of adjacent buckets are separate intervals", "[MemoryAccessTrace]")
{
  const uintptr_t bucket = MemoryAccessTrace::get_bucket_size();
  MemoryAccessTrace tracker;
  // Write, read, read-and-write (reported as a write), then read, across a word boundary
  tracker.create_memory_access(MemOpType::Write, base + 62 * bucket, bucket);
  tracker.create_memory_access(MemOpType::Read, base + 63 * bucket, 2 * bucket);
  tracker.create_memory_access(MemOpType::Write, base + 64 * bucket, bucket);
  tracker.create_memory_access(MemOpType::Read, base + 65 * bucket, bucket);

  auto expected = expected_intervals(base + 62 * bucket, base + 63 * bucket, MemOpType::Write);
  std::ranges::copy(expected_intervals(base + 63 * bucket, base + 64 * bucket, MemOpType::Read),
                    std::back_inserter(expected));
  std::ranges::copy(expected_intervals(base + 64 * bucket, base + 65 * bucket, MemOpType::Write),
                    std::back_inserter(expected));
  std::ranges::copy(expected_intervals(base + 65 * bucket, base + 66 * bucket, MemOpType::Read),
                    std::back_inserter(expected));
  REQUIRE(yielded_intervals(tracker) == expected);
}

TEST_CASE("Iterator: an empty trace yields nothing", "[MemoryAccessTrace]")
{
  MemoryAccessTrace tracker;
  tracker.create_memory_access(MemOpType::Write, base, 0); // An empty memset, for instance
  REQUIRE(tracker.empty());
  REQUIRE(yielded_intervals(tracker).empty());
}
