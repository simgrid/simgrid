/* Copyright (c) 2004-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MODULE_HPP
#define SIMGRID_MODULE_HPP

#include <xbt/base.h>

#include <functional>
#include <string>
#include <vector>

namespace simgrid {

struct Module {
  const char* name_;
  const char* description_;
  std::function<void()> init;
  Module(const char* id, const char* desc, std::function<void()> init_fun)
      : name_(id), description_(desc), init(init_fun)
  {
  }
};

class ModuleGroup {
  std::vector<Module> table_;
  const std::string kind_; // either 'plugin' or 'CPU model' or whatever. Used in error messages only
public:
  ModuleGroup(const std::string& kind) : kind_(kind) {}

  ModuleGroup& add(const char* id, const char* desc, std::function<void()> init);
  Module const& by_name(const std::string& name) const;
  void help() const;
  const std::string get_kind() const { return kind_; }
  std::string existing_values() const;
  void create_flag(const std::string& opt_name, const std::string& descr, const std::string& default_value,
                   bool init_now);
};

}; // namespace simgrid

#define SIMGRID_REGISTER_PLUGIN(id, desc, init)                                                                        \
  static void XBT_ATTRIB_CONSTRUCTOR(800) _XBT_CONCAT3(simgrid_, id, _plugin_register)()                               \
  {                                                                                                                    \
    simgrid_plugins().add(_XBT_STRINGIFY(id), (desc), (init));                                                         \
  }

/** @brief The list of all available plugins */
inline auto& simgrid_plugins() // Function to avoid static initialization order fiasco
{
  static simgrid::ModuleGroup plugins("plugin");
  return plugins;
}

#define SIMGRID_REGISTER_NETWORK_MODEL(id, desc, init)                                                                 \
  static void XBT_ATTRIB_CONSTRUCTOR(800) _XBT_CONCAT3(simgrid_, id, _network_model_register)()                        \
  {                                                                                                                    \
    simgrid_network_models().add(_XBT_STRINGIFY(id), (desc), (init));                                                  \
  }
/** @brief The list of all available network model (pick one with --cfg=network/model) */
inline auto& simgrid_network_models() // Function to avoid static initialization order fiasco
{
  static simgrid::ModuleGroup models("network model");
  return models;
}

#define SIMGRID_REGISTER_CPU_MODEL(id, desc, init)                                                                     \
  static void XBT_ATTRIB_CONSTRUCTOR(800) _XBT_CONCAT3(simgrid_, id, _cpu_model_register)()                            \
  {                                                                                                                    \
    simgrid_cpu_models().add(_XBT_STRINGIFY(id), (desc), (init));                                                      \
  }
/** @brief The list of all available CPU model (pick one with --cfg=cpu/model) */
inline auto& simgrid_cpu_models() // Function to avoid static initialization order fiasco
{
  static simgrid::ModuleGroup models("CPU model");
  return models;
}
#endif