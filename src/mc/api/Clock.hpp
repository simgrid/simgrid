/* Copyright (c) 2026-2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_CLOCK_HPP
#define SIMGRID_MC_CLOCK_HPP

#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace simgrid::mc {
class InvalidClock : public std::logic_error {
public:
  explicit InvalidClock(const std::string& reason) : std::logic_error(reason) {}
};

// Helper function to prevent assigning a negative value to a Clock variable. If the value is negative, the code tries
// to raise an exception, which is forbidden in a constexpr, raising a compilation error
static constexpr int verify_clock_is_not_negative(int val)
{
  return (val >= 0) ? val : throw InvalidClock("Error: The value of a clock cannot be negative");
}

struct Clock {
  using storage_type = unsigned int; // No need to make it big: It must fit in an uint32 for Epoch

private:
  static constexpr storage_type INVALID_VALUE = ~storage_type(0);
  storage_type value_;

public:
  static Clock INVALID;                        // Similar to std::nullopt, this represents an invalid value
  constexpr Clock() : value_(INVALID_VALUE) {} // The default constructor gives the invalid value
  // Constructor used for literals and constants
  constexpr Clock(int val) : value_(static_cast<storage_type>(verify_clock_is_not_negative(val))) {}

  // The constructor for unsigned variables.
  template <typename T>
    requires std::integral<T> && std::unsigned_integral<T>
  constexpr Clock(T val) : value_(static_cast<storage_type>(val))
  {
  }

  // The constructor for signed variables is removed
  template <typename T>
    requires std::integral<T> && std::signed_integral<T>
  Clock(T val) = delete;

  // These functions provide an API that is somewhat similar to std::optional
  constexpr bool has_value() const { return value_ != INVALID_VALUE; }
  constexpr storage_type value() const
  {
    return value_ != INVALID_VALUE ? value_ : throw InvalidClock("Cannot extract the value of an invalid clock");
  }
  constexpr explicit operator bool() const { return has_value(); }

  constexpr void operator++()
  {
    if (value_ == INVALID_VALUE)
      throw InvalidClock("Cannot increase the value of the invalid clock");
    value_++;
  }
  constexpr void operator++(int) { ++*this; }
  constexpr std::strong_ordering operator<=>(const Clock& rhs) const noexcept
  {
    // INVALID is seen as the smallest possible value, despite its numerical value
    if (not this->has_value())
      return not rhs.has_value() ? std::strong_ordering::equal : std::strong_ordering::less;
    if (not rhs.has_value())
      return std::strong_ordering::greater;
    return this->value_ <=> rhs.value_;
  }
  constexpr bool operator==(const Clock& rhs) const noexcept = default;
};
}; // namespace simgrid::mc

#endif