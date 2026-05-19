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

namespace simgrid::mc {

// Data structure to implement FastTrack algorithm (see [Flanagan'09])
// Layout: [ AID (6 bits) | CLOCK (26 bits) ]
//          31          26   25             0

struct Epoch {
private:
  uint32_t data_;

  static constexpr int AID_BITS   = __builtin_ctz(smemory::config::max_threads);
  static constexpr int CLOCK_BITS = 32 - AID_BITS;

  static constexpr uint32_t CLOCK_MASK = (1U << CLOCK_BITS) - 1;
  static constexpr uint32_t AID_MASK   = (1U << AID_BITS) - 1;

public:
  Epoch() = delete; // The epoch must be correctly initialized
  constexpr Epoch(Aid aid, Clock clock)
      : // This is the constructor to use
      data_((static_cast<uint32_t>(aid.value()) << CLOCK_BITS) | (static_cast<uint32_t>(clock.value()) & CLOCK_MASK))
  {
  }
  // Forbids type coercisions: Epoch cannot be created from a literal
  explicit constexpr Epoch(uint32_t) = delete;

  // accessors
  constexpr Aid get_aid() const { return Aid{static_cast<int>(data_ >> CLOCK_BITS)}; }
  constexpr Clock get_clock() const { return Clock{static_cast<int>(data_ & CLOCK_MASK)}; }

  // Comparisons between epochs
  constexpr bool operator<(const Epoch& rhs) const { return data_ < rhs.data_; }
  constexpr bool operator==(const Epoch& rhs) const { return data_ == rhs.data_; }
  // Forbids comparison with integer types
  template <typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0> bool operator<(T) const = delete;
};

} // namespace simgrid::mc

#endif