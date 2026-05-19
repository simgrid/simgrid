/* Copyright (c) 2026-2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_CLOCK_HPP
#define SIMGRID_MC_CLOCK_HPP

#include <cstdint>
#include <type_traits>

namespace simgrid::mc {

// Helper function to prevent assigning a negative value to a Clock variable. If the value is negative, the code tries
// to raise an exception, which is forbidden in a constexpr, raising a compilation error
static constexpr int verify_clock_is_not_negative(int val)
{
  return (val >= 0) ? val : throw "Error: The value of a clock cannot be negative";
}

struct Clock {
private:
  static constexpr uint64_t INVALID_VALUE = static_cast<uint64_t>(~uint64_t(0));
  uint64_t value_;

public:
  static Clock INVALID;                        // Similar to std::nullopt, this represents an invalid value
  constexpr Clock() : value_(INVALID_VALUE) {} // The default constructor gives the invalid value
  // Constructor used for literals and constants
  constexpr Clock(int val) : value_(static_cast<uint64_t>(verify_clock_is_not_negative(val))) {}

  // The constructor for unsigned variables. This code would be simpler using C++20 concepts rather than using a type
  // template to trick the compiler into accepting two templated definitions of clock_t(T val) as we do here
  template <typename T, std::enable_if_t<std::is_integral_v<T> && std::is_unsigned_v<T>, int> = 0>
  constexpr Clock(T val) : value_(static_cast<uint64_t>(val))
  {
  }

  // The constructor for signed variables is removed
  // The use of void* ensures that the compiler sees the difference with the previous template, even if the int and the
  // void* are never used.
  template <typename T,
            std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T> && !std::is_same_v<T, int>, void*> = nullptr>
  Clock(T val) = delete;

  // These functions provide an API that is somewhat similar to std::optional
  constexpr bool has_value() const { return value_ != INVALID_VALUE; }
  constexpr uint64_t value() const
  {
    return value_ != INVALID_VALUE ? value_ : throw "Cannot extract the value of an invalid clock";
  }
  constexpr explicit operator bool() const { return has_value(); }

  constexpr void operator++()
  {
    if (value_ == INVALID_VALUE)
      throw "Cannot increase the value of the invalid clock";
    value_++;
  }
  constexpr void operator++(int) { ++*this; }
  constexpr bool operator!=(const Clock& rhs) const { return value_ != rhs.value_; }
  constexpr bool operator==(const Clock& rhs) const { return value_ == rhs.value_; }
  constexpr bool operator<(const Clock& rhs) const
  {
    if (not this->has_value())
      return rhs.has_value(); // INVALID is seen as the smallest possible value, despite its numerical value
    if (not rhs.has_value())
      return false; // this is valid
    return this->value_ < rhs.value_;
  }
};
}; // namespace simgrid::mc

#endif