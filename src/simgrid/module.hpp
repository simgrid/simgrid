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
  std::string kind_; // either 'plugin' or 'CPU model' or whatever. Used in error messages only
public:
  ModuleGroup(std::string kind) : kind_(kind) {}

  ModuleGroup& add(const char* id, const char* desc, std::function<void()> init);
  Module const& by_name(const std::string& name) const;
  void help() const;
  std::string existing_values() const;
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

#endif