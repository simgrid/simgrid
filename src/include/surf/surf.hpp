/* Copyright (c) 2004-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_SURF_H
#define SURF_SURF_H

#include "simgrid/forward.h"
#include "xbt/graph.h"

/** \brief Resource model description
 */
struct surf_model_description {
  const char* name;
  const char* description;
  void_f_void_t model_init_preparse;
};
typedef struct surf_model_description s_surf_model_description_t;

XBT_PUBLIC int find_model_description(s_surf_model_description_t* table, std::string name);
XBT_PUBLIC void model_help(const char* category, s_surf_model_description_t* table);

/***************************/
/* Generic model object */
/***************************/

/** @{ @ingroup SURF_c_bindings */

/**
 * @brief Pop an action from the done actions set
 *
 * @param model The model from which the action is extracted
 * @return An action in done state
 */
XBT_PUBLIC simgrid::kernel::resource::Action*
surf_model_extract_done_action_set(simgrid::kernel::resource::Model* model);

/**
 * @brief Pop an action from the failed actions set
 *
 * @param model The model from which the action is extracted
 * @return An action in failed state
 */
XBT_PUBLIC simgrid::kernel::resource::Action*
surf_model_extract_failed_action_set(simgrid::kernel::resource::Model* model);

/**
 * @brief Get the size of the running action set of a model
 *
 * @param model The model
 * @return The size of the running action set
 */
XBT_PUBLIC int surf_model_running_action_set_size(simgrid::kernel::resource::Model* model);

/**
 * @brief [brief description]
 * @details [long description]
 *
 * @param action The surf cpu action
 * @param bound [description]
 */
XBT_PUBLIC void surf_cpu_action_set_bound(simgrid::kernel::resource::Action* action, double bound);

/** @} */

/**************************************/
/* Implementations of model object */
/**************************************/

/** \ingroup SURF_models
 *  \brief The CPU model object for the physical machine layer
 */
XBT_PUBLIC_DATA simgrid::surf::CpuModel* surf_cpu_model_pm;

/** \ingroup SURF_models
 *  \brief The CPU model object for the virtual machine layer
 */
XBT_PUBLIC_DATA simgrid::surf::CpuModel* surf_cpu_model_vm;

/** \ingroup SURF_models
 *  \brief The list of all available optimization modes (both for cpu and networks).
 *  These optimization modes can be set using --cfg=cpu/optim:... and --cfg=network/optim:...
 */
XBT_PUBLIC_DATA s_surf_model_description_t surf_optimization_mode_description[];

/** \ingroup SURF_plugins
 *  \brief The list of all available surf plugins
 */
XBT_PUBLIC_DATA s_surf_model_description_t surf_plugin_description[];

/** \ingroup SURF_models
 *  \brief The list of all available cpu model models
 */
XBT_PUBLIC_DATA s_surf_model_description_t surf_cpu_model_description[];

/** \ingroup SURF_models
 *  \brief The list of all available network model models
 */
XBT_PUBLIC_DATA s_surf_model_description_t surf_network_model_description[];

/** \ingroup SURF_models
 *  \brief The list of all available storage modes.
 *  This storage mode can be set using --cfg=storage/model:...
 */
XBT_PUBLIC_DATA s_surf_model_description_t surf_storage_model_description[];

XBT_PUBLIC_DATA simgrid::surf::StorageModel* surf_storage_model;

/** \ingroup SURF_models
 *  \brief The host model
 *
 *  Note that when you create an API on top of SURF, the host model should be the only one you use
 *  because depending on the platform model, the network model and the CPU model may not exist.
 */
XBT_PUBLIC_DATA simgrid::surf::HostModel* surf_host_model;

/** \ingroup SURF_models
 *  \brief The list of all available host model models
 */
XBT_PUBLIC_DATA s_surf_model_description_t surf_host_model_description[];

/*** SURF Globals **************************/

/** \ingroup SURF_simulation
 *  \brief Initialize SURF
 *  \param argc argument number
 *  \param argv arguments
 *
 *  This function has to be called to initialize the common structures. Then you will have to create the environment by
 *  calling  e.g. surf_host_model_init_CM02()
 *
 *  \see surf_host_model_init_CM02(), surf_host_model_init_compound(), surf_exit()
 */
XBT_PUBLIC void surf_init(int* argc, char** argv); /* initialize common structures */

/** \ingroup SURF_simulation
 *  \brief Finish simulation initialization
 *
 *  This function must be called before the first call to surf_solve()
 */
XBT_PUBLIC void surf_presolve();

/** \ingroup SURF_simulation
 *  \brief Performs a part of the simulation
 *  \param max_date Maximum date to update the simulation to, or -1
 *  \return the elapsed time, or -1.0 if no event could be executed
 *
 *  This function execute all possible events, update the action states  and returns the time elapsed.
 *  When you call execute or communicate on a model, the corresponding actions are not executed immediately but only
 *  when you call surf_solve.
 *  Note that the returned elapsed time can be zero.
 */
XBT_PUBLIC double surf_solve(double max_date);

/** \ingroup SURF_simulation
 *  \brief Return the current time
 *
 *  Return the current time in millisecond.
 */
XBT_PUBLIC double surf_get_clock();

/** \ingroup SURF_simulation
 *  \brief Exit SURF
 *
 *  Clean everything.
 *
 *  \see surf_init()
 */
XBT_PUBLIC void surf_exit();

/* surf parse file related (public because called from a test suite) */
XBT_PUBLIC void parse_platform_file(const char* file);

/********** Tracing **********/

/* instr_routing.c */
xbt_graph_t instr_routing_platform_graph();
void instr_routing_platform_graph_export_graphviz(xbt_graph_t g, const char* filename);

#endif
