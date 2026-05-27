/* Copyright (c) 2025-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smemory_observer.h"
#include "src/sthread/sthread.h"
#include "src/xbt/memory_map.hpp"
#include "xbt/asserts.h"
#include "xbt/log.h"

#include <algorithm>
#include <cstdint>
#include <sys/mman.h>
#include <utility>
#include <vector>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(smemory);

struct MemRange {
  uintptr_t start, end;
};

static std::vector<MemRange> stacks_;
void smemory_add_stack(uintptr_t begin, uintptr_t end)
{
  MemRange elem{reinterpret_cast<uintptr_t>(begin), reinterpret_cast<uintptr_t>(end)};
  for (auto reg : stacks_)
    xbt_assert(reg.end <= elem.start || reg.start >= elem.end,
               "Newly added stack [%zu;%zu] intersects with previously added stack [%zu;%zu]", elem.start, elem.end,
               reg.start, reg.end);
  stacks_.push_back(elem);
  std::ranges::sort(stacks_, {}, &MemRange::start);
}
void smemory_remove_stack(uintptr_t begin, uintptr_t end)
{
  MemRange elem{reinterpret_cast<uintptr_t>(begin), reinterpret_cast<uintptr_t>(end)};
  XBT_DEBUG("Removing stack %p-%p from a list of %lu stacks.", (void*)begin, (void*)end, stacks_.size());
  for (auto it = stacks_.begin(); it != stacks_.end(); it++)
    if (it->start == elem.start && it->end == elem.end) {
      stacks_.erase(it);
      std::ranges::sort(stacks_, {}, &MemRange::start);
      return;
    }
  xbt_die("Cannot remove stack [%zu;%zu] from the list as it's not in it", elem.start, elem.end);
}
bool smemory_is_on_stack(uintptr_t ptr)
{
  uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);

  // binary search
  size_t left  = 0;
  size_t right = stacks_.size();

  while (left < right) {
    size_t mid = left + (right - left) / 2;

    if (addr < stacks_[mid].start)
      right = mid;
    else if (addr >= stacks_[mid].end)
      left = mid + 1;
    else
      return true; // found
  }

  return false;
}

static std::vector<MemRange> rw_ranges;

bool smemory_is_rw_segment(uintptr_t location)
{
  // Initialize the data if it's still empty
  if (rw_ranges.empty()) [[unlikely]] {
    sthread_disable();
    for (const auto& seg : simgrid::xbt::get_memory_map(getpid()))
      if (seg.prot & PROT_READ && seg.prot & PROT_WRITE)
        rw_ranges.emplace_back(seg.start_addr, seg.end_addr);

    // Not sure whether it's sorted by default. Play self.
    std::ranges::sort(rw_ranges, {}, &MemRange::start);
    sthread_enable();
  }

  // upper_bound gives the start element whose start element is above the location
  // The candidate is the element just before it (if any).
  const auto it = std::ranges::upper_bound(rw_ranges, location, {}, &MemRange::start);

  if (it == rw_ranges.begin()) // No matching candidate
    return false;

  const auto& candidate = *std::prev(it);
  return location < candidate.end; // open upper bound: [start, end)
}