/* Copyright (c) 2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_CHRONO_HPP
#define SIMGRID_CHRONO_HPP

/** @file chrono.hpp Time support
 *
 *  Define clock, duration types, time point types compatible with the standard
 *  C++ library API.
 */

#include <chrono>
#include <ratio>

#include <simgrid/simix.h>

namespace simgrid {

/** A C++ compatible TrivialClock working with simulated-time */
struct SimulationClock {
  using rep        = double;
  using period     = std::ratio<1>;
  using duration   = std::chrono::duration<rep, period>;
  using time_point = std::chrono::time_point<SimulationClock, duration>;
  static constexpr bool is_steady = true;
  static time_point now()
  {
    return time_point(duration(SIMIX_get_clock()));
  }
};

/** Default duration for simulated time */
using SimulationClockDuration  = SimulationClock::duration;

/** Default time point for simulated time */
using SimulationClockTimePoint = SimulationClock::time_point;

// Durations based on doubles:
using nanoseconds = std::chrono::duration<double, std::nano>;
using microseconds = std::chrono::duration<double, std::micro>;
using milliseconds = std::chrono::duration<double, std::milli>;
using seconds = std::chrono::duration<double>;
using minutes = std::chrono::duration<double, std::ratio<60>>;
using hours = std::chrono::duration<double, std::ratio<3600>>;

/** A time point in the simulated time */
template<class Duration>
using SimulationTimePoint = std::chrono::time_point<SimulationClock, Duration>;

}

#endif
