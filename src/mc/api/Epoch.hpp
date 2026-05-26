/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This type encodes both an AID and a clock in the same integer, for a compact implementation of the FastTrack
 * algorithm */

#ifndef SIMGRID_MC_EPOCH_HPP
#define SIMGRID_MC_EPOCH_HPP

#include "src/mc/api/Aid.hpp"
#include "src/mc/api/Clock.hpp"
#include "src/mc/smemory/smemory_config.hpp"
#include "xbt/asserts.h"
#include <cstdint>
#include <stdexcept>
#include <string>

namespace simgrid::mc {

// Data structure to implement FastTrack algorithm (see [Flanagan'09])
// It can be either a real epoch, merging the AID and its clock in the same uint32, or an index into a VectorClockPool.
// The first bit is the selector.
//
// Layout of an Epoch:     [ 0 (1 bit) | [ AID (6 bits) | CLOCK (24 bits) ]
//                         [31       31|30            25|24              0]
//
// Layout of a pool index: [ 1 (1 bit) |          index (31 bits)         ]
//                         [31       31|30                               0]

struct EpochIsNotPureIndex : public std::logic_error {
  EpochIsNotPureIndex(const std::string& reason) : std::logic_error(reason) {}
};

struct Epoch {
private:
  uint32_t data_;

  static constexpr uint32_t SELECTOR_MASK = 1U << 31; // Bit 31 is the selector bit

  static constexpr int AID_BITS   = __builtin_ctz(smemory::config::max_threads);
  static constexpr int CLOCK_BITS = 31 - AID_BITS;

  static constexpr uint32_t CLOCK_MASK = (1U << CLOCK_BITS) - 1;
  static constexpr uint32_t AID_MASK   = (1U << AID_BITS) - 1;
  static constexpr uint32_t INDEX_MASK = (1U << 31) - 1;

public:
  Epoch() = delete; // The epoch must be correctly initialized
  explicit constexpr Epoch(Aid aid, Clock clock) // This is the constructor to use for real Epochs
      : data_((static_cast<uint32_t>(aid.value()) << CLOCK_BITS) | (static_cast<uint32_t>(clock.value()) & CLOCK_MASK))
  {
  }
  explicit constexpr Epoch(uint32_t idx) : data_(idx)
  {
    if ((idx & SELECTOR_MASK) != 0)
      throw EpochIsNotPureIndex("Cannot create an Epoch from an uint32 that is not a pure index.");
  }

  // Differenciate between indexes and real epochs
  constexpr bool is_pure_epoch() const noexcept { return (data_ & SELECTOR_MASK) == 0; }
  constexpr bool is_pool_index() const noexcept { return not is_pure_epoch(); }

  // accessors
  constexpr uint32_t get_pool_index() const
  {
    if (!is_pool_index()) [[unlikely]] {
      throw std::logic_error("This Epoch object is not a pool index in desguise");
    }
    return static_cast<uint32_t>(data_ & INDEX_MASK);
  }
  constexpr Aid get_aid() const
  {
    if (is_pool_index()) [[unlikely]]
      throw std::logic_error("An Epoch does not contain an aid when it's a pool index in desguise.");
    return Aid{static_cast<int>(data_ >> CLOCK_BITS)};
  }
  constexpr Clock get_clock() const
  {
    if (is_pool_index()) [[unlikely]]
      throw std::logic_error("An Epoch does not contain a clock when it's a pool index in desguise.");
    return Clock{static_cast<int>(data_ & CLOCK_MASK)};
  }

  // Comparisons between epochs
  constexpr bool operator<(const Epoch& rhs) const { return data_ < rhs.data_; }
  constexpr bool operator==(const Epoch& rhs) const { return data_ == rhs.data_; }
  // Forbids comparison with integer types
  template <typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0> bool operator<(T) const = delete;
};

} // namespace simgrid::mc

#endif