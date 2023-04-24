/* Copyright (c) 2016-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_CLOCK_VECTOR_HPP
#define SIMGRID_MC_CLOCK_VECTOR_HPP

#include "simgrid/forward.h"

#include <cstdint>
#include <optional>
#include <unordered_map>

namespace simgrid::mc {

/**
 * @brief A multi-dimensional vector used to keep track of
 * happens-before relation between dependent events in an
 * execution of DPOR, SDPOR, or ODPOR
 */
struct ClockVector final {
private:
  std::unordered_map<aid_t, uint32_t> contents;

public:
  ClockVector()                              = default;
  ClockVector(const ClockVector&)            = default;
  ClockVector& operator=(ClockVector const&) = default;
  ClockVector(ClockVector&&)                 = default;

  /**
   * @brief The number of components in this
   * clock vector
   *
   * A `ClockVector` implicitly maps the id of an actor
   * it does not contain to a default value of `0`.
   * Thus, a `ClockVector` is "lazy" in the sense
   * that new actors are "automatically" mapped
   * without needing to be explicitly added the clock
   * vector when the actor is created. This means that
   * comparison between clock vectors is possible
   * even as actors become enabled and disabled
   *
   * @return uint32_t the number of elements in
   * the clock vector
   */
  size_t size() const { return this->contents.size(); }

  uint32_t& operator[](aid_t aid)
  {
    // NOTE: The `operator[]` overload of
    // unordered_map will create a new key-value
    // pair if `tid` does not exist and will use
    // a _default_ value for the value (0 in this case)
    // which is precisely what we want here
    return this->contents[aid];
  }

  /**
   * @brief Retrieves the value mapped to the given
   * actor if it is contained in this clock vector
   */
  std::optional<uint32_t> get(aid_t aid) const
  {
    if (const auto iter = this->contents.find(aid); iter != this->contents.end())
      return std::optional<uint32_t>{iter->second};
    return std::nullopt;
  }

  /**
   * @brief Computes a clock vector whose components
   * are larger than the components of both of
   * the given clock vectors
   *
   * The maximum of two clock vectors is definied to
   * be the clock vector whose components are the maxmimum
   * of the corresponding components of the arguments.
   * Since the `ClockVector` class is "lazy", the two
   * clock vectors given as arguments may not be of the same size.
   * The resultant clock vector has components as follows:
   *
   * 1. For each actor that each clock vector maps, the
   * resulting clock vector maps that thread to the maxmimum
   * of the values mapped for the actor in each clock vector
   *
   * 2. For each actor that only a single clock vector maps,
   * the resulting clock vector maps that thread to the
   * value mapped by the lone clock vector
   *
   * The scheme is equivalent to assuming that an unmapped
   * thread by any one clock vector is implicitly mapped to zero
   *
   * @param cv1 the first clock vector
   * @param cv2  the second clock vector
   * @return a clock vector whose components are at
   * least as large as the corresponding components of each clock
   * vector and whose size is large enough to contain the union
   * of components of each clock vector
   */
  static ClockVector max(const ClockVector& cv1, const ClockVector& cv2);
};

} // namespace simgrid::mc

#endif
