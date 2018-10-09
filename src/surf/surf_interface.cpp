/* Copyright (c) 2004-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_interface.hpp"
#include "mc/mc.h"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/sg_config.hpp"
#include "src/internal_config.h"
#include "src/surf/HostImpl.hpp"
#include "src/surf/xml/platf.hpp"
#include "surf/surf.hpp"
#include "xbt/module.h"

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

std::vector<simgrid::kernel::resource::Model*> all_existing_models; /* to destroy models correctly */

simgrid::trace_mgr::future_evt_set future_evt_set;
std::vector<std::string> surf_path;
/**  set of hosts for which one want to be notified if they ever restart. */
std::set<std::string> watched_hosts;
extern std::map<std::string, simgrid::surf::StorageType*> storage_types;

s_surf_model_description_t* surf_plugin_description = nullptr;
XBT_PUBLIC void simgrid_add_plugin_description(const char* name, const char* description, void_f_void_t init_fun)
{
  static int plugin_amount = 0;

  /* no need to check for plugin name conflict: the compiler already ensures that the generated
   * simgrid_##id##_plugin_register() is unique */

  plugin_amount++;
  surf_plugin_description = static_cast<s_surf_model_description_t*>(
      xbt_realloc(surf_plugin_description, sizeof(s_surf_model_description_t) * (plugin_amount + 2)));

  surf_plugin_description[plugin_amount - 1] = {name, description, init_fun};
  surf_plugin_description[plugin_amount]     = {nullptr, nullptr, nullptr}; // this array must be null terminated
}

/* Don't forget to update the option description in smx_config when you change this */
s_surf_model_description_t surf_network_model_description[] = {
    {"LV08", "Realistic network analytic model (slow-start modeled by multiplying latency by 13.01, bandwidth by .97; "
             "bottleneck sharing uses a payload of S=20537 for evaluating RTT). ",
     &surf_network_model_init_LegrandVelho},
    {"Constant", "Simplistic network model where all communication take a constant time (one second). This model "
                 "provides the lowest realism, but is (marginally) faster.",
     &surf_network_model_init_Constant},
    {"SMPI", "Realistic network model specifically tailored for HPC settings (accurate modeling of slow start with "
             "correction factors on three intervals: < 1KiB, < 64 KiB, >= 64 KiB)",
     &surf_network_model_init_SMPI},
    {"IB", "Realistic network model specifically tailored for HPC settings, with Infiniband contention model",
     &surf_network_model_init_IB},
    {"CM02", "Legacy network analytic model (Very similar to LV08, but without corrective factors. The timings of "
             "small messages are thus poorly modeled).",
     &surf_network_model_init_CM02},
    {"NS3", "Network pseudo-model using the NS3 tcp model instead of an analytic model", &surf_network_model_init_NS3},
    {"Reno",
     "Model from Steven H. Low using lagrange_solve instead of lmm_solve (experts only; check the code for more info).",
     &surf_network_model_init_Reno},
    {"Reno2",
     "Model from Steven H. Low using lagrange_solve instead of lmm_solve (experts only; check the code for more info).",
     &surf_network_model_init_Reno2},
    {"Vegas",
     "Model from Steven H. Low using lagrange_solve instead of lmm_solve (experts only; check the code for more info).",
     &surf_network_model_init_Vegas},
    {nullptr, nullptr, nullptr} /* this array must be nullptr terminated */
};

#if ! HAVE_SMPI
void surf_network_model_init_SMPI() {
  xbt_die("Please activate SMPI support in cmake to use the SMPI network model.");
}
void surf_network_model_init_IB() {
  xbt_die("Please activate SMPI support in cmake to use the IB network model.");
}
#endif
#if !SIMGRID_HAVE_NS3
void surf_network_model_init_NS3() {
  xbt_die("Please activate NS3 support in cmake and install the dependencies to use the NS3 network model.");
}
#endif

s_surf_model_description_t surf_cpu_model_description[] = {
  {"Cas01", "Simplistic CPU model (time=size/power).", &surf_cpu_model_init_Cas01},
  {nullptr, nullptr,  nullptr}      /* this array must be nullptr terminated */
};

s_surf_model_description_t surf_host_model_description[] = {
  {"default",   "Default host model. Currently, CPU:Cas01 and network:LV08 (with cross traffic enabled)", &surf_host_model_init_current_default},
  {"compound",  "Host model that is automatically chosen if you change the network and CPU models", &surf_host_model_init_compound},
  {"ptask_L07", "Host model somehow similar to Cas01+CM02 but allowing parallel tasks", &surf_host_model_init_ptask_L07},
  {nullptr, nullptr, nullptr}      /* this array must be nullptr terminated */
};

s_surf_model_description_t surf_optimization_mode_description[] = {
  {"Lazy", "Lazy action management (partial invalidation in lmm + heap in action remaining).", nullptr},
  {"TI",   "Trace integration. Highly optimized mode when using availability traces (only available for the Cas01 CPU model for now).", nullptr},
  {"Full", "Full update of remaining and variables. Slow but may be useful when debugging.", nullptr},
  {nullptr, nullptr, nullptr}      /* this array must be nullptr terminated */
};

s_surf_model_description_t surf_storage_model_description[] = {
  {"default", "Simplistic storage model.", &surf_storage_model_init_default},
  {nullptr, nullptr,  nullptr}      /* this array must be nullptr terminated */
};

double NOW = 0;

double surf_get_clock()
{
  return NOW;
}

/* returns whether #file_path is a absolute file path. Surprising, isn't it ? */
static bool is_absolute_file_path(std::string file_path)
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

std::ifstream* surf_ifsopen(std::string name)
{
  xbt_assert(not name.empty());

  std::ifstream* fs = new std::ifstream();
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

FILE* surf_fopen(std::string name, const char* mode)
{
  FILE *file = nullptr;

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
void model_help(const char *category, s_surf_model_description_t * table)
{
  printf("Long description of the %s models accepted by this simulator:\n", category);
  for (int i = 0; table[i].name; i++)
    printf("  %s: %s\n", table[i].name, table[i].description);
}

int find_model_description(s_surf_model_description_t* table, std::string name)
{
  for (int i = 0; table[i].name; i++)
    if (name == table[i].name)
      return i;

  if (not table[0].name)
    xbt_die("No model is valid! This is a bug.");

  std::string name_list = std::string(table[0].name);
  for (int i = 1; table[i].name; i++)
    name_list = name_list + ", " + table[i].name;

  xbt_die("Model '%s' is invalid! Valid models are: %s.", name.c_str(), name_list.c_str());
  return -1;
}

void sg_version_check(int lib_version_major, int lib_version_minor, int lib_version_patch)
{
  if ((lib_version_major != SIMGRID_VERSION_MAJOR) || (lib_version_minor != SIMGRID_VERSION_MINOR)) {
    fprintf(stderr, "FATAL ERROR: Your program was compiled with SimGrid version %d.%d.%d, "
                    "and then linked against SimGrid %d.%d.%d. Please fix this.\n",
            lib_version_major, lib_version_minor, lib_version_patch, SIMGRID_VERSION_MAJOR, SIMGRID_VERSION_MINOR,
            SIMGRID_VERSION_PATCH);
    abort();
  }
  if (lib_version_patch != SIMGRID_VERSION_PATCH) {
    if (SIMGRID_VERSION_PATCH >= 90 || lib_version_patch >= 90) {
      fprintf(
          stderr,
          "FATAL ERROR: Your program was compiled with SimGrid version %d.%d.%d, "
          "and then linked against SimGrid %d.%d.%d. \n"
          "One of them is a development version, and should not be mixed with the stable release. Please fix this.\n",
          lib_version_major, lib_version_minor, lib_version_patch, SIMGRID_VERSION_MAJOR, SIMGRID_VERSION_MINOR,
          SIMGRID_VERSION_PATCH);
      abort();
    }
    fprintf(stderr, "Warning: Your program was compiled with SimGrid version %d.%d.%d, "
                    "and then linked against SimGrid %d.%d.%d. Proceeding anyway.\n",
            lib_version_major, lib_version_minor, lib_version_patch, SIMGRID_VERSION_MAJOR, SIMGRID_VERSION_MINOR,
            SIMGRID_VERSION_PATCH);
  }
}

void sg_version_get(int* ver_major, int* ver_minor, int* ver_patch)
{
  *ver_major = SIMGRID_VERSION_MAJOR;
  *ver_minor = SIMGRID_VERSION_MINOR;
  *ver_patch = SIMGRID_VERSION_PATCH;
}

void sg_version()
{
  std::printf("This program was linked against %s (git: %s), found in %s.\n",
              SIMGRID_VERSION_STRING, SIMGRID_GIT_VERSION, SIMGRID_INSTALL_PREFIX);

#if SIMGRID_HAVE_MC
  std::printf("   Model-checking support compiled in.\n");
#else
  std::printf("   Model-checking support disabled at compilation.\n");
#endif

#if SIMGRID_HAVE_NS3
  std::printf("   NS3 support compiled in.\n");
#else
  std::printf("   NS3 support disabled at compilation.\n");
#endif

#if SIMGRID_HAVE_JEDULE
  std::printf("   Jedule support compiled in.\n");
#else
  std::printf("   Jedule support disabled at compilation.\n");
#endif

#if SIMGRID_HAVE_LUA
  std::printf("   Lua support compiled in.\n");
#else
  std::printf("   Lua support disabled at compilation.\n");
#endif

#if SIMGRID_HAVE_MALLOCATOR
  std::printf("   Mallocator support compiled in.\n");
#else
  std::printf("   Mallocator support disabled at compilation.\n");
#endif

  std::printf("\nTo cite SimGrid in a publication, please use:\n"
              "   Henri Casanova, Arnaud Giersch, Arnaud Legrand, Martin Quinson, Frédéric Suter. \n"
              "   Versatile, Scalable, and Accurate Simulation of Distributed Applications and Platforms. \n"
              "   Journal of Parallel and Distributed Computing, Elsevier, 2014, 74 (10), pp.2899-2917.\n");
  std::printf("The pdf file and a BibTeX entry for LaTeX users can be found at http://hal.inria.fr/hal-01017319\n");
}

void surf_init(int *argc, char **argv)
{
  if (USER_HOST_LEVEL != -1) // Already initialized
    return;

  XBT_DEBUG("Create all Libs");
  USER_HOST_LEVEL = simgrid::s4u::Host::extension_create(nullptr);

  xbt_init(argc, argv);

  sg_config_init(argc, argv);

  if (MC_is_active())
    MC_memory_init();
}

void surf_exit()
{
  simgrid::s4u::Engine::shutdown();
  for (auto const& e : storage_types) {
    simgrid::surf::StorageType* stype = e.second;
    delete stype->properties;
    delete stype->model_properties;
    delete stype;
  }

  for (auto const& model : all_existing_models)
    delete model;

  xbt_free(surf_plugin_description);

  tmgr_finalize();
  sg_platf_exit();

  NOW = 0;                      /* Just in case the user plans to restart the simulation afterward */
}
