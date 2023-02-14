/* Copyright (c) 2004-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/asserts.h>
#include <xbt/log.h>

#include "src/simgrid/module.hpp"
#include "src/surf/surf_interface.hpp"

#include <sstream>

XBT_LOG_NEW_CATEGORY(plugin, "Common category for the logging of all plugins");
XBT_LOG_EXTERNAL_CATEGORY(xbt_help);

using namespace simgrid;

ModuleGroup& ModuleGroup::add(const char* id, const char* desc, std::function<void()> init)
{
  table_.emplace_back(Module(id, desc, init));
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

/* -------------------------------------------------------------------------------------------------------------- */
simgrid::ModuleGroup surf_optimization_mode_description("optimization mode");
simgrid::ModuleGroup surf_cpu_model_description("CPU model");
simgrid::ModuleGroup surf_network_model_description("network model");
simgrid::ModuleGroup surf_disk_model_description("disk model");
simgrid::ModuleGroup surf_host_model_description("host model");

void simgrid_create_models()
{
  surf_network_model_description
      .add("LV08",
           "Realistic network analytic model (slow-start modeled by multiplying latency by 13.01, bandwidth by .97; "
           "bottleneck sharing uses a payload of S=20537 for evaluating RTT). ",
           &surf_network_model_init_LegrandVelho)
      .add("Constant",
           "Simplistic network model where all communication take a constant time (one second). This model "
           "provides the lowest realism, but is (marginally) faster.",
           &surf_network_model_init_Constant)
      .add("SMPI",
           "Realistic network model specifically tailored for HPC settings (accurate modeling of slow start with "
           "correction factors on three intervals: < 1KiB, < 64 KiB, >= 64 KiB)",
           &surf_network_model_init_SMPI)
      .add("IB",
#if HAVE_SMPI
           "Realistic network model specifically tailored for HPC settings, with Infiniband contention model",
#else
           "(Infiniband model is only enabled when SMPI is)",
#endif
           &surf_network_model_init_IB)
      .add("CM02",
           "Legacy network analytic model (Very similar to LV08, but without corrective factors. The timings of "
           "small messages are thus poorly modeled).",
           &surf_network_model_init_CM02)
      .add("ns-3",
#if SIMGRID_HAVE_NS3
           "Network pseudo-model using the ns-3 tcp model instead of an analytic model",
#else
           "(ns-3 pseudo-model is only activated when ns-3 support was compiled it)",
#endif
           &surf_network_model_init_NS3);

  surf_cpu_model_description.add("Cas01", "Simplistic CPU model (time=size/speed).", &surf_cpu_model_init_Cas01);
  surf_disk_model_description.add("S19", "Simplistic disk model.", &surf_disk_model_init_S19);

  surf_host_model_description
      .add("default",
           "Default host model. Currently, CPU:Cas01, network:LV08 (with cross traffic enabled), and disk:S19",
           &surf_host_model_init_current_default)
      .add("compound", "Host model that is automatically chosen if you change the CPU, network, and disk models",
           &surf_host_model_init_compound)
      .add("ptask_L07", "Host model somehow similar to Cas01+CM02+S19 but allowing parallel tasks",
           &surf_host_model_init_ptask_L07);

  surf_optimization_mode_description
      .add("Lazy", "Lazy action management (partial invalidation in lmm + heap in action remaining).", nullptr)
      .add("TI",
           "Trace integration. Highly optimized mode when using availability traces (only available for the Cas01 CPU "
           "model for now).",
           nullptr)
      .add("Full", "Full update of remaining and variables. Slow but may be useful when debugging.", nullptr);
}

#if !HAVE_SMPI
void surf_network_model_init_IB()
{
  xbt_die("Please activate SMPI support in cmake to use the IB network model.");
}
#endif
#if !SIMGRID_HAVE_NS3
void surf_network_model_init_NS3()
{
  xbt_die("Please activate ns-3 support in cmake and install the dependencies to use the NS3 network model.");
}
#endif
