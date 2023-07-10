/* Copyright (c) 2004-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/asserts.h>
#include <xbt/log.h>

#include "src/simgrid/module.hpp"
#include "src/simgrid/sg_config.hpp"

#include <algorithm>
#include <sstream>

XBT_LOG_NEW_CATEGORY(plugin, "Common category for the logging of all plugins");
XBT_LOG_EXTERNAL_CATEGORY(xbt_help);

using namespace simgrid;

void ModuleGroup::create_flag(const std::string& opt_name, const std::string& descr, const std::string& default_value,
                              bool init_now)
{
  opt_name_               = opt_name;
  std::string description = descr + ". Possible values (other compilation flags may activate more " + get_kind() +
                            "s): " + existing_values() +
                            ".\n       (use 'help' as a value to see the long description of each one)";

  simgrid::config::declare_flag<std::string>(
      opt_name, description, default_value, [this, default_value, init_now](const std::string& value) {
        xbt_assert(_sg_cfg_init_status < 2, "Cannot load a %s after the initialization", kind_.c_str());

        if (value == default_value)
          return;

        if (value == "help") {
          help();
          exit(0);
        }

        if (init_now)
          by_name(value).init();
        else
          by_name(value); // Simply ensure that this value exists, it will be picked up later
      });
}
void ModuleGroup::init_from_flag_value() const
{
  by_name(simgrid::config::get_value<std::string>(opt_name_)).init();
}

ModuleGroup& ModuleGroup::add(const char* id, const char* desc, std::function<void()> init)
{
  table_.emplace_back(id, desc, std::move(init));
  return *this;
}

Module const& ModuleGroup::by_name(const std::string& name) const
{
  if (auto pos = std::find_if(table_.begin(), table_.end(), [&name](const Module& item) { return item.name_ == name; });
      pos != table_.end())
    return *pos;

  xbt_die("Unable to find %s '%s'. Valid values are: %s.", kind_.c_str(), name.c_str(), existing_values().c_str());
}
/** Displays the long description of all registered models, and quit */
void ModuleGroup::help() const
{
  XBT_HELP("Long description of the %s accepted by this simulator:", kind_.c_str());
  for (auto const& item : table_)
    XBT_HELP("  %s: %s", item.name_, item.description_);
}
std::string ModuleGroup::existing_values() const
{
  std::stringstream ss;
  std::string sep;
  for (auto const& item : table_) {
    ss << sep + item.name_;
    sep = ", ";
  }
  return ss.str();
}
