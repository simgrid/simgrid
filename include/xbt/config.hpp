/* Copyright (c) 2016-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_CONFIG_HPP
#define XBT_CONFIG_HPP

#include <xbt/base.h>

#include <cstdlib>

#include <functional>
#include <initializer_list>
#include <map>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include <xbt/base.h>
#include <xbt/sysdep.h>

namespace simgrid {
namespace config {

class Config;

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

// Set default

template <class T> XBT_PUBLIC void set_default(const char* name, T value);

extern template XBT_PUBLIC void set_default<int>(const char* name, int value);
extern template XBT_PUBLIC void set_default<double>(const char* name, double value);
extern template XBT_PUBLIC void set_default<bool>(const char* name, bool value);
extern template XBT_PUBLIC void set_default<std::string>(const char* name, std::string value);

XBT_PUBLIC bool is_default(const char* name);

// Set config

template <class T> XBT_PUBLIC void set_value(const char* name, T value);

extern template XBT_PUBLIC void set_value<int>(const char* name, int value);
extern template XBT_PUBLIC void set_value<double>(const char* name, double value);
extern template XBT_PUBLIC void set_value<bool>(const char* name, bool value);
extern template XBT_PUBLIC void set_value<std::string>(const char* name, std::string value);

XBT_PUBLIC void set_as_string(const char* name, const std::string& value);
XBT_PUBLIC void set_parse(const std::string& options);

// Get config

template <class T> XBT_PUBLIC T const& get_value(const std::string& name);

extern template XBT_PUBLIC int const& get_value<int>(const std::string& name);
extern template XBT_PUBLIC double const& get_value<double>(const std::string& name);
extern template XBT_PUBLIC bool const& get_value<bool>(const std::string& name);
extern template XBT_PUBLIC std::string const& get_value<std::string>(const std::string& name);

// Register:

/** Register a configuration flag
 *
 *  @param name        name of the option
 *  @param description Description of the option
 *  @param value       Initial/default value
 *  @param callback    called with the option value
 */
template <class T>
XBT_PUBLIC void declare_flag(const std::string& name, const std::string& description, T value,
                             std::function<void(const T&)> callback = std::function<void(const T&)>());

extern template XBT_PUBLIC void declare_flag(const std::string& name, const std::string& description, int value,
                                             std::function<void(int const&)> callback);
extern template XBT_PUBLIC void declare_flag(const std::string& name, const std::string& description, double value,
                                             std::function<void(double const&)> callback);
extern template XBT_PUBLIC void declare_flag(const std::string& name, const std::string& description, bool value,
                                             std::function<void(bool const&)> callback);
extern template XBT_PUBLIC void declare_flag(const std::string& name, const std::string& description, std::string value,
                                             std::function<void(std::string const&)> callback);

// ***** alias *****

XBT_PUBLIC void alias(const char* realname, std::initializer_list<const char*> aliases);

/** Bind a variable to configuration flag
 *
 *  @param value Bound variable
 *  @param name  Flag name
 *  @param description Option description
 */
template <class T> void bind_flag(T& value, const char* name, const char* description)
{
  declare_flag<T>(name, description, value, [&value](T const& val) { value = val; });
}

template <class T>
void bind_flag(T& value, const char* name, std::initializer_list<const char*> aliases, const char* description)
{
  bind_flag(value, name, description);
  alias(name, aliases);
}

/** Bind a variable to configuration flag
 *
 *  <pre><code>
 *  static int x;
 *  simgrid::config::bind_flag(a, "x", [](int x) {
 *    if (x < x_min || x => x_max)
 *      throw std::range_error("must be in [x_min, x_max)")
 *  });
 *  </code></pre>
 */
// F is a checker, F : T& -> ()
template <class T, class F>
typename std::enable_if<std::is_same<void, decltype(std::declval<F>()(std::declval<const T&>()))>::value, void>::type
bind_flag(T& value, const char* name, const char* description, F callback)
{
  declare_flag(name, description, value, std::function<void(const T&)>([&value, callback](const T& val) {
                 callback(val);
                 value = std::move(val);
               }));
}

template <class T, class F>
typename std::enable_if<std::is_same<void, decltype(std::declval<F>()(std::declval<const T&>()))>::value, void>::type
bind_flag(T& value, const char* name, std::initializer_list<const char*> aliases, const char* description, F callback)
{
  bind_flag(value, name, description, std::move(callback));
  alias(name, aliases);
}

template <class F>
typename std::enable_if<std::is_same<void, decltype(std::declval<F>()(std::declval<const std::string&>()))>::value,
                        void>::type
bind_flag(std::string& value, const char* name, const char* description,
          const std::map<std::string, std::string, std::less<>>& valid_values, F callback)
{
  declare_flag(name, description, value,
               std::function<void(const std::string&)>([&value, name, valid_values, callback](const std::string& val) {
                 callback(val);
                 if (valid_values.find(val) != valid_values.end()) {
                   value = std::move(val);
                   return;
                 }
                 std::string mesg = "\n";
                 if (val == "help")
                   mesg += std::string("Possible values for option ") + name + ":\n";
                 else
                   mesg += std::string("Invalid value '") + val + "' for option " + name + ". Possible values:\n";
                 for (auto const& kv : valid_values)
                   mesg += "  - '" + kv.first + "': " + kv.second + (kv.first == value ? "  <=== DEFAULT" : "") + "\n";
                 xbt_die("%s", mesg.c_str());
               }));
}
template <class F>
typename std::enable_if<std::is_same<void, decltype(std::declval<F>()(std::declval<const std::string&>()))>::value,
                        void>::type
bind_flag(std::string& value, const char* name, std::initializer_list<const char*> aliases, const char* description,
          const std::map<std::string, std::string, std::less<>>& valid_values, F callback)
{
  bind_flag(value, name, description, valid_values, std::move(callback));
  alias(name, aliases);
}

/** Bind a variable to configuration flag
 *
 *  <pre><code>
 *  static int x;
 *  simgrid::config::bind_flag(a, "x", [](int x) { return x > 0; });
 *  </code></pre>
 */
// F is a predicate, F : T const& -> bool
template <class T, class F>
typename std::enable_if<std::is_same<bool, decltype(std::declval<F>()(std::declval<const T&>()))>::value, void>::type
bind_flag(T& value, const char* name, const char* description, F callback)
{
  declare_flag(name, description, value, std::function<void(const T&)>([&value, callback](const T& val) {
                 if (not callback(val))
                   throw std::range_error("invalid value.");
                 value = std::move(val);
               }));
}

/** A variable bound to a CLI option
 *
 *  <pre><code>
 *  static simgrid::config::flag<int> answer("answer", "Expected answer", 42);
 *  static simgrid::config::flag<std::string> name("name", "Ford Perfect", "John Doe");
 *  static simgrid::config::flag<double> gamma("gamma", "Gamma factor", 1.987);
 *  </code></pre>
 */
template<class T>
class Flag {
  T value_;
  std::string name_;

public:
  /** Constructor
   *
   *  @param name  Flag name
   *  @param desc  Flag description
   *  @param value Flag initial/default value
   */
  Flag(const char* name, const char* desc, T value) : value_(value), name_(name)
  {
    simgrid::config::bind_flag(value_, name, desc);
  }

  /** Constructor taking also an array of aliases for name */
  Flag(const char* name, std::initializer_list<const char*> aliases, const char* desc, T value)
      : value_(value), name_(name)
  {
    simgrid::config::bind_flag(value_, name, aliases, desc);
  }

  /* A constructor accepting a callback that will be passed the parameter.
   * It can either return a boolean (informing whether the parameter is valid), or returning void.
   */
  template <class F> Flag(const char* name, const char* desc, T value, F callback) : value_(value), name_(name)
  {
    simgrid::config::bind_flag(value_, name, desc, std::move(callback));
  }

  template <class F>
  Flag(const char* name, std::initializer_list<const char*> aliases, const char* desc, T value, F callback)
      : value_(value), name_(name)
  {
    simgrid::config::bind_flag(value_, name, aliases, desc, std::move(callback));
  }

  /* A constructor accepting a map of valid values -> their description,
   * and producing an informative error message when an invalid value is passed, or when help is passed as a value.
   */
  template <class F>
  Flag(const char* name, const char* desc, T value, const std::map<std::string, std::string, std::less<>>& valid_values,
       F callback)
      : value_(value), name_(name)
  {
    simgrid::config::bind_flag(value_, name, desc, std::move(valid_values), std::move(callback));
  }

  /* A constructor with everything */
  template <class F>
  Flag(const char* name, std::initializer_list<const char*> aliases, const char* desc, T value,
       const std::map<std::string, std::string, std::less<>>& valid_values, F callback)
      : value_(value), name_(name)
  {
    simgrid::config::bind_flag(value_, name, aliases, desc, valid_values, std::move(callback));
  }

  // No copy:
  Flag(Flag const&) = delete;
  Flag& operator=(Flag const&) = delete;

  // Get the underlying value:
  T& get() { return value_; }
  T const& get() const { return value_; }

  const std::string& get_name() const { return name_; }
  // Implicit conversion to the underlying type:
  operator T&() { return value_; }
  operator T const&() const{ return value_; }

  // Basic interop with T:
  template<class U>
  Flag& operator=(U const& that) { value_ = that; return *this; }
  template<class U>
  Flag& operator=(U&& that) { value_ = std::forward<U>(that); return *this; }
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

XBT_PUBLIC void finalize();
XBT_PUBLIC void show_aliases();
XBT_PUBLIC void help();
} // namespace config
} // namespace simgrid

#endif
