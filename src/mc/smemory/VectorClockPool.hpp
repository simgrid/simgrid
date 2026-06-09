/* Copyright (c) 2026-2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SMEMORY_VECTOR_CLOCK_POOL_HPP
#define SIMGRID_MC_SMEMORY_VECTOR_CLOCK_POOL_HPP

#include "src/internal_config.h"
#include "src/mc/api/Epoch.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <unordered_map>
#include <vector>
#if HAVE_XXHASH
#include <xxhash.h>
#endif
#if SIMGRID_HAVE_SMEMORY
#include <immintrin.h> /* x86-only*/
#endif

/* VectorClockPool centralizes the instances of VectorClocks to reduce the memory usage
 * (see https://en.wikipedia.org/wiki/String_interning)
 *
 * This file is part of our super optimized implementation of FastTrack. It uses its own VectorClocks which store Epochs
 * rather than Clocks to reduce the amount of data conversions between the Epoch that FastTrack uses and Clock that
 * other VectorClock use.
 *
 * The code also uses AVX2 and strive to pack its data in few cache lines to optimize as much as possible.
 */

namespace simgrid::mc::smemory {

using index_t = uint32_t;

/* Optimized implementation of VectorClock for the FastTrack algorithm
 * It's aligned to allow for SIMD optimizations, and it's compact enough to fit in few cache lines */
struct alignas(64) VectorClock {
  // We are storing Epoch rather than Clocks so that we don't have to convert our Epochs in Clocks in the calling code
  ::std::array<Epoch, smemory::config::max_threads> clocks;

  VectorClock() noexcept;

  // Comparison operators (based on memcmp to be as fast as it could be)
  [[nodiscard]] bool operator==(const VectorClock& rhs) const noexcept
  {
    return std::memcmp(clocks.data(), rhs.clocks.data(), sizeof(clocks)) == 0;
  }
  [[nodiscard]] bool operator!=(const VectorClock& rhs) const noexcept { return !(*this == rhs); }
};

/* Hashing functor, using XXH3 if possible and FNV-1a if the dependency is missing
 *
 * It takes a pointer as parameter (to not copy the heavy objects) but works on the pointed data on need
 */
struct VectorClockPtrHash {
  std::size_t operator()(const VectorClock* vc) const noexcept
  {
    if (vc == nullptr) [[unlikely]]
      return 0;

#if HAVE_XXHASH
    return static_cast<std::size_t>(XXH3_64bits(vc->clocks.data(), sizeof(vc->clocks)));
#else
    // The classical FNV-1a algorithm for 64 bits
    uint64_t hash                = 0xcbf29ce484222325ULL;
    constexpr uint64_t fnv_prime = 0x100000001b3ULL;

    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(vc->clocks.data());
    for (std::size_t i = 0; i < sizeof(vc->clocks); ++i) {
      hash ^= bytes[i];
      hash *= fnv_prime;
    }

    return static_cast<std::size_t>(hash);
#endif
  }
};

/* Equality functor (working on the pointed data with no copy) */
struct VectorClockPtrEqual {
  bool operator()(const VectorClock* lhs, const VectorClock* rhs) const noexcept
  {
    if (lhs == rhs)
      return true;
    if (lhs == nullptr || rhs == nullptr)
      return false;
    return *lhs == *rhs;
  }
};

class VectorClockPool {
private:
  // The real data
  std::vector<std::unique_ptr<VectorClock>> pool_;
  // The key is a pointer, but the hash and equality functors actually compare what's on the pointer end
  std::unordered_map<const VectorClock*, index_t, VectorClockPtrHash, VectorClockPtrEqual> lookup_;
  // Stack of free indexes that we could reuse
  std::vector<index_t> free_list_;

public:
  VectorClockPool();

  // no copy
  VectorClockPool(const VectorClockPool&)            = delete;
  VectorClockPool& operator=(const VectorClockPool&) = delete;
  // move allowed
  VectorClockPool(VectorClockPool&&) noexcept            = default;
  VectorClockPool& operator=(VectorClockPool&&) noexcept = default;

  /* Inserts a new VC if it wasn't there yet (deduplication), and returns its index
   * @throws std::overflow_error if the maximal index 0x7FFFFFFF is reached */
  index_t get_or_insert(const VectorClock& vc);

  /* Get a VectorClock from its index (super quick) */
  [[nodiscard]] const VectorClock& get(index_t index) const;

  /* Garbage collection: removes all entries that are in the past according to the previous function
   * This is an heavy operation so don't do that too often. */
  void clean_before(const VectorClock& global_min) noexcept;

  /* Check (using AVX2) whether all components of the VC behind that index are less or equal to global_min
   * This will fail if called on a free index (no verification done internally), so don't do that. */
  [[nodiscard]] bool is_past(index_t index, const VectorClock& global_min) const noexcept;

  [[nodiscard]] std::size_t size() const noexcept { return pool_.size(); }
};

} // namespace simgrid::mc::smemory

#endif // SIMGRID_MC_SMEMORY_VECTOR_CLOCK_POOL_HPP