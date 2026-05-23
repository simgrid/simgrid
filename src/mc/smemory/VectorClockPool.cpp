/* Copyright (c) 2026-2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "VectorClockPool.hpp"
#include "src/internal_config.h"

#include <utility>

namespace simgrid::mc::smemory {
constexpr index_t INVALID_INDEX =
    0x7FFFFFFF; // We cannot use indexes larger than this value, because the indexes are 31 bits only in Epoch

// Build an initializer of clocks for the VC constructor. We need to cap the value of Aid to max_threads-2 to avoid
// Aid::INVALID
template <std::size_t... Is>
static std::array<Epoch, sizeof...(Is)> make_default_clocks_impl(std::index_sequence<Is...>) noexcept
{
  constexpr std::size_t Cap = smemory::config::max_threads - 2;
  return {Epoch(Aid(static_cast<uint32_t>(Is < Cap ? Is : Cap)), Clock(0u))...};
}
static const std::array<Epoch, smemory::config::max_threads> s_default_clocks =
    make_default_clocks_impl(std::make_index_sequence<smemory::config::max_threads>{});

VectorClock::VectorClock() noexcept : clocks(s_default_clocks) {}

VectorClockPool::VectorClockPool()
{
  // Ensure that we won't reallocate these maps right away
  pool_.reserve(2048);
  lookup_.reserve(2048);

  // L'index 0 est explicitement réservé pour le VectorClock neutre/vide
  auto neutral                   = std::make_unique<VectorClock>();
  const VectorClock* neutral_ptr = neutral.get();

  pool_.push_back(std::move(neutral));
  lookup_[neutral_ptr] = 0;
}

index_t VectorClockPool::get_or_insert(const VectorClock& vc)
{
  auto it = lookup_.find(&vc);
  if (it != lookup_.end())
    return it->second;

  // Insert the new entry in the contents vector and the lookup map
  auto new_vc                   = std::make_unique<VectorClock>(vc);
  const VectorClock* new_vc_ptr = new_vc.get();
  index_t new_index;

  if (!free_list_.empty()) { // Reuse one of the free cells
    new_index = free_list_.back();
    free_list_.pop_back();
    pool_[new_index] = std::move(new_vc);

  } else { // No free cell => allocate a new one
    if (pool_.size() >= static_cast<std::size_t>(INVALID_INDEX)) [[unlikely]]
      throw std::overflow_error("The VectorClockPool is saturating with over 0x7FFFFFFF entries.");

    new_index = static_cast<index_t>(pool_.size());
    pool_.push_back(std::move(new_vc));
  }

  lookup_[new_vc_ptr] = new_index;

  return new_index;
}

const VectorClock& VectorClockPool::get(index_t index) const
{
  if (index >= pool_.size()) [[unlikely]] {
    throw std::out_of_range("VectorClockPool index out of bounds.");
  }
  return *pool_[index];
}

bool VectorClockPool::is_past(index_t index, const VectorClock& global_min) const noexcept
{
#if SIMGRID_HAVE_SMEMORY
  if (index >= pool_.size()) [[unlikely]]
    return false;

  // Aligned loads, as requested by AVX2
  const __m256i* a_ptr = reinterpret_cast<const __m256i*>(pool_[index]->clocks.data());
  const __m256i* b_ptr = reinterpret_cast<const __m256i*>(global_min.clocks.data());

  __m256i accumulator = _mm256_setzero_si256();

  constexpr std::size_t simd_chunks = smemory::config::max_threads / 8;
#pragma unroll
  for (std::size_t i = 0; i < simd_chunks; ++i) {
    __m256i a = _mm256_load_si256(a_ptr + i);
    __m256i b = _mm256_load_si256(b_ptr + i);

    /* TECHNICAL NOTE:
     * _mm256_cmpgt_epi32() does a signed comparison, but there is no equivalent for unsigned data (Epoch is an
     * uint32_t).
     *
     * It works for us anyway because every component of the VectorClock contains an Epoch.
     * Since that Epoch is really one and not an index in desguise, the MSB bit is 0.
     * So, _mm256_cmpgt_epi32() will be called only on positive values, making it correct provided that the VectorClock
     * does not contain garbage.
     */
    /* TECHNICAL NOTE:
     * The */
    __m256i gt  = _mm256_cmpgt_epi32(a, b);
    accumulator = _mm256_or_si256(accumulator, gt);
  }

  // _mm256_testz_si256 returns 1 of (accumulator & accumulator) == 0.
  // If at least one component of 'a' was strictly greater, some bits are on, and testz returns 0.
  return _mm256_testz_si256(accumulator, accumulator) != 0;
#else
  xbt_die("Using smemory without AVX2 is not supported yet");
#endif
}

void VectorClockPool::clean_before(const VectorClock& global_min) noexcept
{
  // Ignore index 0 that is the neutral VectorClock that must remain in place
  for (index_t i = 1; i < pool_.size(); ++i) {

    // Only test whether the cell is in the past if it's not already free
    if (pool_[i] != nullptr && is_past(i, global_min)) {

      lookup_.erase(pool_[i].get()); // Remove the object from the lookup table
      pool_[i].reset();              // Free the VC
      free_list_.push_back(i);       // Cache it for future allocations
    }
  }
}

} // namespace simgrid::mc::smemory