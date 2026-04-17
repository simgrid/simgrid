/* Copyright (c) 2026-2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/3rd-party/catch.hpp"
#include <cstddef>
#include <vector>

#define private public
#include "MemoryAccessTracker.hpp"

using namespace simgrid::mc::smemory;

TEST_CASE("MemoryAccessTracker: one read", "[MemoryAccessTracker]")
{
  MemoryAccessTracker tracker;
  std::vector<char> buffer(32, 0); // Addresses on the stack are ignored

  tracker.create_memory_access(MemOpType::Read, buffer.data(), 1);

  REQUIRE(tracker.was_read(buffer.data()) == true);
  REQUIRE(tracker.was_written(buffer.data()) == false);

  // not touched (+tracker.get_bucket_size() because of the granularity simplification)
  REQUIRE(tracker.was_read(buffer.data() + tracker.get_bucket_size() + 1) == false);
  REQUIRE(tracker.was_written(buffer.data() + tracker.get_bucket_size() + 1) == false);
}

TEST_CASE("MemoryAccessTracker: one write", "[MemoryAccessTracker]")
{
  MemoryAccessTracker tracker;
  std::vector<char> buffer(32, 0);

  tracker.create_memory_access(MemOpType::Write, buffer.data(), 1);

  REQUIRE(tracker.was_read(buffer.data()) == false);
  REQUIRE(tracker.was_written(buffer.data()) == true);

  // not touched
  REQUIRE(tracker.was_read(buffer.data() + tracker.get_bucket_size() + 1) == false);
  REQUIRE(tracker.was_written(buffer.data() + tracker.get_bucket_size() + 1) == false);
}

TEST_CASE("MemoryAccessTracker: write + read", "[MemoryAccessTracker]")
{
  MemoryAccessTracker tracker;
  std::vector<char> buffer(32, 0);

  tracker.create_memory_access(MemOpType::Read, buffer.data(), 1);
  tracker.create_memory_access(MemOpType::Write, buffer.data(), 1);

  REQUIRE(tracker.was_read(buffer.data()) == false); // write dominates
  REQUIRE(tracker.was_written(buffer.data()) == true);
}

TEST_CASE("MemoryAccessTracker: several bytes of the same bucket", "[MemoryAccessTracker]")
{
  MemoryAccessTracker tracker;
  std::vector<char> buffer(32, 0);

  tracker.create_memory_access(MemOpType::Read, buffer.data(), MemoryAccessTracker::get_bucket_size());
  for (unsigned int i = 0; i < MemoryAccessTracker::get_bucket_size(); i++)
    REQUIRE(tracker.was_read(buffer.data() + i) == true);
  REQUIRE(tracker.was_read(buffer.data() + MemoryAccessTracker::get_bucket_size()) == false); // next bucket

  // next bucket
  tracker.create_memory_access(MemOpType::Write, buffer.data() + MemoryAccessTracker::get_bucket_size(),
                               MemoryAccessTracker::get_bucket_size());
  for (unsigned int i = 0; i < MemoryAccessTracker::get_bucket_size(); i++)
    REQUIRE(tracker.was_written(buffer.data() + MemoryAccessTracker::get_bucket_size() + i) == true);
  REQUIRE(tracker.was_written(buffer.data() + 2 * MemoryAccessTracker::get_bucket_size()) == false);
  REQUIRE(tracker.was_read(buffer.data() + 2 * MemoryAccessTracker::get_bucket_size()) == false);
}

TEST_CASE("MemoryAccessTracker: access accross two words", "[MemoryAccessTracker]")
{
  MemoryAccessTracker tracker;
  constexpr size_t word_bytes = 64 * MemoryAccessTracker::get_bucket_size(); // 64 buckets * 4 bytes

  std::vector<char> buffer(word_bytes * 2, 0);

  // Crossing the border
  tracker.create_memory_access(MemOpType::Read, buffer.data() + word_bytes - 8, 16);

  // Check word 0
  REQUIRE(tracker.was_read(buffer.data() + word_bytes - 9) == false);
  REQUIRE(tracker.was_read(buffer.data() + word_bytes - 8) == true);
  REQUIRE(tracker.was_read(buffer.data() + word_bytes - 1) == MemoryAccessTracker::is_coalescing());

  // check word 1
  REQUIRE(tracker.was_read(buffer.data() + word_bytes) == MemoryAccessTracker::is_coalescing());
  REQUIRE(tracker.was_read(buffer.data() + word_bytes + 7) == MemoryAccessTracker::is_coalescing());
  REQUIRE(tracker.was_read(buffer.data() + word_bytes + 8) == false);
}

TEST_CASE("MemoryAccessTracker: accross page boundary", "[MemoryAccessTracker]")
{
  MemoryAccessTracker tracker;

  constexpr size_t page_size = 4096;
  std::vector<char> buffer(page_size * 2, 0);

  // This write ends exactly at the end of the page
  tracker.create_memory_access(MemOpType::Write, buffer.data() + page_size - 8, 8);

  REQUIRE(tracker.was_written(buffer.data() + page_size - 1) == MemoryAccessTracker::is_coalescing());
  REQUIRE(tracker.was_written(buffer.data() + page_size) == false);

  // Accross the page boundary
  tracker.create_memory_access(MemOpType::Write, buffer.data() + page_size - 4, 8);

  REQUIRE(tracker.was_written(buffer.data() + page_size - 1) ==
          (MemoryAccessTracker::is_coalescing() || MemoryAccessTracker::get_bucket_size() >= 4));
  REQUIRE(tracker.was_written(buffer.data() + page_size) == MemoryAccessTracker::is_coalescing());
  REQUIRE(tracker.was_written(buffer.data() + page_size + 3) == MemoryAccessTracker::is_coalescing());
  REQUIRE(tracker.was_written(buffer.data() + page_size + 4) == false);
}

static std::vector<bool>& make_result(MemoryAccessTracker& tracker, char* base, size_t size, MemOpType kind)
{
  static std::vector<bool> result;
  result.clear();
  for (unsigned i = 0; i < size; i++)
    result.push_back(kind == MemOpType::Read ? tracker.was_read(base + i) : tracker.was_written(base + i));
  return result;
}

TEST_CASE("MemoryAccessTracker: writes overlapping on previous reads", "[MemoryAccessTracker]")
{
  MemoryAccessTracker tracker;
  std::vector<char> buffer(32, 0);
  {
    std::vector<bool> reads;
    std::vector<bool> writes;
    std::vector<bool> expected;

    for (unsigned i = 0; i < 8; i++) {
      UNSCOPED_INFO("Before, value " << i << " is: read=" << tracker.was_read(buffer.data() + i)
                                     << " ; write=" << tracker.was_written(buffer.data() + i));
      reads.push_back(tracker.was_read(buffer.data() + i));
      writes.push_back(tracker.was_written(buffer.data() + i));
      expected.push_back(false);
    }

    REQUIRE_THAT(reads, Catch::Matchers::Equals(expected));
    REQUIRE_THAT(writes, Catch::Matchers::Equals(expected));
  }

  // read 8 bytes
  tracker.create_memory_access(MemOpType::Read, buffer.data(), 8);
  {
    std::vector<bool> result;
    std::vector<bool> expected;

    for (unsigned i = 0; i < 8; i++) {
      UNSCOPED_INFO("Value " << i << " is: read=" << tracker.was_read(buffer.data() + i)
                             << " ; write=" << tracker.was_written(buffer.data() + i));
      result.push_back(tracker.was_read(buffer.data() + i));
      expected.push_back(i < MemoryAccessTracker::get_bucket_size() || MemoryAccessTracker::is_coalescing());
    }

    REQUIRE_THAT(result, Catch::Matchers::Equals(expected));
  }

  // write 4 bytes, overlaps the previous read
  tracker.create_memory_access(MemOpType::Write, buffer.data() + 4, 4);
  {
    std::vector<bool> result_reads;
    std::vector<bool> result_writes;
    std::vector<bool> expected_reads;
    std::vector<bool> expected_writes;

    for (unsigned i = 0; i < 8; i++) {
      UNSCOPED_INFO("After write " << i << " is: read=" << tracker.was_read(buffer.data() + i)
                                   << " ; write=" << tracker.was_written(buffer.data() + i));
      result_reads.push_back(tracker.was_read(buffer.data() + i));
      result_writes.push_back(tracker.was_written(buffer.data() + i));
      if (MemoryAccessTracker::is_coalescing()) {
        expected_reads.push_back(i < 4 ? true : false);  // write dominates
        expected_writes.push_back(i < 4 ? false : true); // Only wrote on the first part
      } else {
        expected_reads.push_back(i < MemoryAccessTracker::get_bucket_size() ? true : false); // write dominates
        expected_writes.push_back(i < 4 ? false
                                        : (i - 4 < MemoryAccessTracker::get_bucket_size()
                                               ? true
                                               : false)); // Only wrote on the first part, but a whole bucklet
      }
    }
    REQUIRE_THAT(expected_reads, Catch::Matchers::Equals(result_reads));
    REQUIRE_THAT(expected_writes, Catch::Matchers::Equals(result_writes));
  }
}

// ============ Iterator ================

TEST_CASE("Iterator: reads in a single page", "[MemoryAccessTracker]")
{
  MemoryAccessTracker tracker; // granularité 4 bytes
  std::vector<char> buffer(16, 0);

  tracker.create_memory_access(MemOpType::Read, buffer.data(), 8);

  auto it  = tracker.begin();
  auto end = tracker.end();

  REQUIRE(it != end);

  auto [addr, size, type] = *it;
  REQUIRE(addr == (void*)buffer.data());
  if (MemoryAccessTracker::is_coalescing())
    REQUIRE(size == 8);
  else
    REQUIRE(size == std::min<size_t>(8, MemoryAccessTracker::get_bucket_size()));
  REQUIRE(type == MemOpType::Read);

  ++it;
  REQUIRE(it == end); // only one chunk
}

TEST_CASE("Iterator: writes in a single page", "[MemoryAccessTracker]")
{
  MemoryAccessTracker tracker;
  std::vector<char> buffer(16, 0);

  tracker.create_memory_access(MemOpType::Write, buffer.data(), 8);

  auto it                 = tracker.begin();
  auto [addr, size, type] = *it;

  REQUIRE(addr == buffer.data());
  if (MemoryAccessTracker::is_coalescing())
    REQUIRE(size == 8);
  else
    REQUIRE(size == std::min<size_t>(8, MemoryAccessTracker::get_bucket_size()));
  REQUIRE(type == MemOpType::Write);
}

TEST_CASE("Iterator: reads and then writes", "[MemoryAccessTracker]")
{
  MemoryAccessTracker tracker;
  std::vector<char> buffer(16, 0);

  tracker.create_memory_access(MemOpType::Read, buffer.data(), 8);
  tracker.create_memory_access(MemOpType::Write, buffer.data() + 4, 4);

  auto it = tracker.begin();
  // First chunk
  auto [addr1, size1, type1] = *it;
  REQUIRE(addr1 == buffer.data());
  REQUIRE(type1 == MemOpType::Read);
  if (MemoryAccessTracker::is_coalescing())
    REQUIRE(size1 == 4);
  else
    REQUIRE(size1 == std::max<size_t>(1, MemoryAccessTracker::get_bucket_size()));

  // Second chunk
  ++it;
  auto [addr2, size2, type2] = *it;
  REQUIRE(addr2 == buffer.data() + 4);
  if (MemoryAccessTracker::is_coalescing())
    REQUIRE(size2 == 4);
  else
    REQUIRE(size2 == std::max<size_t>(1, MemoryAccessTracker::get_bucket_size()));
  REQUIRE(type2 == MemOpType::Write);

  // No more chunks
  ++it;
  REQUIRE(it == tracker.end());
}

TEST_CASE("Iterator: contiguity within page", "[MemoryAccessTracker]")
{
  MemoryAccessTracker tracker;
  std::vector<char> buffer(32, 0);

  // Two contiguous writes in the same page
  tracker.create_memory_access(MemOpType::Write, buffer.data(), 4);
  tracker.create_memory_access(MemOpType::Write, buffer.data() + 4, 4);

  auto it                 = tracker.begin();
  auto [addr, size, type] = *it;
  REQUIRE(addr == (void*)buffer.data());
  REQUIRE(type == MemOpType::Write);
  if (MemoryAccessTracker::is_coalescing())
    REQUIRE(size == 8); // both chunks were merged
  else {
    REQUIRE(size == std::min<size_t>(4, MemoryAccessTracker::get_bucket_size()));
    ++it;
    REQUIRE(it != tracker.end()); // not merged

    std::tie(addr, size, type) = *it;
    REQUIRE(addr == (void*)(buffer.data() + 4));
    REQUIRE(size == std::min<size_t>(4, MemoryAccessTracker::get_bucket_size()));
    REQUIRE(type == MemOpType::Write);
  }

  // No more chunks
  ++it;
  REQUIRE(it == tracker.end());
}

TEST_CASE("Iterator: marking adjacent variables", "[MemoryAccessTracker]")
{
  MemoryAccessTracker tracker;

  // Simulate variables instead of using real ones to avoid ASLR and alignment issues
  tracker.create_memory_access(MemOpType::Write, (void*)0x4000, 4); // Simulate an integer
  tracker.create_memory_access(MemOpType::Write, (void*)0x4004, 1); // Simulate a char
  tracker.create_memory_access(MemOpType::Write, (void*)0x4005, 1);

  auto it                 = tracker.begin();
  auto [addr, size, type] = *it;
  REQUIRE(addr == (void*)0x4000);
  REQUIRE(type == MemOpType::Write);
  if (MemoryAccessTracker::is_coalescing())
    REQUIRE(size == 6); // chunks were merged
  else {
    REQUIRE(size == std::max<size_t>(1, MemoryAccessTracker::get_bucket_size()));
    ++it;
    if (4 >= MemoryAccessTracker::get_bucket_size()) { // Not all variables fall into the same bucklet
      REQUIRE(it != tracker.end());                    // not merged

      std::tie(addr, size, type) = *it;
      REQUIRE(addr == (void*)0x4004);
      REQUIRE(size == std::max<size_t>(1, MemoryAccessTracker::get_bucket_size()));
      REQUIRE(type == MemOpType::Write);
      ++it;

      if (MemoryAccessTracker::get_bucket_size() == 1) { // The two chars do not fall into the same bucklet
        REQUIRE(it != tracker.end());                    // not merged

        std::tie(addr, size, type) = *it;
        REQUIRE(addr == (void*)0x4005);
        REQUIRE(size == std::max<size_t>(1, MemoryAccessTracker::get_bucket_size()));
        REQUIRE(type == MemOpType::Write);
        ++it;
      }
    }
  }

  // No more chunks
  ++it;
  REQUIRE(it == tracker.end());
}

// ============ Page::find_prev_marked_bucket() and Page::find_next_marked_bucket() ================

TEST_CASE("find_*_marked_bucket - no bucket marked", "[MemoryAccessTracker]")
{
  MemoryAccessTracker::Page page;

  auto res = page.find_prev_marked_bucket(10, MemOpType::Write);
  REQUIRE_FALSE(res.has_value());

  res = page.find_next_marked_bucket(10, MemOpType::Write);
  REQUIRE_FALSE(res.has_value());
}

TEST_CASE("find_*_marked_bucket - single bucket exact match", "[MemoryAccessTracker]")
{
  MemoryAccessTracker::Page page;
  page.mark_bucket(5, MemOpType::Write);

  auto res = page.find_prev_marked_bucket(5, MemOpType::Write);
  REQUIRE(res.has_value());
  REQUIRE(res.value() == 5);

  res = page.find_next_marked_bucket(5, MemOpType::Write);
  REQUIRE(res.has_value());
  REQUIRE(res.value() == 5);
}

TEST_CASE("find_*_marked_bucket - single bucket before/after start -> found", "[MemoryAccessTracker]")
{
  MemoryAccessTracker::Page page;
  page.mark_bucket(5, MemOpType::Write);

  auto res = page.find_prev_marked_bucket(10, MemOpType::Write);
  REQUIRE(res.has_value());
  REQUIRE(res.value() == 5);

  res = page.find_next_marked_bucket(0, MemOpType::Write);
  REQUIRE(res.has_value());
  REQUIRE(res.value() == 5);
}

TEST_CASE("find_prev_marked_bucket - single bucket after/before start -> not found", "[MemoryAccessTracker]")
{
  MemoryAccessTracker::Page page;
  page.mark_bucket(20, MemOpType::Write);

  auto res = page.find_prev_marked_bucket(10, MemOpType::Write);
  REQUIRE_FALSE(res.has_value());

  res = page.find_next_marked_bucket(30, MemOpType::Write);
  REQUIRE_FALSE(res.has_value());
}

TEST_CASE("find_*_marked_bucket - multiple buckets same word", "[MemoryAccessTracker]")
{
  MemoryAccessTracker::Page page;
  page.mark_bucket(3, MemOpType::Write);
  page.mark_bucket(7, MemOpType::Write);
  page.mark_bucket(12, MemOpType::Write);

  auto res = page.find_prev_marked_bucket(15, MemOpType::Write);
  REQUIRE(res.value() == 12);

  res = page.find_prev_marked_bucket(10, MemOpType::Write);
  REQUIRE(res.value() == 7);

  res = page.find_next_marked_bucket(2, MemOpType::Write);
  REQUIRE(res.value() == 3);

  res = page.find_next_marked_bucket(4, MemOpType::Write);
  REQUIRE(res.value() == 7);

  res = page.find_next_marked_bucket(8, MemOpType::Write);
  REQUIRE(res.value() == 12);
}

TEST_CASE("find_*_marked_bucket - bucket at word boundary 63", "[MemoryAccessTracker]")
{
  MemoryAccessTracker::Page page;
  page.mark_bucket(63, MemOpType::Write);

  auto res = page.find_prev_marked_bucket(63, MemOpType::Write);
  REQUIRE(res.value() == 63);

  res = page.find_prev_marked_bucket(62, MemOpType::Write);
  REQUIRE_FALSE(res.has_value());

  res = page.find_next_marked_bucket(63, MemOpType::Write);
  REQUIRE(res.value() == 63);

  res = page.find_next_marked_bucket(62, MemOpType::Write);
  REQUIRE(res.value() == 63);
}
TEST_CASE("find_next_marked_bucket - boundary 64", "[MemoryAccessTracker]")
{
  MemoryAccessTracker::Page page;
  page.mark_bucket(64, MemOpType::Write);

  auto res = page.find_next_marked_bucket(63, MemOpType::Write);
  REQUIRE(res.value() == 64);

  res = page.find_next_marked_bucket(0, MemOpType::Write);
  REQUIRE(res.value() == 64);

  res = page.find_prev_marked_bucket(65, MemOpType::Write);
  REQUIRE(res.value() == 64);
  res = page.find_prev_marked_bucket(200, MemOpType::Write);
  REQUIRE(res.value() == 64);
}

TEST_CASE("find_prev_marked_bucket - across words", "[MemoryAccessTracker]")
{
  MemoryAccessTracker::Page page;
  page.mark_bucket(10, MemOpType::Write);
  page.mark_bucket(130, MemOpType::Write); // second word

  auto res = page.find_prev_marked_bucket(200, MemOpType::Write);
  REQUIRE(res.value() == 130);
  res = page.find_next_marked_bucket(11, MemOpType::Write);
  REQUIRE(res.value() == 130);

  res = page.find_prev_marked_bucket(129, MemOpType::Write);
  REQUIRE(res.value() == 10);
}

TEST_CASE("find_prev_marked_bucket - first bucket of page", "[MemoryAccessTracker]")
{
  MemoryAccessTracker::Page page;
  page.mark_bucket(0, MemOpType::Write);

  auto res = page.find_prev_marked_bucket(0, MemOpType::Write);
  REQUIRE(res.value() == 0);

  res = page.find_prev_marked_bucket(10, MemOpType::Write);
  REQUIRE(res.value() == 0);

  res = page.find_prev_marked_bucket(200, MemOpType::Write); // Accross words
  REQUIRE(res.value() == 0);
}

TEST_CASE("find_next_marked_bucket - last bucket of page", "[MemoryAccessTracker]")
{
  MemoryAccessTracker::Page page;
  uintptr_t last = MemoryAccessTracker::buckets_per_page_ - 1;
  page.mark_bucket(last, MemOpType::Write);

  auto res = page.find_next_marked_bucket(last, MemOpType::Write);
  REQUIRE(res.value() == last);
  res = page.find_next_marked_bucket(0, MemOpType::Write);
  REQUIRE(res.value() == last);
}

TEST_CASE("find_prev_marked_bucket - start_bucket = 0 no match", "[MemoryAccessTracker]")
{
  MemoryAccessTracker::Page page;
  page.mark_bucket(10, MemOpType::Write);

  auto res = page.find_prev_marked_bucket(0, MemOpType::Write);
  REQUIRE_FALSE(res.has_value());
}

TEST_CASE("find_prev_marked_bucket - read vs write separation", "[MemoryAccessTracker]")
{
  MemoryAccessTracker::Page page;
  page.mark_bucket(15, MemOpType::Read);

  auto res = page.find_prev_marked_bucket(20, MemOpType::Write);
  REQUIRE_FALSE(res.has_value());

  res = page.find_prev_marked_bucket(20, MemOpType::Read);
  REQUIRE(res.value() == 15);

  res = page.find_next_marked_bucket(10, MemOpType::Write);
  REQUIRE_FALSE(res.has_value());

  res = page.find_next_marked_bucket(10, MemOpType::Read);
  REQUIRE(res.value() == 15);
}
