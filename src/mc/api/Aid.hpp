/* Copyright (c) 2026-2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_AID_HPP
#define SIMGRID_MC_AID_HPP

#include "src/mc/smemory/smemory_config.hpp"
#include "xbt/asserts.h"
#include "xbt/log.h"
#include <cstdint>
#include <functional> // std::hash
#include <string>
#include <type_traits>

namespace simgrid::mc {
class AidCannotBeNegative : std::exception {
  std::string reason_;

public:
  AidCannotBeNegative(int id)
      : reason_("Error: The value of an aid cannot be negative, so " + std::to_string(id) + " is invalid.")
  {
    xbt_backtrace_display_current();
  }
  const char* what() const noexcept override { return reason_.c_str(); }
};
class AidCannotBeAboveMaxThreads : std::exception {
  std::string reason_;

public:
  AidCannotBeAboveMaxThreads(int id)
      : reason_(std::string("The model-checker assumes that no actor ID will ever be larger than ") +
                std::to_string(mc::smemory::config::max_threads - 1) +
                ". Change max_threads in "
                "src/mc/smemory/smemory_config.hpp to verify larger applications (you seem to need at least " +
                std::to_string(id) +
                "), but be warned that it will slow "
                "down the exploration while the state space of such a large application is probably be too large "
                "to be explored anyway. Slowing down the exploration is thus both inevitable and a bad idea in "
                "this case.")
  {
  }
  const char* what() const noexcept override { return reason_.c_str(); }
};

// Helper function to prevent assigning a negative value to a Aid. If the value is negative, the code tries to raise an
// exception, which is forbidden in a constexpr, raising a compilation error
static constexpr int verify_aid_is_valid(int val)
{
  if (val >= simgrid::mc::smemory::config::max_threads - 1) { // -1 because we need a value for the INVALID_VALUE
    xbt_backtrace_display_current();
    XBT_CCRITICAL(root, "Aid is %d, which is above config::max_threads", val);
    throw AidCannotBeAboveMaxThreads(val);
  }
  return (val >= 0) ? val : throw AidCannotBeNegative(val);
}

struct Aid {
  using storage_type =
      std::conditional_t<(smemory::config::max_threads <= 256), std::uint8_t,
                         std::conditional_t<(smemory::config::max_threads <= 65536), std::uint16_t, std::uint32_t>>;

private:
  storage_type value_;

public:
  static Aid INVALID_VALUE;                         // Similar to std::nullopt, this represents an invalid value
  constexpr Aid() : value_(INVALID_VALUE.value_) {} // The default constructor gives the invalid value
  // Constructor used for literals and constants
  constexpr Aid(
      int val,
      bool check_validity = true) // You should always check validity. The only exception is when building INVALID_VALUE
      : value_(static_cast<storage_type>(check_validity ? verify_aid_is_valid(val) : val))
  {
  }

  // The constructor for unsigned variables. This code would be simpler using C++20 concepts rather than using a type
  // template to trick the compiler into accepting two templated definitions of clock_t(T val) as we do here
  template <typename T, std::enable_if_t<std::is_integral_v<T> && std::is_unsigned_v<T>, int> = 0>
  constexpr Aid(T val) : value_(static_cast<storage_type>(val))
  {
  }

  // The constructor for signed variables is removed
  // The use of void* ensures that the compiler sees the difference with the previous template, even if the int and the
  // void* are never used.
  template <typename T,
            std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T> && !std::is_same_v<T, int>, void*> = nullptr>
  Aid(T val) = delete;

  // These functions provide an API that is somewhat similar to std::optional
  constexpr bool has_value() const { return value_ != INVALID_VALUE.value_; }
  constexpr storage_type value() const
  {
    if (value_ != INVALID_VALUE.value_)
      return value_;
    xbt_backtrace_display_current();
    throw "Cannot extract the value of an invalid aid";
  }
  constexpr int c_val() const { return value_ != INVALID_VALUE.value_ ? value_ : -1; }
  constexpr explicit operator bool() const { return has_value(); }

  // Forbid comparison with integer types
  template <typename T, std::enable_if_t<std::is_integral_v<T>>> bool operator<(T) const  = delete;
  template <typename T, std::enable_if_t<std::is_integral_v<T>>> bool operator==(T) const = delete;

  // Comparisons between aids
  constexpr bool operator!=(const Aid& rhs) const { return value_ != rhs.value_; }
  constexpr bool operator==(const Aid& rhs) const { return value_ == rhs.value_; }
  constexpr bool operator<(const Aid& rhs) const
  {
    if (not this->has_value())
      return rhs.has_value(); // INVALID is seen as the smallest possible value, despite its numerical value
    if (not rhs.has_value())
      return false; // this is valid
    return this->value_ < rhs.value_;
  }
  // Arithmetics on aids
  constexpr Aid operator++(int)
  {
    if (value_ == INVALID_VALUE.value_)
      throw "Cannot increase the value of the invalid AID";
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