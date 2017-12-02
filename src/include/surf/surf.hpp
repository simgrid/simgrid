/* Copyright (c) 2004-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_SURF_H
#define SURF_SURF_H

#include "simgrid/datatypes.h"
#include "simgrid/forward.h"
#include "src/internal_config.h"
#include "xbt/config.h"
#include "xbt/dict.h"
#include "xbt/dynar.h"
#include "xbt/graph.h"
#include "xbt/misc.h"

#ifndef __cplusplus
#error This is a C++ only file, now
#endif

namespace simgrid {
namespace surf {
class Model;
class CpuModel;
class HostModel;
class NetworkModel;
class StorageModel;
class NetworkCm02Link;
class Action;
}
}

/** @ingroup SURF_c_bindings
 *  \brief Model datatype
 *
 *  Generic data structure for a model. The hosts,
 *  the CPUs and the network links are examples of models.
 */
typedef simgrid::surf::Model* surf_model_t;
typedef simgrid::surf::CpuModel* surf_cpu_model_t;
typedef simgrid::surf::HostModel* surf_host_model_t;
typedef simgrid::surf::NetworkModel* surf_network_model_t;
typedef simgrid::surf::StorageModel* surf_storage_model_t;
/** @ingroup SURF_c_bindings
 *  \brief Action structure
 *
 *  Never create s_surf_action_t by yourself ! The actions are created
 *  on the fly when you call execute or communicate on a model.
 *
 *  \see e_surf_action_state_t
 */
typedef simgrid::surf::Action* surf_action_t;

SG_BEGIN_DECL()
/* Actions and models are highly connected structures... */

/* user-visible parameters */
extern XBT_PRIVATE double sg_tcp_gamma;
extern XBT_PRIVATE double sg_latency_factor;
extern XBT_PRIVATE double sg_bandwidth_factor;
extern XBT_PRIVATE double sg_weight_S_parameter;
extern XBT_PRIVATE int sg_network_crosstraffic;

/** \brief Resource model description
 */
struct surf_model_description {
  const char* name;
  const char* description;
  void_f_void_t model_init_preparse;
};
typedef struct surf_model_description s_surf_model_description_t;

XBT_PUBLIC(int) find_model_description(s_surf_model_description_t* table, std::string name);
XBT_PUBLIC(void) model_help(const char* category, s_surf_model_description_t* table);

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
XBT_PUBLIC(surf_action_t) surf_model_extract_done_action_set(surf_model_t model);

/**
 * @brief Pop an action from the failed actions set
 *
 * @param model The model from which the action is extracted
 * @return An action in failed state
 */
XBT_PUBLIC(surf_action_t) surf_model_extract_failed_action_set(surf_model_t model);

/**
 * @brief Get the size of the running action set of a model
 *
 * @param model The model
 * @return The size of the running action set
 */
XBT_PUBLIC(int) surf_model_running_action_set_size(surf_model_t model);

/**
 * @brief [brief description]
 * @details [long description]
 *
 * @param action The surf cpu action
 * @param bound [description]
 */
XBT_PUBLIC(void) surf_cpu_action_set_bound(surf_action_t action, double bound);

/**
 * @brief [brief description]
 * @details [long description]
 *
 * @param action The surf network action
 */
XBT_PUBLIC(double) surf_network_action_get_latency_limited(surf_action_t action);

/** @} */

/**************************************/
/* Implementations of model object */
/**************************************/

/** \ingroup SURF_models
 *  \brief The CPU model object for the physical machine layer
 */
XBT_PUBLIC_DATA(surf_cpu_model_t) surf_cpu_model_pm;

/** \ingroup SURF_models
 *  \brief The CPU model object for the virtual machine layer
 */
XBT_PUBLIC_DATA(surf_cpu_model_t) surf_cpu_model_vm;

/** \ingroup SURF_models
 *  \brief Initializes the CPU model with the model Cas01
 *
 *  By default, this model uses the lazy optimization mechanism that relies on partial invalidation in LMM and a heap
 *  for lazy action update.
 *  You can change this behavior by setting the cpu/optim configuration variable to a different value.
 *
 *  You shouldn't have to call it by yourself.
 */
XBT_PUBLIC(void) surf_cpu_model_init_Cas01();

/** \ingroup SURF_models
 *  \brief Initializes the CPU model with trace integration [Deprecated]
 *
 *  You shouldn't have to call it by yourself.
 */
XBT_PUBLIC(void) surf_cpu_model_init_ti();

/** \ingroup SURF_models
 *  \brief The list of all available optimization modes (both for cpu and networks).
 *  These optimization modes can be set using --cfg=cpu/optim:... and --cfg=network/optim:...
 */
XBT_PUBLIC_DATA(s_surf_model_description_t) surf_optimization_mode_description[];

/** \ingroup SURF_plugins
 *  \brief The list of all available surf plugins
 */
XBT_PUBLIC_DATA(s_surf_model_description_t) surf_plugin_description[];

/** \ingroup SURF_models
 *  \brief The list of all available cpu model models
 */
XBT_PUBLIC_DATA(s_surf_model_description_t) surf_cpu_model_description[];

/** \ingroup SURF_models
 *  \brief The network model
 *
 *  When creating a new API on top on SURF, you shouldn't use the network model unless you know what you are doing.
 *  Only the host model should be accessed because depending on the platform model, the network model can be NULL.
 */
XBT_PUBLIC_DATA(surf_network_model_t) surf_network_model;

/** \ingroup SURF_models
 *  \brief Same as network model 'LagrangeVelho', only with different correction factors.
 *
 * This model is proposed by Pierre-Nicolas Clauss and Martin Quinson and Stéphane Génaud based on the model 'LV08' and
 * different correction factors depending on the communication size (< 1KiB, < 64KiB, >= 64KiB).
 * See comments in the code for more information.
 *
 *  \see surf_host_model_init_SMPI()
 */
XBT_PUBLIC(void) surf_network_model_init_SMPI();

/** \ingroup SURF_models
 *  \brief Same as network model 'LagrangeVelho', only with different correction factors.
 *
 * This model impelments a variant of the contention model on Infinband networks based on
 * the works of Jérôme Vienne : http://mescal.imag.fr/membres/jean-marc.vincent/index.html/PhD/Vienne.pdf
 *
 *  \see surf_host_model_init_IB()
 */
XBT_PUBLIC(void) surf_network_model_init_IB();

/** \ingroup SURF_models
 *  \brief Initializes the platform with the network model 'LegrandVelho'
 *
 * This model is proposed by Arnaud Legrand and Pedro Velho based on the results obtained with the GTNets simulator for
 * onelink and dogbone sharing scenarios. See comments in the code for more information.
 *
 *  \see surf_host_model_init_LegrandVelho()
 */
XBT_PUBLIC(void) surf_network_model_init_LegrandVelho();

/** \ingroup SURF_models
 *  \brief Initializes the platform with the network model 'Constant'
 *
 *  In this model, the communication time between two network cards is constant, hence no need for a routing table.
 *  This is particularly useful when simulating huge distributed algorithms where scalability is really an issue. This
 *  function is called in conjunction with surf_host_model_init_compound.
 *
 *  \see surf_host_model_init_compound()
 */
XBT_PUBLIC(void) surf_network_model_init_Constant();

/** \ingroup SURF_models
 *  \brief Initializes the platform with the network model CM02
 *
 *  You sould call this function by yourself only if you plan using surf_host_model_init_compound.
 *  See comments in the code for more information.
 */
XBT_PUBLIC(void) surf_network_model_init_CM02();

/** \ingroup SURF_models
 *  \brief Initializes the platform with the network model NS3
 *
 *  This function is called by surf_host_model_init_NS3 or by yourself only if you plan using
 *  surf_host_model_init_compound
 *
 *  \see surf_host_model_init_NS3()
 */
XBT_PUBLIC(void) surf_network_model_init_NS3();

/** \ingroup SURF_models
 *  \brief Initializes the platform with the network model Reno
 *
 *  The problem is related to max( sum( arctan(C * Df * xi) ) ).
 *
 *  Reference:
 *  [LOW03] S. H. Low. A duality model of TCP and queue management algorithms.
 *  IEEE/ACM Transaction on Networking, 11(4):525-536, 2003.
 *
 *  Call this function only if you plan using surf_host_model_init_compound.
 */
XBT_PUBLIC(void) surf_network_model_init_Reno();

/** \ingroup SURF_models
 *  \brief Initializes the platform with the network model Reno2
 *
 *  The problem is related to max( sum( arctan(C * Df * xi) ) ).
 *
 *  Reference:
 *  [LOW01] S. H. Low. A duality model of TCP and queue management algorithms.
 *  IEEE/ACM Transaction on Networking, 11(4):525-536, 2003.
 *
 *  Call this function only if you plan using surf_host_model_init_compound.
 */
XBT_PUBLIC(void) surf_network_model_init_Reno2();

/** \ingroup SURF_models
 *  \brief Initializes the platform with the network model Vegas
 *
 *  This problem is related to max( sum( a * Df * ln(xi) ) ) which is equivalent  to the proportional fairness.
 *
 *  Reference:
 *  [LOW03] S. H. Low. A duality model of TCP and queue management algorithms.
 *  IEEE/ACM Transaction on Networking, 11(4):525-536, 2003.
 *
 *  Call this function only if you plan using surf_host_model_init_compound.
 */
XBT_PUBLIC(void) surf_network_model_init_Vegas();

/** \ingroup SURF_models
 *  \brief The list of all available network model models
 */
XBT_PUBLIC_DATA(s_surf_model_description_t) surf_network_model_description[];

/** \ingroup SURF_models
 *  \brief The storage model
 */
XBT_PUBLIC(void) surf_storage_model_init_default();

/** \ingroup SURF_models
 *  \brief The list of all available storage modes.
 *  This storage mode can be set using --cfg=storage/model:...
 */
XBT_PUBLIC_DATA(s_surf_model_description_t) surf_storage_model_description[];

XBT_PUBLIC_DATA(surf_storage_model_t) surf_storage_model;

/** \ingroup SURF_models
 *  \brief The host model
 *
 *  Note that when you create an API on top of SURF, the host model should be the only one you use
 *  because depending on the platform model, the network model and the CPU model may not exist.
 */
XBT_PUBLIC_DATA(surf_host_model_t) surf_host_model;

/** \ingroup SURF_models
 *  \brief Initializes the platform with a compound host model
 *
 *  This function should be called after a cpu_model and a network_model have been set up.
 */
XBT_PUBLIC(void) surf_host_model_init_compound();

/** \ingroup SURF_models
 *  \brief Initializes the platform with the current best network and cpu models at hand
 *
 *  This platform model separates the host model and the network model.
 *  The host model will be initialized with the model compound, the network model with the model LV08 (with cross
 *  traffic support) and the CPU model with the model Cas01.
 *  Such model is subject to modification with warning in the ChangeLog so monitor it!
 */
XBT_PUBLIC(void) surf_host_model_init_current_default();

/** \ingroup SURF_models
 *  \brief Initializes the platform with the model L07
 *
 *  With this model, only parallel tasks can be used. Resource sharing is done by identifying bottlenecks and giving an
 *  equal share of the model to each action.
 */
XBT_PUBLIC(void) surf_host_model_init_ptask_L07();

/** \ingroup SURF_models
 *  \brief The list of all available host model models
 */
XBT_PUBLIC_DATA(s_surf_model_description_t) surf_host_model_description[];

/** \ingroup SURF_models
 *  \brief Initializes the platform with the current best network and cpu models at hand
 *
 *  This platform model seperates the host model and the network model.
 *  The host model will be initialized with the model compound, the network model with the model LV08 (with cross
 *  traffic support) and the CPU model with the model Cas01.
 *  Such model is subject to modification with warning in the ChangeLog so monitor it!
 */
XBT_PUBLIC(void) surf_vm_model_init_HL13();

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
XBT_PUBLIC(void) surf_init(int* argc, char** argv); /* initialize common structures */

/** \ingroup SURF_simulation
 *  \brief Finish simulation initialization
 *
 *  This function must be called before the first call to surf_solve()
 */
XBT_PUBLIC(void) surf_presolve();

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
XBT_PUBLIC(double) surf_solve(double max_date);

/** \ingroup SURF_simulation
 *  \brief Return the current time
 *
 *  Return the current time in millisecond.
 */
XBT_PUBLIC(double) surf_get_clock();

/** \ingroup SURF_simulation
 *  \brief Exit SURF
 *
 *  Clean everything.
 *
 *  \see surf_init()
 */
XBT_PUBLIC(void) surf_exit();

/* surf parse file related (public because called from a test suite) */
XBT_PUBLIC(void) parse_platform_file(const char* file);

/********** Tracing **********/
/* from surf_instr.c */
void TRACE_surf_action(surf_action_t surf_action, const char* category);

/* instr_routing.c */
void instr_routing_define_callbacks();
int instr_platform_traced();
xbt_graph_t instr_routing_platform_graph();
void instr_routing_platform_graph_export_graphviz(xbt_graph_t g, const char* filename);

SG_END_DECL()

#endif
