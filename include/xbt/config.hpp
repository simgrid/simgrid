/* Copyright (c) 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_CONFIG_HPP_
#define _XBT_CONFIG_HPP_

#include <cstdlib>

#include <functional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include <xbt/base.h>
#include <xbt/config.h>

namespace simgrid {
namespace config {

template<class T> inline
std::string to_string(T&& value)
{
  return std::to_string(std::forward<T>(value));
}
inline std::string const& to_string(std::string& value)
{
  return value;
}
inline std::string const& to_string(std::string const& value)
{
  return value;
}
inline std::string to_string(std::string&& value)
{
  return std::move(value);
}

// Get config

template<class T>
XBT_PUBLIC(T const&) getConfig(const char* name);

extern template XBT_PUBLIC(int const&) getConfig<int>(const char* name);
extern template XBT_PUBLIC(double const&) getConfig<double>(const char* name);
extern template XBT_PUBLIC(bool const&) getConfig<bool>(const char* name);
extern template XBT_PUBLIC(std::string const&) getConfig<std::string>(const char* name);

// Register:

/** Register a configuration flag
 *
 *  @param name        name of the option
 *  @param description Description of the option
 *  @param value       Initial/default value
 *  @param callback    called with the option value
 */
template<class T>
XBT_PUBLIC(void) declareFlag(const char* name, const char* description,
  T value, std::function<void(const T&)> callback = std::function<void(const T&)>());

extern template XBT_PUBLIC(void) declareFlag(const char* name,
  const char* description, int value, std::function<void(int const &)> callback);
extern template XBT_PUBLIC(void) declareFlag(const char* name,
  const char* description, double value, std::function<void(double const &)> callback);
extern template XBT_PUBLIC(void) declareFlag(const char* name,
  const char* description, bool value, std::function<void(bool const &)> callback);
extern template XBT_PUBLIC(void) declareFlag(const char* name,
  const char* description, std::string value, std::function<void(std::string const &)> callback);

/** Bind a variable to configuration flag
 *
 *  @param value Bound variable
 *  @param name  Flag name
 *  @param description Option description
 */
template<class T>
void bindFlag(T& value, const char* name, const char* description)
{
  using namespace std;
  declareFlag<T>(name, description, value, [&value](T const& val) {
    value = val;
  });
}

/** Bind a variable to configuration flag
 *
 *  <pre><code>
 *  static int x;
 *  simgrid::config::bindFlag(a, "x", [](const char* value) {
 *    return simgrid::config::parse(value);
 *  }
 *  </pre><code>
 */
// F is a parser, F : const char* -> T
template<class T, class F>
typename std::enable_if<std::is_same<
  T,
  typename std::remove_cv< decltype(
    std::declval<F>()(std::declval<const char*>())
  ) >::type
>::value, void>::type
bindFlag(T& value, const char* name, const char* description,
  F callback)
{
  declareFlag(name, description, value, [&value,callback](const char* val) {
    value = callback(val);
  });
}

/** Bind a variable to configuration flag
 *
 *  <pre><code>
 *  static int x;
 *  simgrid::config::bindFlag(a, "x", [](int x) {
 *    if (x < x_min || x => x_max)
 *      throw std::range_error("must be in [x_min, x_max)")
 *  });
 *  </pre><code>
 */
// F is a checker, F : T& -> ()
template<class T, class F>
typename std::enable_if<std::is_same<
  void,
  decltype( std::declval<F>()(std::declval<const T&>()) )
>::value, void>::type
bindFlag(T& value, const char* name, const char* description,
  F callback)
{
  declareFlag(name, description, value, [&value,callback](const T& val) {
    callback(val);
    value = std::move(val);
  });
}

/** Bind a variable to configuration flag
 *
 *  <pre><code>
 *  static int x;
 *  simgrid::config::bindFlag(a, "x", [](int x) { return return x > 0; });
 *  </pre><code>
 */
// F is a predicate, F : T const& -> bool
template<class T, class F>
typename std::enable_if<std::is_same<
  bool,
  decltype( std::declval<F>()(std::declval<const T&>()) )
>::value, void>::type
bindFlag(T& value, const char* name, const char* description,
  F callback)
{
  declareFlag(name, description, value, [&value,callback](const T& val) {
    if (!callback(val))
      throw std::range_error("invalid value");
    value = std::move(val);
  });
}

/** A variable bound to a CLI option
 *
 *  <pre><code>
 *  static simgrid::config::flag<int> answer("answer", "Expected answer", 42);
 *  static simgrid::config::flag<std::string> name("name", "Ford Prefect", "John Doe");
 *  static simgrid::config::flag<double> gamma("gamma", "Gamma factor", 1.987);
 *  </code></pre>
 */
template<class T>
class Flag {
  T value_;
public:

  /** Constructor
   *
   *  @param name  Flag name
   *  @param desc  Flag description
   *  @param value Flag initial/default value
   */
  Flag(const char* name, const char* desc, T value) : value_(value)
  {
    simgrid::config::bindFlag(value_, name, desc);
  }

  template<class F>
  Flag(const char* name, const char* desc, T value, F callback) : value_(value)
  {
    simgrid::config::bindFlag(value_, name, desc, std::move(callback));
  }

  // No copy:
  Flag(Flag const&) = delete;
  Flag& operator=(Flag const&) = delete;

  // Get the underlying value:
  T& get() { return value_; }
  T const& get() const { return value_; }

  // Implicit conversion to the underlying type:
  operator T&() { return value_; }
  operator T const&() const{ return value_; }

  // Basic interop with T:
  Flag& operator=(T const& that) { value_ = that; return *this; }
  Flag& operator=(T && that)     { value_ = that; return *this; }
  bool operator==(T const& that) const { return value_ == that; }
  bool operator!=(T const& that) const { return value_ != that; }
  bool operator<(T const& that) const { return value_ < that; }
  bool operator>(T const& that) const { return value_ > that; }
  bool operator<=(T const& that) const { return value_ <= that; }
  bool operator>=(T const& that) const { return value_ >= that; }
};

}
}

#endif
