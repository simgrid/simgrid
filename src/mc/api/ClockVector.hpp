/* Copyright (c) 2016-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_CLOCK_VECTOR_HPP
#define SIMGRID_MC_CLOCK_VECTOR_HPP

#include "src/mc/api/Aid.hpp"
#include "src/mc/api/Clock.hpp"
#include "src/mc/api/static_config.hpp"

#include <algorithm>
#include <vector>

namespace simgrid::mc {

/**
 * @brief A multi-dimensional vector used to keep track of happens-before relation between dependent events in an
 * execution of DPOR, SDPOR, or ODPOR. It is also used in the SlabTrack algorithm that detects dataraces
 *
 * Clock vectors permit actors in a distributed system to determine whether two events occurred one after the other but
 * they may not have); i.e. they permit actors to keep track of "time". A clever observation made in the original DPOR
 * paper is that a transition-based "happens-before" relation can be computed for any particular trace `S` using clock
 * vectors, effectively treating dependency like the passing of a message (the context in which vector clocks are
 * typically used).
 */
struct ClockVector final {
private:
  std::vector<Clock> contents_ = std::vector<Clock>(static_config::max_threads, Clock::INVALID);

public:
  ClockVector()                              = default;
  ClockVector(const ClockVector&)            = default;
  ClockVector& operator=(ClockVector const&) = default;
  ClockVector(ClockVector&&)                 = default;
  // ClockVector(std::initializer_list<std::pair<const mc_aid_t, uint32_t>> init) : contents_(std::move(init)) {}
  ClockVector(std::vector<Clock> init) : contents_(std::move(init)) {}
  ClockVector(Clock initial_value) : contents_(std::vector<Clock>(static_config::max_threads, initial_value)) {}

  bool empty() const { return this->contents_.empty(); }
  auto begin() const { return this->contents_.begin(); }
  auto end() const { return this->contents_.end(); }

  Clock& operator[](const Aid aid)
  {
    if (not aid.has_value())
      return Clock::INVALID;
    return contents_[aid.value()];
  }

  /**
   * @brief Retrieves the value mapped to the given actor if it is contained in this clock vector
   */
  const Clock get(Aid aid) const
  {
    if (not aid.has_value())
      return Clock::INVALID;
    if (aid.value() >= contents_.size())
      return Clock::INVALID;
    return contents_[aid.value()];
  }

  /**
   * @brief Computes a clock vector whose components are larger than the components of both of the given clock vectors
   *
   * The maximum of two clock vectors is definied to be the clock vector whose components are the maxmimum of the
   * corresponding components of the arguments. Since the `ClockVector` class is "lazy", the two clock vectors given as
   * arguments may not be of the same size. The resultant clock vector has components as follows:
   *
   * 1. For each actor that each clock vector maps, the resulting clock vector maps that thread to the maxmimum of the
   * values mapped for the actor in each clock vector
   *
   * 2. For each actor that only a single clock vector maps, the resulting clock vector maps that thread to the value
   * mapped by the lone clock vector
   *
   * The scheme is equivalent to assuming that an unmapped thread by any one clock vector is implicitly mapped to zero
   *
   * @param cv1 the first clock vector
   * @param cv2  the second clock vector
   * @return a clock vector whose components are at least as large as the corresponding components of each clock vector
   * and whose size is large enough to contain the union of components of each clock vector
   */
  static ClockVector max(const ClockVector& cv1, const ClockVector& cv2);

  inline void static max_emplace_left(ClockVector& cv1, const ClockVector& cv2)
  {
    std::transform(cv2.begin(), cv2.end(), cv1.contents_.begin(), cv1.contents_.begin(),
                   [](Clock a, Clock b) { return std::max(a, b); });
  }
};

} // namespace simgrid::mc

#endif
