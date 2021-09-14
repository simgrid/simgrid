/* Copyright (c) 2004-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_interface.hpp"
#include "mc/mc.h"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/sg_config.hpp"
#include "src/kernel/resource/profile/FutureEvtSet.hpp"
#include "src/kernel/resource/profile/Profile.hpp"
#include "src/surf/HostImpl.hpp"
#include "src/surf/xml/platf.hpp"
#include "surf/surf.hpp"
#include "xbt/module.h"
#include "xbt/xbt_modinter.h" /* whether initialization was already done */

#include <fstream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

XBT_LOG_NEW_CATEGORY(surf, "All SURF categories");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_kernel, surf, "Logging specific to SURF (kernel)");

/*********
 * Utils *
 *********/

simgrid::kernel::profile::FutureEvtSet future_evt_set;
std::vector<std::string> surf_path;

/* Don't forget to update the option description in smx_config when you change this */
const std::vector<surf_model_description_t> surf_network_model_description = {
    {"LV08",
     "Realistic network analytic model (slow-start modeled by multiplying latency by 13.01, bandwidth by .97; "
     "bottleneck sharing uses a payload of S=20537 for evaluating RTT). ",
     &surf_network_model_init_LegrandVelho},
    {"Constant",
     "Simplistic network model where all communication take a constant time (one second). This model "
     "provides the lowest realism, but is (marginally) faster.",
     &surf_network_model_init_Constant},
    {"SMPI",
     "Realistic network model specifically tailored for HPC settings (accurate modeling of slow start with "
     "correction factors on three intervals: < 1KiB, < 64 KiB, >= 64 KiB)",
     &surf_network_model_init_SMPI},
    {"IB", "Realistic network model specifically tailored for HPC settings, with Infiniband contention model",
     &surf_network_model_init_IB},
    {"CM02",
     "Legacy network analytic model (Very similar to LV08, but without corrective factors. The timings of "
     "small messages are thus poorly modeled).",
     &surf_network_model_init_CM02},
    {"ns-3", "Network pseudo-model using the ns-3 tcp model instead of an analytic model",
     &surf_network_model_init_NS3},
};

#if !HAVE_SMPI
void surf_network_model_init_SMPI()
{
  xbt_die("Please activate SMPI support in cmake to use the SMPI network model.");
}
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

const std::vector<surf_model_description_t> surf_cpu_model_description = {
    {"Cas01", "Simplistic CPU model (time=size/speed).", &surf_cpu_model_init_Cas01},
};

const std::vector<surf_model_description_t> surf_host_model_description = {
    {"default", "Default host model. Currently, CPU:Cas01 and network:LV08 (with cross traffic enabled)",
     &surf_host_model_init_current_default},
    {"compound", "Host model that is automatically chosen if you change the network and CPU models",
     &surf_host_model_init_compound},
    {"ptask_L07", "Host model somehow similar to Cas01+CM02 but allowing parallel tasks",
     &surf_host_model_init_ptask_L07},
};

const std::vector<surf_model_description_t> surf_optimization_mode_description = {
    {"Lazy", "Lazy action management (partial invalidation in lmm + heap in action remaining).", nullptr},
    {"TI",
     "Trace integration. Highly optimized mode when using availability traces (only available for the Cas01 CPU "
     "model for now).",
     nullptr},
    {"Full", "Full update of remaining and variables. Slow but may be useful when debugging.", nullptr},
};

const std::vector<surf_model_description_t> surf_disk_model_description = {
    {"default", "Simplistic disk model.", &surf_disk_model_init_default},
};

double NOW = 0;

double surf_get_clock()
{
  return NOW;
}

/* returns whether #file_path is an absolute file path. Surprising, isn't it ? */
static bool is_absolute_file_path(const std::string& file_path)
{
#ifdef _WIN32
  WIN32_FIND_DATA wfd = {0};
  HANDLE hFile        = FindFirstFile(file_path.c_str(), &wfd);

  if (INVALID_HANDLE_VALUE == hFile)
    return false;

  FindClose(hFile);
  return true;
#else
  return (file_path.c_str()[0] == '/');
#endif
}

std::ifstream* surf_ifsopen(const std::string& name)
{
  xbt_assert(not name.empty());

  auto* fs = new std::ifstream();
  if (is_absolute_file_path(name)) { /* don't mess with absolute file names */
    fs->open(name.c_str(), std::ifstream::in);
  }

  /* search relative files in the path */
  for (auto const& path_elm : surf_path) {
    std::string buff = path_elm + "/" + name;
    fs->open(buff.c_str(), std::ifstream::in);

    if (not fs->fail()) {
      XBT_DEBUG("Found file at %s", buff.c_str());
      return fs;
    }
  }

  return fs;
}

FILE* surf_fopen(const std::string& name, const char* mode)
{
  FILE* file = nullptr;

  if (is_absolute_file_path(name)) /* don't mess with absolute file names */
    return fopen(name.c_str(), mode);

  /* search relative files in the path */
  for (auto const& path_elm : surf_path) {
    std::string buff = path_elm + "/" + name;
    file             = fopen(buff.c_str(), mode);

    if (file)
      return file;
  }
  return nullptr;
}

/** Displays the long description of all registered models, and quit */
void model_help(const char* category, const std::vector<surf_model_description_t>& table)
{
  XBT_HELP("Long description of the %s models accepted by this simulator:", category);
  for (auto const& item : table)
    XBT_HELP("  %s: %s", item.name, item.description);
}

const surf_model_description_t* find_model_description(const std::vector<surf_model_description_t>& table,
                                                       const std::string& name)
{
  auto pos = std::find_if(table.begin(), table.end(),
                          [&name](const surf_model_description_t& item) { return item.name == name; });
  if (pos != table.end())
    return &*pos;

  std::string sep;
  std::string name_list;
  for (auto const& item : table) {
    name_list += sep + item.name;
    sep = ", ";
  }
  xbt_die("Model '%s' is invalid! Valid models are: %s.", name.c_str(), name_list.c_str());
}

void surf_init(int* argc, char** argv)
{
  if (xbt_initialized > 0)
    return;

  xbt_init(argc, argv);

  sg_config_init(argc, argv);
}
