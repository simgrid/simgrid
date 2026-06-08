/* Copyright (c) 2026-2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_AID_HPP
#define SIMGRID_MC_AID_HPP

#include "src/mc/smemory/smemory_config.hpp"
#include "xbt/asserts.h"
#include "xbt/string.hpp"

#include <compare>
#include <cstdint>
#include <functional> // std::hash
#include <stdexcept>
#include <string>
#include <type_traits>

namespace simgrid::mc {
class AidCannotBeNegative : public std::logic_error {
public:
  explicit AidCannotBeNegative(int id)
      : std::logic_error(
            simgrid::xbt::string_printf("Error: The value of an aid cannot be negative, so %d is invalid.", id))
  {
    xbt_backtrace_display_current();
  }
};
class AidCannotBeAboveMaxThreads : public std::logic_error {
public:
  explicit AidCannotBeAboveMaxThreads(int id)
      : std::logic_error(simgrid::xbt::string_printf(
            "The model-checker assumes that no actor ID will ever be larger than %d. Change max_threads in "
            "src/mc/smemory/smemory_config.hpp to verify larger applications (you seem to need at least %d), "
            "but be warned that it will slow down the exploration while the state space of such a large application is "
            "probably be too large to be explored anyway. Slowing down the exploration is thus both inevitable and a "
            "bad idea in this case.",
            mc::smemory::config::max_threads - 1, id))
  {
  }
};
class InvalidAid : public std::logic_error {
public:
  explicit InvalidAid(const std::string& reason) : std::logic_error(reason) {}
};

// Helper function to prevent assigning a negative value to a Aid. If the value is negative, the code tries to raise an
// exception, which is forbidden in a constexpr, raising a compilation error
static constexpr int verify_aid_is_valid(int val)
{
  if (val >= simgrid::mc::smemory::config::max_threads - 1) { // -1 because we need a value for the INVALID_VALUE
    xbt_backtrace_display_current();
    throw AidCannotBeAboveMaxThreads(val);
  }
  return (val >= 0) ? val : throw AidCannotBeNegative(val);
}

struct Aid {
  using storage_type =
      std::conditional_t<(smemory::config::max_threads <= 256), std::uint8_t,
                         std::conditional_t<(smemory::config::max_threads <= 65536), std::uint16_t, std::uint32_t>>;

private:
  // Getting g++12 to consider this as a constexpr requires to give it a complete type such as storage_type. Aid cannot
  // be used
  static constexpr storage_type INVALID_VALUE = static_cast<storage_type>(smemory::config::max_threads - 1);
  storage_type value_;

public:
  static Aid INVALID;                        // Similar to std::nullopt, this represents an invalid value
  constexpr Aid() : value_(INVALID_VALUE) {} // The default constructor gives the invalid value
  // Constructor used for literals and constants
  explicit constexpr Aid(
      int val,
      bool check_validity = true) // You should always check validity. The only exception is when building INVALID_VALUE
      : value_(static_cast<storage_type>(check_validity ? verify_aid_is_valid(val) : val))
  {
  }

  // The constructor for unsigned variables.
  template <typename T>
    requires std::integral<T> && std::unsigned_integral<T>
  constexpr Aid(T val) : value_(static_cast<storage_type>(val))
  {
  }

  // The constructor for signed variables is removed
  template <typename T>
    requires std::integral<T> && std::signed_integral<T>
  Aid(T val) = delete;

  // These functions provide an API that is somewhat similar to std::optional
  constexpr bool has_value() const { return value_ != INVALID_VALUE; }
  constexpr storage_type value() const
  {
    if (value_ != INVALID_VALUE)
      return value_;
    throw InvalidAid("Cannot extract the value of an invalid aid");
  }
  constexpr int c_val() const { return value_ != INVALID_VALUE ? value_ : -1; }
  constexpr explicit operator bool() const { return has_value(); }

  // Forbid comparison with integer types
  template <typename T>
    requires std::integral<T> && std::signed_integral<T>
  bool operator<(T) const = delete;
  template <typename T>
    requires std::integral<T> && std::signed_integral<T>
  bool operator==(T) const = delete;

  // Comparisons between aids
  constexpr std::strong_ordering operator<=>(const Aid& rhs) const
  {
    // INVALID is seen as the smallest possible value, despite its numerical value
    if (not this->has_value() && not rhs.has_value())
      return std::strong_ordering::equivalent;
    if (not this->has_value())
      return std::strong_ordering::less; // INVALID < any valid value
    if (not rhs.has_value())
      return std::strong_ordering::greater; // any valid value > INVALID
    return this->value_ <=> rhs.value_;
  }
  constexpr bool operator==(const Aid& rhs) const = default;
  constexpr bool operator!=(const Aid& rhs) const = default;
  // Arithmetics on aids
  constexpr Aid operator++(int)
  {
    if (value_ == INVALID_VALUE)
      throw InvalidAid("Cannot increase the value of the invalid AID");
    Aid temp = *this;
    value_++;
    return temp;
  }
};
}; // namespace simgrid::mc

namespace std {
template <> struct hash<simgrid::mc::Aid> {
  std::size_t operator()(const simgrid::mc::Aid& aid) const noexcept
  {
    return aid.has_value() ? std::hash<int>{}(aid.c_val()) : std::size_t(-1);
  }
};
} // namespace std
#endif