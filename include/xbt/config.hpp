/* Copyright (c) 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_CONFIG_HPP_
#define _XBT_CONFIG_HPP_

#include <xbt/base.h>

#include <cstdlib>

#include <functional>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include <xbt/base.h>
#include <xbt/config.h>

namespace simgrid {
namespace config {

XBT_PUBLIC_CLASS missing_key_error : public std::runtime_error {
public:
  explicit missing_key_error(const std::string& what)
    : std::runtime_error(what) {}
  explicit missing_key_error(const char* what)
    : std::runtime_error(what) {}
  ~missing_key_error() override;
};

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

// ***** alias *****

XBT_PUBLIC(void) alias(const char* realname, const char* aliasname);

inline
void alias(std::initializer_list<const char*> names)
{
  auto i = names.begin();
  for (++i; i != names.end(); ++i)
    alias(*names.begin(), *i);
}

/** Bind a variable to configuration flag
 *
 *  @param value Bound variable
 *  @param name  Flag name
 *  @param description Option description
 */
template<class T>
void bindFlag(T& value, const char* name, const char* description)
{
  declareFlag<T>(name, description, value, [&value](T const& val) {
    value = val;
  });
}

template<class T>
void bindFlag(T& value, std::initializer_list<const char*> names, const char* description)
{
  bindFlag(value, *names.begin(), description);
  alias(names);
}

/** Bind a variable to configuration flag
 *
 *  <pre><code>
 *  static int x;
 *  simgrid::config::bindFlag(a, "x", [](int x) {
 *    if (x < x_min || x => x_max)
 *      throw std::range_error("must be in [x_min, x_max)")
 *  });
 *  </code></pre>
 */
// F is a checker, F : T& -> ()
template<class T, class F>
typename std::enable_if<std::is_same<
  void,
  decltype( std::declval<F>()(std::declval<const T&>()) )
>::value, void>::type
bindFlag(T& value, std::initializer_list<const char*> names, const char* description,
  F callback)
{
  bindFlag(value, *names.begin(), description);
  alias(names);
}

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
 *  </code></pre>
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
  template<class U>
  Flag& operator=(U const& that) { value_ = that; return *this; }
  template<class U>
  Flag& operator=(U && that)     { value_ = that; return *this; }
  template<class U>
  bool operator==(U const& that) const { return value_ == that; }
  template<class U>
  bool operator!=(U const& that) const { return value_ != that; }
  template<class U>
  bool operator<(U const& that) const { return value_ < that; }
  template<class U>
  bool operator>(U const& that) const { return value_ > that; }
  template<class U>
  bool operator<=(U const& that) const { return value_ <= that; }
  template<class U>
  bool operator>=(U const& that) const { return value_ >= that; }
};

}
}

#endif
