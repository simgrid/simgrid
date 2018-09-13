/* Copyright (c) 2004-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_MODEL_H_
#define SURF_MODEL_H_

#include "src/surf/surf_private.hpp"

#include <cmath>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

/*********
 * Utils *
 *********/

/* user-visible parameters */
XBT_PUBLIC_DATA double sg_maxmin_precision;
XBT_PUBLIC_DATA double sg_surf_precision;
XBT_PUBLIC_DATA int sg_concurrency_limit;

extern XBT_PRIVATE double sg_latency_factor;
extern XBT_PRIVATE double sg_bandwidth_factor;
extern XBT_PRIVATE double sg_weight_S_parameter;
extern XBT_PRIVATE std::vector<std::string> surf_path;
extern XBT_PRIVATE std::unordered_map<std::string, tmgr_trace_t> traces_set_list;
extern XBT_PRIVATE std::set<std::string> watched_hosts;

static inline void double_update(double* variable, double value, double precision)
{
  // printf("Updating %g -= %g +- %g\n",*variable,value,precision);
  // xbt_assert(value==0  || value>precision);
  // Check that precision is higher than the machine-dependent size of the mantissa. If not, brutal rounding  may
  // happen, and the precision mechanism is not active...
  // xbt_assert(*variable< (2<<DBL_MANT_DIG)*precision && FLT_RADIX==2);
  *variable -= value;
  if (*variable < precision)
    *variable = 0.0;
}

static inline int double_positive(double value, double precision)
{
  return (value > precision);
}

static inline int double_equals(double value1, double value2, double precision)
{
  return (fabs(value1 - value2) < precision);
}

/** @ingroup SURF_models
 *  @brief Initializes the CPU model with the model Cas01
 *
 *  By default, this model uses the lazy optimization mechanism that relies on partial invalidation in LMM and a heap
 *  for lazy action update.
 *  You can change this behavior by setting the cpu/optim configuration variable to a different value.
 *
 *  You shouldn't have to call it by yourself.
 */
XBT_PUBLIC void surf_cpu_model_init_Cas01();

/** @ingroup SURF_models
 *  @brief Same as network model 'LagrangeVelho', only with different correction factors.
 *
 * This model is proposed by Pierre-Nicolas Clauss and Martin Quinson and Stéphane Génaud based on the model 'LV08' and
 * different correction factors depending on the communication size (< 1KiB, < 64KiB, >= 64KiB).
 * See comments in the code for more information.
 *
 *  @see surf_host_model_init_SMPI()
 */
XBT_PUBLIC void surf_network_model_init_SMPI();

/** @ingroup SURF_models
 *  @brief Same as network model 'LagrangeVelho', only with different correction factors.
 *
 * This model impelments a variant of the contention model on Infinband networks based on
 * the works of Jérôme Vienne : http://mescal.imag.fr/membres/jean-marc.vincent/index.html/PhD/Vienne.pdf
 *
 *  @see surf_host_model_init_IB()
 */
XBT_PUBLIC void surf_network_model_init_IB();

/** @ingroup SURF_models
 *  @brief Initializes the platform with the network model 'LegrandVelho'
 *
 * This model is proposed by Arnaud Legrand and Pedro Velho based on the results obtained with the GTNets simulator for
 * onelink and dogbone sharing scenarios. See comments in the code for more information.
 *
 *  @see surf_host_model_init_LegrandVelho()
 */
XBT_PUBLIC void surf_network_model_init_LegrandVelho();

/** @ingroup SURF_models
 *  @brief Initializes the platform with the network model 'Constant'
 *
 *  In this model, the communication time between two network cards is constant, hence no need for a routing table.
 *  This is particularly useful when simulating huge distributed algorithms where scalability is really an issue. This
 *  function is called in conjunction with surf_host_model_init_compound.
 *
 *  @see surf_host_model_init_compound()
 */
XBT_PUBLIC void surf_network_model_init_Constant();

/** @ingroup SURF_models
 *  @brief Initializes the platform with the network model CM02
 *
 *  You sould call this function by yourself only if you plan using surf_host_model_init_compound.
 *  See comments in the code for more information.
 */
XBT_PUBLIC void surf_network_model_init_CM02();

/** @ingroup SURF_models
 *  @brief Initializes the platform with the network model NS3
 *
 *  This function is called by surf_host_model_init_NS3 or by yourself only if you plan using
 *  surf_host_model_init_compound
 *
 *  @see surf_host_model_init_NS3()
 */
XBT_PUBLIC void surf_network_model_init_NS3();

/** @ingroup SURF_models
 *  @brief Initializes the platform with the network model Reno
 *
 *  The problem is related to max( sum( arctan(C * Df * xi) ) ).
 *
 *  Reference:
 *  [LOW03] S. H. Low. A duality model of TCP and queue management algorithms.
 *  IEEE/ACM Transaction on Networking, 11(4):525-536, 2003.
 *
 *  Call this function only if you plan using surf_host_model_init_compound.
 */
XBT_PUBLIC void surf_network_model_init_Reno();

/** @ingroup SURF_models
 *  @brief Initializes the platform with the network model Reno2
 *
 *  The problem is related to max( sum( arctan(C * Df * xi) ) ).
 *
 *  Reference:
 *  [LOW01] S. H. Low. A duality model of TCP and queue management algorithms.
 *  IEEE/ACM Transaction on Networking, 11(4):525-536, 2003.
 *
 *  Call this function only if you plan using surf_host_model_init_compound.
 */
XBT_PUBLIC void surf_network_model_init_Reno2();

/** @ingroup SURF_models
 *  @brief Initializes the platform with the network model Vegas
 *
 *  This problem is related to max( sum( a * Df * ln(xi) ) ) which is equivalent  to the proportional fairness.
 *
 *  Reference:
 *  [LOW03] S. H. Low. A duality model of TCP and queue management algorithms.
 *  IEEE/ACM Transaction on Networking, 11(4):525-536, 2003.
 *
 *  Call this function only if you plan using surf_host_model_init_compound.
 */
XBT_PUBLIC void surf_network_model_init_Vegas();

/** @ingroup SURF_models
 *  @brief Initializes the platform with the current best network and cpu models at hand
 *
 *  This platform model seperates the host model and the network model.
 *  The host model will be initialized with the model compound, the network model with the model LV08 (with cross
 *  traffic support) and the CPU model with the model Cas01.
 *  Such model is subject to modification with warning in the ChangeLog so monitor it!
 */
XBT_PUBLIC void surf_vm_model_init_HL13();

/** @ingroup SURF_models
 *  @brief Initializes the platform with a compound host model
 *
 *  This function should be called after a cpu_model and a network_model have been set up.
 */
XBT_PUBLIC void surf_host_model_init_compound();

/** @ingroup SURF_models
 *  @brief Initializes the platform with the current best network and cpu models at hand
 *
 *  This platform model separates the host model and the network model.
 *  The host model will be initialized with the model compound, the network model with the model LV08 (with cross
 *  traffic support) and the CPU model with the model Cas01.
 *  Such model is subject to modification with warning in the ChangeLog so monitor it!
 */
XBT_PUBLIC void surf_host_model_init_current_default();

/** @ingroup SURF_models
 *  @brief Initializes the platform with the model L07
 *
 *  With this model, only parallel tasks can be used. Resource sharing is done by identifying bottlenecks and giving an
 *  equal share of the model to each action.
 */
XBT_PUBLIC void surf_host_model_init_ptask_L07();

/** @ingroup SURF_models
 *  @brief The storage model
 */
XBT_PUBLIC void surf_storage_model_init_default();

/* --------------------
 *  Model Descriptions
 * -------------------- */
/** @brief Resource model description */
struct surf_model_description {
  const char* name;
  const char* description;
  void_f_void_t model_init_preparse;
};
typedef struct surf_model_description s_surf_model_description_t;

XBT_PUBLIC int find_model_description(s_surf_model_description_t* table, std::string name);
XBT_PUBLIC void model_help(const char* category, s_surf_model_description_t* table);

#define SIMGRID_REGISTER_PLUGIN(id, desc, init)                       \
  void simgrid_##id##_plugin_register();                              \
  void XBT_ATTRIB_CONSTRUCTOR(800) simgrid_##id##_plugin_register() { \
    simgrid_add_plugin_description(#id, desc, init);                  \
  }

XBT_PUBLIC void simgrid_add_plugin_description(const char* name, const char* description, void_f_void_t init_fun);

/** @brief The list of all available plugins */
XBT_PUBLIC_DATA s_surf_model_description_t* surf_plugin_description;
/** @brief The list of all available optimization modes (both for cpu and networks).
 *  These optimization modes can be set using --cfg=cpu/optim:... and --cfg=network/optim:... */
XBT_PUBLIC_DATA s_surf_model_description_t surf_optimization_mode_description[];
/** @brief The list of all cpu models (pick one with --cfg=cpu/model) */
XBT_PUBLIC_DATA s_surf_model_description_t surf_cpu_model_description[];
/** @brief The list of all network models (pick one with --cfg=network/model) */
XBT_PUBLIC_DATA s_surf_model_description_t surf_network_model_description[];
/** @brief The list of all storage models (pick one with --cfg=storage/model) */
XBT_PUBLIC_DATA s_surf_model_description_t surf_storage_model_description[];
/** @brief The list of all host models (pick one with --cfg=host/model:) */
XBT_PUBLIC_DATA s_surf_model_description_t surf_host_model_description[];

/**********
 * Action *
 **********/

#endif /* SURF_MODEL_H_ */
