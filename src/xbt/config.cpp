/* Copyright (c) 2004-2018. The SimGrid Team. All rights reserved.     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstdio>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <climits>

#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <vector>

#include "simgrid/Exception.hpp"
#include "simgrid/sg_config.hpp"
#include "xbt/dynar.h"
#include "xbt/log.h"
#include "xbt/misc.h"
#include "xbt/sysdep.h"
#include <xbt/config.h>
#include <xbt/config.hpp>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_cfg, xbt, "configuration support");

XBT_EXPORT_NO_IMPORT xbt_cfg_t simgrid_config = nullptr;

namespace simgrid {
namespace config {

namespace {

const char* true_values[] = {
  "yes", "on", "true", "1"
};
const char* false_values[] = {
  "no", "off", "false", "0"
};

static bool parse_bool(const char* value)
{
  for (const char* const& true_value : true_values)
    if (std::strcmp(true_value, value) == 0)
      return true;
  for (const char* const& false_value : false_values)
    if (std::strcmp(false_value, value) == 0)
      return false;
  throw std::range_error("not a boolean");
}

static double parse_double(const char* value)
{
  char* end;
  errno = 0;
  double res = std::strtod(value, &end);
  if (errno == ERANGE)
    throw std::range_error("out of range");
  else if (errno)
    xbt_die("Unexpected errno");
  if (end == value || *end != '\0')
    throw std::range_error("invalid double");
  else
    return res;
}

static long int parse_long(const char* value)
{
  char* end;
  errno = 0;
  long int res = std::strtol(value, &end, 0);
  if (errno) {
    if (res == LONG_MIN && errno == ERANGE)
      throw std::range_error("underflow");
    else if (res == LONG_MAX && errno == ERANGE)
      throw std::range_error("overflow");
    xbt_die("Unexpected errno");
  }
  if (end == value || *end != '\0')
    throw std::range_error("invalid integer");
  else
    return res;
}

// ***** ConfigType *****

/// A trait which define possible options types:
template <class T> class ConfigType;

template <> class ConfigType<int> {
public:
  static constexpr const char* type_name = "int";
  static inline double parse(const char* value)
  {
    return parse_long(value);
  }
};
template <> class ConfigType<double> {
public:
  static constexpr const char* type_name = "double";
  static inline double parse(const char* value)
  {
    return parse_double(value);
  }
};
template <> class ConfigType<std::string> {
public:
  static constexpr const char* type_name = "string";
  static inline std::string parse(const char* value)
  {
    return std::string(value);
  }
};
template <> class ConfigType<bool> {
public:
  static constexpr const char* type_name = "boolean";
  static inline bool parse(const char* value)
  {
    return parse_bool(value);
  }
};

// **** Forward declarations ****

class ConfigurationElement ;
template<class T> class TypedConfigurationElement;

// **** ConfigurationElement ****

class ConfigurationElement {
private:
  std::string key;
  std::string desc;
  bool isdefault = true;

public:
  /* Callback */
  xbt_cfg_cb_t old_callback = nullptr;

  ConfigurationElement(std::string key, std::string desc) : key(key), desc(desc) {}
  ConfigurationElement(std::string key, std::string desc, xbt_cfg_cb_t cb) : key(key), desc(desc), old_callback(cb) {}

  virtual ~ConfigurationElement() = default;

  virtual std::string get_string_value()           = 0;
  virtual void set_string_value(const char* value) = 0;
  virtual const char* get_type_name()              = 0;

  template <class T> T const& get_value() const
  {
    return dynamic_cast<const TypedConfigurationElement<T>&>(*this).get_value();
  }
  template <class T> void set_value(T value)
  {
    dynamic_cast<TypedConfigurationElement<T>&>(*this).set_value(std::move(value));
  }
  template <class T> void set_default_value(T value)
  {
    dynamic_cast<TypedConfigurationElement<T>&>(*this).set_default_value(std::move(value));
  }
  void unset_default() { isdefault = false; }
  bool is_default() const { return isdefault; }

  std::string const& get_description() const { return desc; }
  std::string const& get_key() const { return key; }
};

// **** TypedConfigurationElement<T> ****

// TODO, could we use boost::any with some Type* reference?
template<class T>
class TypedConfigurationElement : public ConfigurationElement {
private:
  T content;
  std::function<void(T&)> callback;

public:
  TypedConfigurationElement(std::string key, std::string desc, T value = T())
      : ConfigurationElement(key, desc), content(std::move(value))
  {}
  TypedConfigurationElement(std::string key, std::string desc, T value, xbt_cfg_cb_t cb)
      : ConfigurationElement(key, desc, cb), content(std::move(value))
  {}
  TypedConfigurationElement(std::string key, std::string desc, T value, std::function<void(T&)> callback)
      : ConfigurationElement(key, desc), content(std::move(value)), callback(std::move(callback))
  {}
  ~TypedConfigurationElement() = default;

  std::string get_string_value() override;
  const char* get_type_name() override;
  void set_string_value(const char* value) override;

  void update()
  {
    if (old_callback)
      this->old_callback(get_key().c_str());
    if (this->callback)
      this->callback(this->content);
  }

  T const& get_value() const { return content; }

  void set_value(T value)
  {
    this->content = std::move(value);
    this->update();
    this->unset_default();
  }

  void set_default_value(T value)
  {
    if (this->is_default()) {
      this->content = std::move(value);
      this->update();
    } else {
      XBT_DEBUG("Do not override configuration variable '%s' with value '%s' because it was already set.",
                get_key().c_str(), to_string(value).c_str());
    }
  }
};

template <class T> std::string TypedConfigurationElement<T>::get_string_value() // override
{
  return to_string(content);
}

template <class T> void TypedConfigurationElement<T>::set_string_value(const char* value) // override
{
  this->content = ConfigType<T>::parse(value);
  this->unset_default();
  this->update();
}

template <class T> const char* TypedConfigurationElement<T>::get_type_name() // override
{
  return ConfigType<T>::type_name;
}

} // end of anonymous namespace

// **** Config ****

class Config {
private:
  // name -> ConfigElement:
  std::map<std::string, simgrid::config::ConfigurationElement*> options;
  // alias -> ConfigElement from options:
  std::map<std::string, simgrid::config::ConfigurationElement*> aliases;
  bool warn_for_aliases = true;

public:
  Config();
  ~Config();

  // No copy:
  Config(Config const&) = delete;
  Config& operator=(Config const&) = delete;

  ConfigurationElement& operator[](std::string name);
  void alias(std::string realname, std::string aliasname);

  template <class T, class... A>
  simgrid::config::TypedConfigurationElement<T>* register_option(std::string name, A&&... a)
  {
    xbt_assert(options.find(name) == options.end(), "Refusing to register the config element '%s' twice.",
               name.c_str());
    TypedConfigurationElement<T>* variable = new TypedConfigurationElement<T>(name, std::forward<A>(a)...);
    XBT_DEBUG("Register cfg elm %s (%s) of type %s @%p in set %p)", name.c_str(), variable->get_description().c_str(),
              variable->get_type_name(), variable, this);
    options.insert({name, variable});
    variable->update();
    return variable;
  }

  // Debug:
  void dump(const char *name, const char *indent);
  void show_aliases();
  void help();

protected:
  ConfigurationElement* get_dict_element(std::string name);
};

Config::Config()
{
  atexit(&sg_config_finalize);
}
Config::~Config()
{
  XBT_DEBUG("Frees cfg set %p", this);
  for (auto const& elm : options)
    delete elm.second;
}

inline ConfigurationElement* Config::get_dict_element(std::string name)
{
  auto opt = options.find(name);
  if (opt != options.end()) {
    return opt->second;
  } else {
    auto als = aliases.find(name);
    if (als != aliases.end()) {
      ConfigurationElement* res = als->second;
      if (warn_for_aliases)
        XBT_INFO("Option %s has been renamed to %s. Consider switching.", name.c_str(), res->get_key().c_str());
      return res;
    } else {
      THROWF(not_found_error, 0, "Bad config key: %s", name.c_str());
    }
  }
}

inline ConfigurationElement& Config::operator[](std::string name)
{
  return *(get_dict_element(name));
}

void Config::alias(std::string realname, std::string aliasname)
{
  xbt_assert(aliases.find(aliasname) == aliases.end(), "Alias '%s' already.", aliasname.c_str());
  ConfigurationElement* element = this->get_dict_element(realname);
  xbt_assert(element, "Cannot define an alias to the non-existing option '%s'.", realname.c_str());
  this->aliases.insert({aliasname, element});
}

/** @brief Dump a config set for debuging purpose
 *
 * @param name The name to give to this config set
 * @param indent what to write at the beginning of each line (right number of spaces)
 */
void Config::dump(const char *name, const char *indent)
{
  if (name)
    printf("%s>> Dumping of the config set '%s':\n", indent, name);

  for (auto const& elm : options)
    printf("%s  %s: ()%s) %s", indent, elm.first.c_str(), elm.second->get_type_name(),
           elm.second->get_string_value().c_str());

  if (name)
    printf("%s<< End of the config set '%s'\n", indent, name);
  fflush(stdout);
}

/** @brief Displays the declared aliases and their replacement */
void Config::show_aliases()
{
  for (auto const& elm : aliases)
    printf("   %-40s %s\n", elm.first.c_str(), elm.second->get_key().c_str());
}

/** @brief Displays the declared options and their description */
void Config::help()
{
  for (auto const& elm : options) {
    simgrid::config::ConfigurationElement* variable = this->options.at(elm.first);
    printf("   %s: %s\n", elm.first.c_str(), variable->get_description().c_str());
    printf("       Type: %s; ", variable->get_type_name());
    printf("Current value: %s\n", variable->get_string_value().c_str());
  }
}

// ***** set_default *****

template <class T> XBT_PUBLIC void set_default(const char* name, T value)
{
  (*simgrid_config)[name].set_default_value<T>(std::move(value));
}

template XBT_PUBLIC void set_default<int>(const char* name, int value);
template XBT_PUBLIC void set_default<double>(const char* name, double value);
template XBT_PUBLIC void set_default<bool>(const char* name, bool value);
template XBT_PUBLIC void set_default<std::string>(const char* name, std::string value);

bool is_default(const char* name)
{
  return (*simgrid_config)[name].is_default();
}

// ***** set_value *****

template <class T> XBT_PUBLIC void set_value(const char* name, T value)
{
  (*simgrid_config)[name].set_value<T>(std::move(value));
}

template XBT_PUBLIC void set_value<int>(const char* name, int value);
template XBT_PUBLIC void set_value<double>(const char* name, double value);
template XBT_PUBLIC void set_value<bool>(const char* name, bool value);
template XBT_PUBLIC void set_value<std::string>(const char* name, std::string value);

void set_as_string(const char* name, const std::string& value)
{
  (*simgrid_config)[name].set_string_value(value.c_str());
}

void set_parse(std::string options)
{
  XBT_DEBUG("List to parse and set:'%s'", options.c_str());
  while (not options.empty()) {
    XBT_DEBUG("Still to parse and set: '%s'", options.c_str());

    // skip separators
    size_t pos = options.find_first_not_of(" \t\n,");
    options.erase(0, pos);
    // find option
    pos              = options.find_first_of(" \t\n,");
    std::string name = options.substr(0, pos);
    options.erase(0, pos);
    XBT_DEBUG("parse now:'%s'; parse later:'%s'", name.c_str(), options.c_str());

    if (name.empty())
      continue;

    pos = name.find(':');
    xbt_assert(pos != std::string::npos, "Option '%s' badly formatted. Should be of the form 'name:value'",
               name.c_str());

    std::string val = name.substr(pos + 1);
    name.erase(pos);

    const std::string path("path");
    if (name.compare(0, path.length(), path) != 0)
      XBT_INFO("Configuration change: Set '%s' to '%s'", name.c_str(), val.c_str());

    set_as_string(name.c_str(), val);
  }
}

// ***** get_value *****

template <class T> XBT_PUBLIC T const& get_value(std::string name)
{
  return (*simgrid_config)[name].get_value<T>();
}

template XBT_PUBLIC int const& get_value<int>(std::string name);
template XBT_PUBLIC double const& get_value<double>(std::string name);
template XBT_PUBLIC bool const& get_value<bool>(std::string name);
template XBT_PUBLIC std::string const& get_value<std::string>(std::string name);

// ***** alias *****

void alias(const char* realname, std::initializer_list<const char*> aliases)
{
  for (auto const& aliasname : aliases)
    simgrid_config->alias(realname, aliasname);
}

// ***** declare_flag *****

template <class T>
XBT_PUBLIC void declare_flag(std::string name, std::string description, T value, std::function<void(const T&)> callback)
{
  if (simgrid_config == nullptr)
    simgrid_config = new simgrid::config::Config();
  simgrid_config->register_option<T>(name, description, std::move(value), std::move(callback));
}

template XBT_PUBLIC void declare_flag(std::string name, std::string description, int value,
                                      std::function<void(int const&)> callback);
template XBT_PUBLIC void declare_flag(std::string name, std::string description, double value,
                                      std::function<void(double const&)> callback);
template XBT_PUBLIC void declare_flag(std::string name, std::string description, bool value,
                                      std::function<void(bool const&)> callback);
template XBT_PUBLIC void declare_flag(std::string name, std::string description, std::string value,
                                      std::function<void(std::string const&)> callback);

void finalize()
{
  delete simgrid_config;
  simgrid_config = nullptr;
}

void show_aliases()
{
  simgrid_config->show_aliases();
}

void help()
{
  simgrid_config->help();
}
}
}

// ***** C bindings *****

xbt_cfg_t xbt_cfg_new()
{
  return new simgrid::config::Config();
}
void xbt_cfg_free(xbt_cfg_t * cfg) { delete *cfg; }

void xbt_cfg_dump(const char *name, const char *indent, xbt_cfg_t cfg)
{
  cfg->dump(name, indent);
}

/*----[ Registering stuff ]-----------------------------------------------*/

void xbt_cfg_register_double(const char *name, double default_value,
  xbt_cfg_cb_t cb_set, const char *desc)
{
  if (simgrid_config == nullptr)
    simgrid_config = new simgrid::config::Config();
  simgrid_config->register_option<double>(name, desc, default_value, cb_set);
}

void xbt_cfg_register_int(const char *name, int default_value,xbt_cfg_cb_t cb_set, const char *desc)
{
  if (simgrid_config == nullptr)
    simgrid_config = new simgrid::config::Config();
  simgrid_config->register_option<int>(name, desc, default_value, cb_set);
}

void xbt_cfg_register_string(const char *name, const char *default_value, xbt_cfg_cb_t cb_set, const char *desc)
{
  if (simgrid_config == nullptr)
    simgrid_config = new simgrid::config::Config();
  simgrid_config->register_option<std::string>(name, desc, default_value ? default_value : "", cb_set);
}

void xbt_cfg_register_boolean(const char *name, const char*default_value,xbt_cfg_cb_t cb_set, const char *desc)
{
  if (simgrid_config == nullptr)
    simgrid_config = new simgrid::config::Config();
  simgrid_config->register_option<bool>(name, desc, simgrid::config::parse_bool(default_value), cb_set);
}

void xbt_cfg_register_alias(const char *realname, const char *aliasname)
{
  if (simgrid_config == nullptr)
    simgrid_config = new simgrid::config::Config();
  simgrid_config->alias(realname, aliasname);
}

void xbt_cfg_aliases()
{
  simgrid_config->show_aliases();
}
void xbt_cfg_help()
{
  simgrid_config->help();
}

/*----[ Setting ]---------------------------------------------------------*/

/** @brief Add values parsed from a string into a config set
 *
 * @param options a string containing the content to add to the config set. This is a '\\t',' ' or '\\n' or ','
 * separated list of variables. Each individual variable is like "[name]:[value]" where [name] is the name of an
 * already registered variable, and [value] conforms to the data type under which this variable was registered.
 *
 * @todo This is a crude manual parser, it should be a proper lexer.
 */
void xbt_cfg_set_parse(const char *options)
{
  if (options && strlen(options) > 0)
    simgrid::config::set_parse(std::string(options));
}

/** @brief Set the value of a variable, using the string representation of that value
 *
 * @param key name of the variable to modify
 * @param value string representation of the value to set
 */

void xbt_cfg_set_as_string(const char *key, const char *value)
{
  (*simgrid_config)[key].set_string_value(value);
}

/** @brief Set an integer value to \a name within \a cfg if it wasn't changed yet
 *
 * This is useful to change the default value of a variable while allowing
 * users to override it with command line arguments
 */
void xbt_cfg_setdefault_int(const char *key, int value)
{
  (*simgrid_config)[key].set_default_value<int>(value);
}

/** @brief Set an integer value to \a name within \a cfg if it wasn't changed yet
 *
 * This is useful to change the default value of a variable while allowing
 * users to override it with command line arguments
 */
void xbt_cfg_setdefault_double(const char *key, double value)
{
  (*simgrid_config)[key].set_default_value<double>(value);
}

/** @brief Set a string value to \a name within \a cfg if it wasn't changed yet
 *
 * This is useful to change the default value of a variable while allowing
 * users to override it with command line arguments
 */
void xbt_cfg_setdefault_string(const char *key, const char *value)
{
  (*simgrid_config)[key].set_default_value<std::string>(value ? value : "");
}

/** @brief Set an boolean value to \a name within \a cfg if it wasn't changed yet
 *
 * This is useful to change the default value of a variable while allowing
 * users to override it with command line arguments
 */
void xbt_cfg_setdefault_boolean(const char *key, const char *value)
{
  (*simgrid_config)[key].set_default_value<bool>(simgrid::config::parse_bool(value));
}

/** @brief Set an integer value to \a name within \a cfg
 *
 * @param key the name of the variable
 * @param value the value of the variable
 */
void xbt_cfg_set_int(const char *key, int value)
{
  (*simgrid_config)[key].set_value<int>(value);
}

/** @brief Set or add a double value to \a name within \a cfg
 *
 * @param key the name of the variable
 * @param value the double to set
 */
void xbt_cfg_set_double(const char *key, double value)
{
  (*simgrid_config)[key].set_value<double>(value);
}

/** @brief Set or add a string value to \a name within \a cfg
 *
 * @param key the name of the variable
 * @param value the value to be added
 *
 */
void xbt_cfg_set_string(const char* key, const char* value)
{
  (*simgrid_config)[key].set_value<std::string>(value);
}

/** @brief Set or add a boolean value to \a name within \a cfg
 *
 * @param key the name of the variable
 * @param value the value of the variable
 */
void xbt_cfg_set_boolean(const char *key, const char *value)
{
  (*simgrid_config)[key].set_value<bool>(simgrid::config::parse_bool(value));
}


/* Say if the value is the default value */
int xbt_cfg_is_default_value(const char *key)
{
  return (*simgrid_config)[key].is_default() ? 1 : 0;
}

/*----[ Getting ]---------------------------------------------------------*/
/** @brief Retrieve an integer value of a variable (get a warning if not uniq)
 *
 * @param key the name of the variable
 *
 * Returns the first value from the config set under the given name.
 */
int xbt_cfg_get_int(const char *key)
{
  return (*simgrid_config)[key].get_value<int>();
}

/** @brief Retrieve a double value of a variable (get a warning if not uniq)
 *
 * @param key the name of the variable
 *
 * Returns the first value from the config set under the given name.
 */
double xbt_cfg_get_double(const char *key)
{
  return (*simgrid_config)[key].get_value<double>();
}

/** @brief Retrieve a string value of a variable (get a warning if not uniq)
 *
 * @param key the name of the variable
 *
 * Returns the first value from the config set under the given name.
 * If there is more than one value, it will issue a warning.
 * Returns nullptr if there is no value.
 *
 * \warning the returned value is the actual content of the config set
 */
std::string xbt_cfg_get_string(const char* key)
{
  return (*simgrid_config)[key].get_value<std::string>();
}

/** @brief Retrieve a boolean value of a variable (get a warning if not uniq)
 *
 * @param key the name of the variable
 *
 * Returns the first value from the config set under the given name.
 * If there is more than one value, it will issue a warning.
 */
int xbt_cfg_get_boolean(const char *key)
{
  return (*simgrid_config)[key].get_value<bool>() ? 1 : 0;
}

#ifdef SIMGRID_TEST

#include <string>

#include "simgrid/Exception.hpp"
#include "xbt.h"
#include "xbt/ex.h"

#include <xbt/config.hpp>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(xbt_cfg);

XBT_TEST_SUITE("config", "Configuration support");

XBT_PUBLIC_DATA simgrid::config::Config* simgrid_config;

static void make_set()
{
  simgrid_config = nullptr;
  xbt_log_threshold_set(&_XBT_LOGV(xbt_cfg), xbt_log_priority_critical);
  simgrid::config::declare_flag<int>("speed", "description", 0);
  simgrid::config::declare_flag<std::string>("peername", "description", "");
  simgrid::config::declare_flag<std::string>("user", "description", "");
}                               /* end_of_make_set */

XBT_TEST_UNIT("memuse", test_config_memuse, "Alloc and free a config set")
{
  auto temp = simgrid_config;
  make_set();
  xbt_test_add("Alloc and free a config set");
  simgrid::config::set_parse("peername:veloce user:bidule");
  simgrid::config::finalize();
  simgrid_config = temp;
}

XBT_TEST_UNIT("use", test_config_use, "Data retrieving tests")
{
  auto temp = simgrid_config;
  make_set();
  xbt_test_add("Get a single value");
  {
    /* get_single_value */
    simgrid::config::set_parse("peername:toto:42 speed:42");
    int ival = simgrid::config::get_value<int>("speed");
    if (ival != 42)
      xbt_test_fail("Speed value = %d, I expected 42", ival);
  }

  xbt_test_add("Access to a non-existant entry");
  {
    try {
      simgrid::config::set_parse("color:blue");
    } catch(xbt_ex& e) {
      if (e.category != not_found_error)
        xbt_test_exception(e);
    }
  }
  simgrid::config::finalize();
  simgrid_config = temp;
}

XBT_TEST_UNIT("c++flags", test_config_cxx_flags, "C++ flags")
{
  auto temp = simgrid_config;
  make_set();
  xbt_test_add("C++ declaration of flags");

  simgrid::config::Flag<int> int_flag("int", "", 0);
  simgrid::config::Flag<std::string> string_flag("string", "", "foo");
  simgrid::config::Flag<double> double_flag("double", "", 0.32);
  simgrid::config::Flag<bool> bool_flag1("bool1", "", false);
  simgrid::config::Flag<bool> bool_flag2("bool2", "", true);

  xbt_test_add("Parse values");
  simgrid::config::set_parse("int:42 string:bar double:8.0 bool1:true bool2:false");
  xbt_test_assert(int_flag == 42, "Check int flag");
  xbt_test_assert(string_flag == "bar", "Check string flag");
  xbt_test_assert(double_flag == 8.0, "Check double flag");
  xbt_test_assert(bool_flag1, "Check bool1 flag");
  xbt_test_assert(not bool_flag2, "Check bool2 flag");

  simgrid::config::finalize();
  simgrid_config = temp;
}

#endif                          /* SIMGRID_TEST */
