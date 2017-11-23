/* Copyright (c) 2004-2017. The SimGrid Team. All rights reserved.     */

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

#include <xbt/ex.hpp>
#include <xbt/config.h>
#include <xbt/config.hpp>
#include "xbt/misc.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/dynar.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(xbt_cfg, xbt, "configuration support");

XBT_EXPORT_NO_IMPORT(xbt_cfg_t) simgrid_config = nullptr;
extern "C" {
  XBT_PUBLIC(void) sg_config_finalize();
}

namespace simgrid {
namespace config {

missing_key_error::~missing_key_error() = default;

namespace {

const char* true_values[] = {
  "yes", "on", "true", "1"
};
const char* false_values[] = {
  "no", "off", "false", "0"
};

static bool parseBool(const char* value)
{
  for (const char* const& true_value : true_values)
    if (std::strcmp(true_value, value) == 0)
      return true;
  for (const char* const& false_value : false_values)
    if (std::strcmp(false_value, value) == 0)
      return false;
  throw std::range_error("not a boolean");
}

static double parseDouble(const char* value)
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

static long int parseLong(const char* value)
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
    return parseLong(value);
  }
};
template <> class ConfigType<double> {
public:
  static constexpr const char* type_name = "double";
  static inline double parse(const char* value)
  {
    return parseDouble(value);
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
    return parseBool(value);
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

  ConfigurationElement(const char* key, const char* desc) : key(key ? key : ""), desc(desc ? desc : "") {}
  ConfigurationElement(const char* key, const char* desc, xbt_cfg_cb_t cb)
    : key(key ? key : ""), desc(desc ? desc : ""), old_callback(cb) {}

  virtual ~ConfigurationElement()=default;

  virtual std::string getStringValue() = 0;
  virtual void setStringValue(const char* value) = 0;
  virtual const char* getTypeName() = 0;

  template<class T>
  T const& getValue() const
  {
    return dynamic_cast<const TypedConfigurationElement<T>&>(*this).getValue();
  }
  template<class T>
  void setValue(T value)
  {
    dynamic_cast<TypedConfigurationElement<T>&>(*this).setValue(std::move(value));
  }
  template<class T>
  void setDefaultValue(T value)
  {
    dynamic_cast<TypedConfigurationElement<T>&>(*this).setDefaultValue(std::move(value));
  }
  void unsetDefault() { isdefault = false; }
  bool isDefault() const { return isdefault; }

  std::string const& getDescription() const { return desc; }
  std::string const& getKey() const { return key; }
};

// **** TypedConfigurationElement<T> ****

// TODO, could we use boost::any with some Type* reference?
template<class T>
class TypedConfigurationElement : public ConfigurationElement {
private:
  T content;
  std::function<void(T&)> callback;

public:
  TypedConfigurationElement(const char* key, const char* desc, T value = T())
    : ConfigurationElement(key, desc), content(std::move(value))
  {}
  TypedConfigurationElement(const char* key, const char* desc, T value, xbt_cfg_cb_t cb)
      : ConfigurationElement(key, desc, cb), content(std::move(value))
  {}
  TypedConfigurationElement(const char* key, const char* desc, T value, std::function<void(T&)> callback)
      : ConfigurationElement(key, desc), content(std::move(value)), callback(std::move(callback))
  {}
  ~TypedConfigurationElement() = default;

  std::string getStringValue() override;
  const char* getTypeName() override;
  void setStringValue(const char* value) override;

  void update()
  {
    if (old_callback)
      this->old_callback(getKey().c_str());
    if (this->callback)
      this->callback(this->content);
  }

  T const& getValue() const { return content; }

  void setValue(T value)
  {
    this->content = std::move(value);
    this->update();
  }

  void setDefaultValue(T value)
  {
    if (this->isDefault()) {
      this->content = std::move(value);
      this->update();
    } else {
      XBT_DEBUG("Do not override configuration variable '%s' with value '%s' because it was already set.",
                getKey().c_str(), to_string(value).c_str());
    }
  }
};

template<class T>
std::string TypedConfigurationElement<T>::getStringValue() // override
{
  return to_string(content);
}

template<class T>
void TypedConfigurationElement<T>::setStringValue(const char* value) // override
{
  this->content = ConfigType<T>::parse(value);
  this->unsetDefault();
  this->update();
}

template<class T>
const char* TypedConfigurationElement<T>::getTypeName() // override
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
  Config() = default;
  ~Config();

  // No copy:
  Config(Config const&) = delete;
  Config& operator=(Config const&) = delete;

  ConfigurationElement& operator[](const char* name);
  template<class T>
  TypedConfigurationElement<T>& getTyped(const char* name);
  void alias(const char* realname, const char* aliasname);

  template<class T, class... A>
  simgrid::config::TypedConfigurationElement<T>*
    registerOption(const char* name, A&&... a)
  {
    xbt_assert(options.find(name) == options.end(), "Refusing to register the config element '%s' twice.", name);
    TypedConfigurationElement<T>* variable = new TypedConfigurationElement<T>(name, std::forward<A>(a)...);
    XBT_DEBUG("Register cfg elm %s (%s) of type %s @%p in set %p)", name, variable->getDescription().c_str(),
              variable->getTypeName(), variable, this);
    options.insert({name, variable});
    variable->update();
    return variable;
  }

  // Debug:
  void dump(const char *name, const char *indent);
  void showAliases();
  void help();

protected:
  ConfigurationElement* getDictElement(const char* name);
};

Config::~Config()
{
  XBT_DEBUG("Frees cfg set %p", this);
  for (auto const& elm : options)
    delete elm.second;
}

inline ConfigurationElement* Config::getDictElement(const char* name)
{
  auto opt = options.find(name);
  if (opt != options.end()) {
    return opt->second;
  } else {
    auto als = aliases.find(name);
    if (als != aliases.end()) {
      ConfigurationElement* res = als->second;
      if (warn_for_aliases)
        XBT_INFO("Option %s has been renamed to %s. Consider switching.", name, res->getKey().c_str());
      return res;
    } else {
      throw simgrid::config::missing_key_error(std::string("Bad config key: ") + name);
    }
  }
}

inline ConfigurationElement& Config::operator[](const char* name)
{
  return *(getDictElement(name));
}

void Config::alias(const char* realname, const char* aliasname)
{
  xbt_assert(aliases.find(aliasname) == aliases.end(), "Alias '%s' already.", aliasname);
  ConfigurationElement* element = this->getDictElement(realname);
  xbt_assert(element, "Cannot define an alias to the non-existing option '%s'.", realname);
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
    printf("%s  %s: ()%s) %s", indent, elm.first.c_str(), elm.second->getTypeName(),
           elm.second->getStringValue().c_str());

  if (name)
    printf("%s<< End of the config set '%s'\n", indent, name);
  fflush(stdout);
}

/** @brief Displays the declared aliases and their description */
void Config::showAliases()
{
  bool old_warn_for_aliases = false;
  std::swap(warn_for_aliases, old_warn_for_aliases);
  for (auto const& elm : aliases)
    printf("   %s: %s\n", elm.first.c_str(), (*this)[elm.first.c_str()].getDescription().c_str());
  std::swap(warn_for_aliases, old_warn_for_aliases);
}

/** @brief Displays the declared options and their description */
void Config::help()
{
  for (auto const& elm : options) {
    simgrid::config::ConfigurationElement* variable = this->options.at(elm.first);
    printf("   %s: %s\n", elm.first.c_str(), variable->getDescription().c_str());
    printf("       Type: %s; ", variable->getTypeName());
    printf("Current value: %s\n", variable->getStringValue().c_str());
  }
}

// ***** getConfig *****

template<class T>
XBT_PUBLIC(T const&) getConfig(const char* name)
{
  return (*simgrid_config)[name].getValue<T>();
}

template XBT_PUBLIC(int const&) getConfig<int>(const char* name);
template XBT_PUBLIC(double const&) getConfig<double>(const char* name);
template XBT_PUBLIC(bool const&) getConfig<bool>(const char* name);
template XBT_PUBLIC(std::string const&) getConfig<std::string>(const char* name);

// ***** alias *****

void alias(const char* realname, const char* aliasname)
{
  simgrid_config->alias(realname, aliasname);
}

// ***** declareFlag *****

template<class T>
XBT_PUBLIC(void) declareFlag(const char* name, const char* description,
  T value, std::function<void(const T&)> callback)
{
  if (simgrid_config == nullptr) {
    simgrid_config = xbt_cfg_new();
    atexit(sg_config_finalize);
  }
  simgrid_config->registerOption<T>(
    name, description, std::move(value), std::move(callback));
}

template XBT_PUBLIC(void) declareFlag(const char* name,
  const char* description, int value, std::function<void(int const &)> callback);
template XBT_PUBLIC(void) declareFlag(const char* name,
  const char* description, double value, std::function<void(double const &)> callback);
template XBT_PUBLIC(void) declareFlag(const char* name,
  const char* description, bool value, std::function<void(bool const &)> callback);
template XBT_PUBLIC(void) declareFlag(const char* name,
  const char* description, std::string value, std::function<void(std::string const &)> callback);

}
}

// ***** C bindings *****

xbt_cfg_t xbt_cfg_new()        { return new simgrid::config::Config(); }
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
    simgrid_config = xbt_cfg_new();
  simgrid_config->registerOption<double>(name, desc, default_value, cb_set);
}

void xbt_cfg_register_int(const char *name, int default_value,xbt_cfg_cb_t cb_set, const char *desc)
{
  if (simgrid_config == nullptr) {
    simgrid_config = xbt_cfg_new();
    atexit(&sg_config_finalize);
  }
  simgrid_config->registerOption<int>(name, desc, default_value, cb_set);
}

void xbt_cfg_register_string(const char *name, const char *default_value, xbt_cfg_cb_t cb_set, const char *desc)
{
  if (simgrid_config == nullptr) {
    simgrid_config = xbt_cfg_new();
    atexit(sg_config_finalize);
  }
  simgrid_config->registerOption<std::string>(name, desc, default_value ? default_value : "", cb_set);
}

void xbt_cfg_register_boolean(const char *name, const char*default_value,xbt_cfg_cb_t cb_set, const char *desc)
{
  if (simgrid_config == nullptr) {
    simgrid_config = xbt_cfg_new();
    atexit(sg_config_finalize);
  }
  simgrid_config->registerOption<bool>(name, desc, simgrid::config::parseBool(default_value), cb_set);
}

void xbt_cfg_register_alias(const char *realname, const char *aliasname)
{
  if (simgrid_config == nullptr) {
    simgrid_config = xbt_cfg_new();
    atexit(sg_config_finalize);
  }
  simgrid_config->alias(realname, aliasname);
}

void xbt_cfg_aliases() { simgrid_config->showAliases(); }
void xbt_cfg_help()    { simgrid_config->help(); }

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
  if (not options || not strlen(options)) { /* nothing to do */
    return;
  }

  XBT_DEBUG("List to parse and set:'%s'", options);
  std::string optionlist(options);
  while (not optionlist.empty()) {
    XBT_DEBUG("Still to parse and set: '%s'", optionlist.c_str());

    // skip separators
    size_t pos = optionlist.find_first_not_of(" \t\n,");
    optionlist.erase(0, pos);
    // find option
    pos              = optionlist.find_first_of(" \t\n,");
    std::string name = optionlist.substr(0, pos);
    optionlist.erase(0, pos);
    XBT_DEBUG("parse now:'%s'; parse later:'%s'", name.c_str(), optionlist.c_str());

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

    try {
      (*simgrid_config)[name.c_str()].setStringValue(val.c_str());
    }
    catch (simgrid::config::missing_key_error& e) {
      goto on_missing_key;
    }
    catch (...) {
      goto on_exception;
    }
  }
  return;

  /* Do not THROWF from a C++ exception catching context, or some cleanups will be missing */
on_missing_key:
  THROWF(not_found_error, 0, "Could not set variables %s", options);
on_exception:
  THROWF(unknown_error, 0, "Could not set variables %s", options);
}

// Horrible mess to translate C++ exceptions to C exceptions:
// Exit from the catch block (and do the correct exception cleaning) before attempting to THROWF.
#define TRANSLATE_EXCEPTIONS(...) \
  catch(simgrid::config::missing_key_error& e) { THROWF(not_found_error, 0, __VA_ARGS__); abort(); } \
  catch(...) { THROWF(not_found_error, 0, __VA_ARGS__); abort(); }

/** @brief Set the value of a variable, using the string representation of that value
 *
 * @param key name of the variable to modify
 * @param value string representation of the value to set
 */

void xbt_cfg_set_as_string(const char *key, const char *value)
{
  try {
    (*simgrid_config)[key].setStringValue(value);
    return;
  }
  TRANSLATE_EXCEPTIONS("Could not set variable %s as string %s", key, value);
}

/** @brief Set an integer value to \a name within \a cfg if it wasn't changed yet
 *
 * This is useful to change the default value of a variable while allowing
 * users to override it with command line arguments
 */
void xbt_cfg_setdefault_int(const char *key, int value)
{
  try {
    (*simgrid_config)[key].setDefaultValue<int>(value);
    return;
  }
  TRANSLATE_EXCEPTIONS("Could not set variable %s to default integer %i", key, value);
}

/** @brief Set an integer value to \a name within \a cfg if it wasn't changed yet
 *
 * This is useful to change the default value of a variable while allowing
 * users to override it with command line arguments
 */
void xbt_cfg_setdefault_double(const char *key, double value)
{
  try {
    (*simgrid_config)[key].setDefaultValue<double>(value);
    return;
  }
  TRANSLATE_EXCEPTIONS("Could not set variable %s to default double %f", key, value);
}

/** @brief Set a string value to \a name within \a cfg if it wasn't changed yet
 *
 * This is useful to change the default value of a variable while allowing
 * users to override it with command line arguments
 */
void xbt_cfg_setdefault_string(const char *key, const char *value)
{
  try {
    (*simgrid_config)[key].setDefaultValue<std::string>(value ? value : "");
    return;
  }
  TRANSLATE_EXCEPTIONS("Could not set variable %s to default string %s", key, value);
}

/** @brief Set an boolean value to \a name within \a cfg if it wasn't changed yet
 *
 * This is useful to change the default value of a variable while allowing
 * users to override it with command line arguments
 */
void xbt_cfg_setdefault_boolean(const char *key, const char *value)
{
  try {
    (*simgrid_config)[key].setDefaultValue<bool>(simgrid::config::parseBool(value));
    return;
  }
  TRANSLATE_EXCEPTIONS("Could not set variable %s to default boolean %s", key, value);
}

/** @brief Set an integer value to \a name within \a cfg
 *
 * @param key the name of the variable
 * @param value the value of the variable
 */
void xbt_cfg_set_int(const char *key, int value)
{
  try {
    (*simgrid_config)[key].setValue<int>(value);
    return;
  }
  TRANSLATE_EXCEPTIONS("Could not set variable %s to integer %i", key, value);
}

/** @brief Set or add a double value to \a name within \a cfg
 *
 * @param key the name of the variable
 * @param value the double to set
 */
void xbt_cfg_set_double(const char *key, double value)
{
  try {
    (*simgrid_config)[key].setValue<double>(value);
    return;
  }
  TRANSLATE_EXCEPTIONS("Could not set variable %s to double %f", key, value);
}

/** @brief Set or add a string value to \a name within \a cfg
 *
 * @param key the name of the variable
 * @param value the value to be added
 *
 */
void xbt_cfg_set_string(const char* key, const char* value)
{
  try {
    (*simgrid_config)[key].setValue<std::string>(value);
    return;
  }
  TRANSLATE_EXCEPTIONS("Could not set variable %s to string %s", key, value);
}

/** @brief Set or add a boolean value to \a name within \a cfg
 *
 * @param key the name of the variable
 * @param value the value of the variable
 */
void xbt_cfg_set_boolean(const char *key, const char *value)
{
  try {
    (*simgrid_config)[key].setValue<bool>(simgrid::config::parseBool(value));
    return;
  }
  TRANSLATE_EXCEPTIONS("Could not set variable %s to boolean %s", key, value);
}


/* Say if the value is the default value */
int xbt_cfg_is_default_value(const char *key)
{
  try {
    return (*simgrid_config)[key].isDefault() ? 1 : 0;
  }
  TRANSLATE_EXCEPTIONS("Could not get variable %s", key);
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
  try {
    return (*simgrid_config)[key].getValue<int>();
  }
  TRANSLATE_EXCEPTIONS("Could not get variable %s", key);
}

/** @brief Retrieve a double value of a variable (get a warning if not uniq)
 *
 * @param key the name of the variable
 *
 * Returns the first value from the config set under the given name.
 */
double xbt_cfg_get_double(const char *key)
{
  try {
    return (*simgrid_config)[key].getValue<double>();
  }
  TRANSLATE_EXCEPTIONS("Could not get variable %s", key);
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
  try {
    return (*simgrid_config)[key].getValue<std::string>();
  }
  TRANSLATE_EXCEPTIONS("Could not get variable %s", key);
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
  try {
    return (*simgrid_config)[key].getValue<bool>() ? 1 : 0;
  }
  TRANSLATE_EXCEPTIONS("Could not get variable %s", key);
}

#ifdef SIMGRID_TEST

#include <string>

#include "xbt.h"
#include "xbt/ex.h"
#include <xbt/ex.hpp>

#include <xbt/config.hpp>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(xbt_cfg);

XBT_TEST_SUITE("config", "Configuration support");

XBT_PUBLIC_DATA(xbt_cfg_t) simgrid_config;

static void make_set()
{
  simgrid_config = nullptr;
  xbt_log_threshold_set(&_XBT_LOGV(xbt_cfg), xbt_log_priority_critical);
  xbt_cfg_register_int("speed", 0, nullptr, "");
  xbt_cfg_register_string("peername", "", nullptr, "");
  xbt_cfg_register_string("user", "", nullptr, "");
}                               /* end_of_make_set */

XBT_TEST_UNIT("memuse", test_config_memuse, "Alloc and free a config set")
{
  auto temp = simgrid_config;
  make_set();
  xbt_test_add("Alloc and free a config set");
  xbt_cfg_set_parse("peername:veloce user:bidule");
  xbt_cfg_free(&simgrid_config);
  simgrid_config = temp;
}

XBT_TEST_UNIT("use", test_config_use, "Data retrieving tests")
{
  auto temp = simgrid_config;
  make_set();
  xbt_test_add("Get a single value");
  {
    /* get_single_value */
    xbt_cfg_set_parse("peername:toto:42 speed:42");
    int ival = xbt_cfg_get_int("speed");
    if (ival != 42)
      xbt_test_fail("Speed value = %d, I expected 42", ival);
  }

  xbt_test_add("Access to a non-existant entry");
  {
    try {
      xbt_cfg_set_parse("color:blue");
    } catch(xbt_ex& e) {
      if (e.category != not_found_error)
        xbt_test_exception(e);
    }
  }
  xbt_cfg_free(&simgrid_config);
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
  xbt_cfg_set_parse("int:42 string:bar double:8.0 bool1:true bool2:false");
  xbt_test_assert(int_flag == 42, "Check int flag");
  xbt_test_assert(string_flag == "bar", "Check string flag");
  xbt_test_assert(double_flag == 8.0, "Check double flag");
  xbt_test_assert(bool_flag1, "Check bool1 flag");
  xbt_test_assert(not bool_flag2, "Check bool2 flag");

  xbt_cfg_free(&simgrid_config);
  simgrid_config = temp;
}

#endif                          /* SIMGRID_TEST */
