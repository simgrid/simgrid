/* Copyright (c) 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_CONFIG_HPP_
#define _XBT_CONFIG_HPP_

#include <functional>
#include <string>
#include <type_traits>

#include <xbt/base.h>
#include <xbt/config.h>

namespace simgrid {
namespace config {

/** Get the base type of a e_xbt_cfgelm_type_t
 *
 *  * `type` is the type used in the config framework to store
 *     the values of this type;
 *
 *  * `get()` is used to get such a value from the configuration.
 */
template<e_xbt_cfgelm_type_t type>
struct base_type;
template<> struct base_type<xbt_cfgelm_boolean> {
  typedef bool type;
  static inline bool get(const char* name)
  {
    return xbt_cfg_get_boolean(name) ? true : false;
  }
};
template<> struct base_type<xbt_cfgelm_int> {
  typedef int type;
  static inline int get(const char* name)
  {
    return xbt_cfg_get_int(name);
  }
};
template<> struct base_type<xbt_cfgelm_double> {
  typedef double type;
  static inline double get(const char* name)
  {
    return xbt_cfg_get_double(name);
  }
};
template<> struct base_type<xbt_cfgelm_string> {
  typedef std::string type;
  static inline std::string get(const char* name)
  {
    char* value = xbt_cfg_get_string(name);
    return value != nullptr ? value : "";
  }
};

/** Associate the e_xbt_cfgelm_type_t to use for a given type */
template<class T>
struct cfgelm_type : public std::integral_constant<e_xbt_cfgelm_type_t,
  std::is_same<T, bool>::value ? xbt_cfgelm_boolean :
  std::is_integral<T>::value ? xbt_cfgelm_int :
  std::is_floating_point<T>::value ? xbt_cfgelm_double :
  xbt_cfgelm_string>
{};

/** Get a value of a given type from the configuration */
template<class T> inline
T get(const char* name)
{
  return T(base_type<cfgelm_type<T>::value>::get(name));
}
template<class T> inline
T get(std::string const& name)
{
  return T(base_type<cfgelm_type<T>::value>::get(name.c_str()));
}

inline void setDefault(const char* name, int value)
{
  xbt_cfg_setdefault_int(name, value);
}
inline void setDefault(const char* name, double value)
{
  xbt_cfg_setdefault_double(name, value);
}
inline void setDefault(const char* name, const char* value)
{
  xbt_cfg_setdefault_string(name, value);
}
inline void setDefault(const char* name, std::string const& value)
{
  xbt_cfg_setdefault_string(name, value.c_str());
}
inline void setDefault(const char* name, bool value)
{
  xbt_cfg_setdefault_boolean(name, value ? "yes" : "no");
}

/** Set the default value of a given configuration option */
template<class T> inline
void setDefault(const char* name, T value)
{
  setDefault(name, base_type<cfgelm_type<T>::value>::type(value));
}

// Register:

/** Register a configuration flag
 *
 *  @param name        name of the option
 *  @param description Description of the option
 *  @param type        config storage type for the option
 *  @param callback    called with the option name (expected to use `simgrid::config::get`)
 */
XBT_PUBLIC(void) registerConfig(const char* name, const char* description,
  e_xbt_cfgelm_type_t type,
  std::function<void(const char*)> callback);

/** Bind a variable to configuration flag
 *
 *  @param value Bound variable
 *  @param name  Flag name
 *  @param description Option description
 */
template<class T>
void declareFlag(T& value, const char* name, const char* description)
{
  registerConfig(name, description, cfgelm_type<T>::value,
    [&value](const char* name) {
      value = simgrid::config::get<T>(name);
    });
  setDefault(name, value);
}

/** Bind a variable to configuration flag
 *
 *  @param value Bound variable
 *  @param name  Flag name
 *  @param description Option description
 *  @f     Callback (called with the option name) providing the value
 */
template<class T, class F>
void declareFlag(T& value, const char* name, const char* description, F f)
{
  registerConfig(name, description, cfgelm_type<T>::value,
    [&value,f](const char* name) {
      value = f(name);
    });
  setDefault(name, value);
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
class flag {
  T value_;
public:

  /** Constructor
   *
   *  @param name  Flag name
   *  @param desc  Flag description
   *  @param value Flag initial/default value
   */
  flag(const char* name, const char* desc, T value) : value_(value)
  {
    declareFlag(value_, name, desc);
  }

  // No copy:
  flag(flag const&) = delete;
  flag& operator=(flag const&) = delete;

  // Get the underlying value:
  T& get() { return value_; }
  T const& get() const { return value_; }

  // Implicit conversion to the underlying type:
  operator T&() { return value_; }
  operator T const&() const{ return value_; }
};

}
}

#endif
