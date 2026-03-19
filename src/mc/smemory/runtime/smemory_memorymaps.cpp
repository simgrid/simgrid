/* Copyright (c) 2025-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smemory_observer.h"
#include "src/sthread/sthread.h"
#include "xbt/asserts.h"
#include "xbt/log.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(smemory);

static std::vector<std::pair<uintptr_t, uintptr_t>> stacks_;
void smemory_add_stack(void* begin, void* end)
{
  auto elem = std::make_pair(reinterpret_cast<uintptr_t>(begin), reinterpret_cast<uintptr_t>(end));
  for (auto reg : stacks_)
    xbt_assert(reg.second < elem.first || reg.first > elem.second,
               "Newly added stack [%zu;%zu] intersects with previously added stack [%zu;%zu]", elem.first, elem.second,
               reg.first, reg.second);
  stacks_.push_back(elem);
  std::sort(stacks_.begin(), stacks_.end());
}
void smemory_remove_stack(void* begin, void* end)
{
  auto elem = std::make_pair(reinterpret_cast<uintptr_t>(begin), reinterpret_cast<uintptr_t>(end));
  XBT_DEBUG("Removing stack %p-%p from a list of %lu stacks.", begin, end, stacks_.size());
  for (auto it = stacks_.begin(); it != stacks_.end(); it++)
    if (it->first == elem.first && it->second == elem.second) {
      stacks_.erase(it);
      std::sort(stacks_.begin(), stacks_.end());
      return;
    }
  xbt_die("Cannot remove stack [%zu;%zu] from the list as it's not in it", elem.first, elem.second);
}
bool smemory_is_on_stack(void* ptr)
{
  uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);

  // binary search
  size_t left  = 0;
  size_t right = stacks_.size();

  while (left < right) {
    size_t mid = left + (right - left) / 2;

    if (addr < stacks_[mid].first)
      right = mid;
    else if (addr >= stacks_[mid].second)
      left = mid + 1;
    else
      return true; // found
  }

  return false;
}