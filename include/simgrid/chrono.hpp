/* Copyright (c) 2016-2021. The SimGrid Team. All rights reserved.          */

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

#include <simgrid/engine.h>

namespace simgrid {

/** @brief A C++ compatible [TrivialClock](http://en.cppreference.com/w/cpp/concept/TrivialClock) working with simulated-time.
 *
 * SimGrid uses `double` for representing the simulated time, where *durations* are expressed in seconds
 * (with *infinite duration* expressed as a negative value) and
 * *timepoints* are expressed as seconds from the beginning of the simulation.
 * In contrast, all the C++ APIs use the much more sensible `std::chrono::duration` and`std::chrono::time_point`.
 *
 * This class can be used to build `std::chrono` objects that use simulated time,
 * using #SimulationClockDuration and #SimulationClockTimePoint.
 *
 * This means it is possible to use (since C++14):
 *
 * @code{cpp}
 * using namespace std::chrono_literals;
 * simgrid::s4u::actor::sleep_for(42s);
 * @endcode
 *
 */
class SimulationClock {
public:
  using rep        = double;
  using period     = std::ratio<1>;
  using duration   = std::chrono::duration<rep, period>;
  using time_point = std::chrono::time_point<SimulationClock, duration>;
  static constexpr bool is_steady = true;
  static time_point now() { return time_point(duration(simgrid_get_clock())); }
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
