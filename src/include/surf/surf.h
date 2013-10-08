/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_SURF_H
#define _SURF_SURF_H

#include "xbt/swag.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"
#include "xbt/graph.h"
#include "xbt/misc.h"
#include "portable.h"
#include "xbt/config.h"
#include "surf/datatypes.h"
#include "xbt/lib.h"
#include "surf/surf_routing.h"
#include "simgrid/platf_interface.h"

SG_BEGIN_DECL()
/* Actions and models are highly connected structures... */

/* user-visible parameters */
extern double sg_tcp_gamma;
extern double sg_sender_gap;
extern double sg_latency_factor;
extern double sg_bandwidth_factor;
extern double sg_weight_S_parameter;
extern int sg_network_crosstraffic;
#ifdef HAVE_GTNETS
extern double sg_gtnets_jitter;
extern int sg_gtnets_jitter_seed;
#endif
extern xbt_dynar_t surf_path;

typedef enum {
  SURF_NETWORK_ELEMENT_NULL = 0,        /* NULL */
  SURF_NETWORK_ELEMENT_HOST,    /* host type */
  SURF_NETWORK_ELEMENT_ROUTER,  /* router type */
  SURF_NETWORK_ELEMENT_AS       /* AS type */
} e_surf_network_element_type_t;

#ifdef __cplusplus
class Model;
class CpuModel;
class WorkstationModel;
class NetworkCm02Model;
class Resource;
class ResourceLmm;
class WorkstationCLM03;
class NetworkCm02Link;
class Cpu;
class Action;
class ActionLmm;
class StorageActionLmm;
class As;
class RoutingEdge;
class RoutingPlatf;
#else
typedef struct Model Model;
typedef struct CpuModel CpuModel;
typedef struct WorkstationModel WorkstationModel;
typedef struct NetworkCm02Model NetworkCm02Model;
typedef struct Resource Resource;
typedef struct ResourceLmm ResourceLmm;
typedef struct WorkstationCLM03 WorkstationCLM03;
typedef struct NetworkCm02Link NetworkCm02Link;
typedef struct Cpu Cpu;
typedef struct Action Action;
typedef struct ActionLmm ActionLmm;
typedef struct StorageActionLmm StorageActionLmm;
typedef struct As As;
typedef struct RoutingEdge RoutingEdge;
typedef struct RoutingPlatf RoutingPlatf;
#endif

/** \ingroup SURF_models
 *  \brief Model datatype
 *
 *  Generic data structure for a model. The workstations,
 *  the CPUs and the network links are examples of models.
 */
typedef Model *surf_model_t;
typedef CpuModel *surf_cpu_model_t;
typedef WorkstationModel *surf_workstation_model_t;
typedef NetworkCm02Model *surf_network_model_t;

typedef Resource *surf_resource_t;
typedef ResourceLmm *surf_resource_lmm_t;
typedef WorkstationCLM03 *surf_workstation_CLM03_t;
typedef xbt_dictelm_t surf_workstation_t;
typedef NetworkCm02Link *surf_network_link_t;
typedef Cpu *surf_cpu_t;

/** \ingroup SURF_actions
 *  \brief Action structure
 *
 *  Never create s_surf_action_t by yourself ! The actions are created
 *  on the fly when you call execute or communicate on a model.
 *
 *  \see e_surf_action_state_t
 */
typedef Action *surf_action_t;
typedef ActionLmm *surf_action_lmm_t;
typedef StorageActionLmm *surf_storage_action_lmm_t;

typedef As *AS_t;
typedef RoutingEdge *routing_edge_t;
typedef RoutingPlatf *routing_platf_t;

typedef struct surf_file *surf_file_t;

XBT_PUBLIC(e_surf_network_element_type_t)
  routing_get_network_element_type(const char* name);

/** @Brief Specify that we use that action */
XBT_PUBLIC(void) surf_action_ref(surf_action_t action);

/** @brief Creates a new action.
 *
 * @param size The size is the one of the subtype you want to create
 * @param cost initial value
 * @param model to which model we should attach this action
 * @param failed whether we should start this action in failed mode
 */
XBT_PUBLIC(void *) surf_action_new(size_t size, double cost,
                                   surf_model_t model, int failed);

/** \brief Resource model description
 */
typedef struct surf_model_description {
  const char *name;
  const char *description;
  void_f_void_t model_init_preparse;
} s_surf_model_description_t, *surf_model_description_t;

XBT_PUBLIC(int) find_model_description(s_surf_model_description_t * table,
                                       const char *name);
XBT_PUBLIC(void) model_help(const char *category,
                            s_surf_model_description_t * table);



/** \ingroup SURF_actions
 *  \brief Action states
 *
 *  Action states.
 *
 *  \see surf_action_t, surf_action_state_t
 */

typedef enum {
  SURF_ACTION_READY = 0,        /**< Ready        */
  SURF_ACTION_RUNNING,          /**< Running      */
  SURF_ACTION_FAILED,           /**< Task Failure */
  SURF_ACTION_DONE,             /**< Completed    */
  SURF_ACTION_TO_FREE,          /**< Action to free in next cleanup */
  SURF_ACTION_NOT_IN_THE_SYSTEM
                                /**< Not in the system anymore. Why did you ask ? */
} e_surf_action_state_t;

/***************************/
/* Generic model object */
/***************************/
//FIXME:REMOVE typedef struct s_routing_platf s_routing_platf_t, *routing_platf_t;
XBT_PUBLIC_DATA(routing_platf_t) routing_platf;

/*******************************************
 *  TUTORIAL: New model
 *  New model extension public
 *  Public functions specific to a New model.
 */
typedef struct surf_new_model_extension_public {
  surf_action_t(*fct) ();
  void* (*create_resource) ();
} s_surf_model_extension_new_model_t;
/*******************************************/

/** \ingroup SURF_models
 *  \brief Private data available on all models
 */
typedef struct surf_model_private *surf_model_private_t;

     /* Cpu model */

     /** \ingroup SURF_models
      *  \brief CPU model extension public
      *
      *  Public functions specific to the CPU model.
      */
typedef struct surf_cpu_model_extension_public {
  surf_action_t(*execute) (void *cpu, double size);
  surf_action_t(*sleep) (void *cpu, double duration);
  e_surf_resource_state_t(*get_state) (void *cpu);
  int (*get_core) (void *cpu);
  double (*get_speed) (void *cpu, double load);
  double (*get_available_speed) (void *cpu);
  void (*add_traces) (void);
} s_surf_model_extension_cpu_t;

     /* Network model */

     /** \ingroup SURF_models
      *  \brief Network model extension public
      *
      *  Public functions specific to the network model
      */
typedef struct surf_network_model_extension_public {
  surf_action_t (*communicate) (sg_routing_edge_t src,
                                sg_routing_edge_t dst,
                                double size, double rate);
  xbt_dynar_t(*get_route) (void *src, void *dst); //FIXME: kill field? That is done by the routing nowadays
  double (*get_link_bandwidth) (const void *link);
  double (*get_link_latency) (const void *link);
  int (*link_shared) (const void *link);
  void (*add_traces) (void);
} s_surf_model_extension_network_t;

/* Storage model */
//
/** \ingroup SURF_models
 *  \brief Storage model extension public
 *
 *  Public functions specific to the Storage model.
 */

typedef struct surf_storage_model_extension_public {
  surf_action_t(*open) (void *storage, const char* mount, const char* path);
  surf_action_t(*close) (void *storage, surf_file_t fd);
  surf_action_t(*read) (void *storage, void* ptr, size_t size,
                        surf_file_t fd);
  surf_action_t(*write) (void *storage, const void* ptr, size_t size,
                         surf_file_t fd);
  surf_action_t(*stat) (void *storage, surf_file_t fd);
  surf_action_t(*ls) (void *storage, const char *path);
} s_surf_model_extension_storage_t;

     /** \ingroup SURF_models
      *  \brief Workstation model extension public
      *
      *  Public functions specific to the workstation model.
      */
typedef struct surf_workstation_model_extension_public {
  surf_action_t(*execute) (void *workstation, double size);                                /**< Execute a computation amount on a workstation
                                      and create the corresponding action */
  surf_action_t(*sleep) (void *workstation, double duration);                              /**< Make a workstation sleep during a given duration */
  e_surf_resource_state_t(*get_state) (void *workstation);                                      /**< Return the CPU state of a workstation */
  int (*get_core) (void *workstation); 
  double (*get_speed) (void *workstation, double load);                                    /**< Return the speed of a workstation */
  double (*get_available_speed) (void *workstation);                                       /**< Return tha available speed of a workstation */
   surf_action_t(*communicate) (void *workstation_src,                                     /**< Execute a communication amount between two workstations */
                                void *workstation_dst, double size,
                                double max_rate);
   // FIXME: kill next field, which duplicates the routing
   xbt_dynar_t(*get_route) (void *workstation_src, void *workstation_dst);                 /**< Get the list of links between two ws */

   surf_action_t(*execute_parallel_task) (int workstation_nb,                              /**< Execute a parallel task on several workstations */
                                          void **workstation_list,
                                          double *computation_amount,
                                          double *communication_amount,
                                          double rate);
  double (*get_link_bandwidth) (const void *link);                                         /**< Return the current bandwidth of a network link */
  double (*get_link_latency) (const void *link);                                           /**< Return the current latency of a network link */
  surf_action_t(*open) (void *workstation, const char* storage,
                        const char* path);
  surf_action_t(*close) (void *workstation, surf_file_t fd);
  surf_action_t(*read) (void *workstation, void* ptr, size_t size,
                        surf_file_t fd);
  surf_action_t(*write) (void *workstation, const void* ptr, size_t size,
                         surf_file_t fd);
  surf_action_t(*stat) (void *workstation, surf_file_t fd);
  int(*unlink) (void *workstation, surf_file_t fd);
  surf_action_t(*ls) (void *workstation, const char* mount, const char *path);
  size_t (*get_size) (void *workstation, surf_file_t fd);

  int (*link_shared) (const void *link);
   xbt_dict_t(*get_properties) (const void *resource);
  void (*add_traces) (void);

} s_surf_model_extension_workstation_t;


static inline void *surf_cpu_resource_priv(const void *host) {
  return xbt_lib_get_level((xbt_dictelm_t)host, SURF_CPU_LEVEL);
}
static inline void *surf_workstation_resource_priv(const void *host){
  return (void*)xbt_lib_get_level((xbt_dictelm_t)host, SURF_WKS_LEVEL);
}
static inline void *surf_storage_resource_priv(const void *host){
  return (void*)xbt_lib_get_level((xbt_dictelm_t)host, SURF_STORAGE_LEVEL);
}

static inline void *surf_cpu_resource_by_name(const char *name) {
  return xbt_lib_get_elm_or_null(host_lib, name);
}
static inline void *surf_workstation_resource_by_name(const char *name){
  return xbt_lib_get_elm_or_null(host_lib, name);
}
static inline void *surf_storage_resource_by_name(const char *name){
  return xbt_lib_get_elm_or_null(storage_lib, name);
}

#ifdef __cplusplus
extern "C" {
#endif
char *surf_routing_edge_name(sg_routing_edge_t edge);

const char *surf_model_name(surf_model_t model);
xbt_swag_t surf_model_done_action_set(surf_model_t model);
xbt_swag_t surf_model_failed_action_set(surf_model_t model);
xbt_swag_t surf_model_ready_action_set(surf_model_t model);
xbt_swag_t surf_model_running_action_set(surf_model_t model);
surf_action_t surf_workstation_model_execute_parallel_task(surf_workstation_model_t model,
		                                    int workstation_nb,
                                            void **workstation_list,
                                            double *computation_amount,
                                            double *communication_amount,
                                            double rate);
surf_action_t surf_workstation_model_communicate(surf_workstation_model_t model, surf_workstation_CLM03_t src, surf_workstation_CLM03_t dst, double size, double rate);
xbt_dynar_t surf_workstation_model_get_route(surf_workstation_model_t model, surf_workstation_t src, surf_workstation_t dst);
surf_action_t surf_network_model_communicate(surf_network_model_t model, sg_routing_edge_t src, sg_routing_edge_t dst, double size, double rate);
const char *surf_resource_name(surf_resource_t resource);
xbt_dict_t surf_resource_get_properties(surf_resource_t resource);
e_surf_resource_state_t surf_resource_get_state(surf_resource_t resource);
double surf_workstation_get_speed(surf_workstation_t resource, double load);
double surf_workstation_get_available_speed(surf_workstation_t resource);
int surf_workstation_get_core(surf_workstation_t resource);
surf_action_t surf_workstation_execute(surf_workstation_t resource, double size);
surf_action_t surf_workstation_sleep(surf_workstation_t resource, double duration);
surf_action_t surf_workstation_communicate(surf_workstation_t workstation_src, surf_workstation_t workstation_dst, double size, double rate);
surf_action_t surf_workstation_open(surf_workstation_t workstation, const char* mount, const char* path);
surf_action_t surf_workstation_close(surf_workstation_t workstation, surf_file_t fd);
const char *surf_cpu_name(surf_cpu_t resource);
surf_action_t surf_cpu_execute(surf_cpu_t cpu, double size);
surf_action_t surf_cpu_sleep(surf_cpu_t cpu, double duration);
int surf_workstation_unlink(surf_workstation_t workstation, surf_file_t fd);
surf_action_t surf_workstation_ls(surf_workstation_t workstation, const char* mount, const char *path);
size_t surf_workstation_get_size(surf_workstation_t workstation, surf_file_t fd);
surf_action_t surf_workstation_read(surf_workstation_t resource, void *ptr, size_t size, surf_file_t fd);
surf_action_t surf_workstation_write(surf_workstation_t resource, const void *ptr, size_t size, surf_file_t fd);
int surf_network_link_is_shared(surf_network_link_t link);
double surf_network_link_get_bandwidth(surf_network_link_t link);
double surf_network_link_get_latency(surf_network_link_t link);
void *surf_action_get_data(surf_action_t action);
void surf_action_set_data(surf_action_t action, void *data);
void surf_action_unref(surf_action_t action);
double surf_action_get_start_time(surf_action_t action);
double surf_action_get_finish_time(surf_action_t action);
double surf_action_get_remains(surf_action_t action);
void surf_action_suspend(surf_action_t action);
void surf_action_resume(surf_action_t action);
void surf_action_cancel(surf_action_t action);
void surf_action_set_priority(surf_action_t action, double priority);
void surf_action_set_category(surf_action_t action, const char *category);
e_surf_action_state_t surf_action_get_state(surf_action_t action);
int surf_action_get_cost(surf_action_t action);
surf_file_t surf_storage_action_get_file(surf_storage_action_lmm_t action);
xbt_dict_t surf_storage_action_get_ls_dict(surf_storage_action_lmm_t action);

#ifdef __cplusplus
}
#endif

/**************************************/
/* Implementations of model object */
/**************************************/


/** \ingroup SURF_models
 *  \brief The CPU model
 */
XBT_PUBLIC_DATA(surf_cpu_model_t) surf_cpu_model;

/** \ingroup SURF_models
 *  \brief Initializes the CPU model with the model Cas01
 *
 *  By default, this model uses the lazy optimization mechanism that
 *  relies on partial invalidation in LMM and a heap for lazy action update.
 *  You can change this behavior by setting the cpu/optim configuration
 *  variable to a different value.
 *
 *  You shouldn't have to call it by yourself.
 */
XBT_PUBLIC(void) surf_cpu_model_init_Cas01(void);

/** \ingroup SURF_models
 *  \brief Initializes the CPU model with trace integration [Deprecated]
 *
 *  You shouldn't have to call it by yourself.
 */
XBT_PUBLIC(void) surf_cpu_model_init_ti(void);

/** \ingroup SURF_models
 *  \brief The list of all available optimization modes (both for cpu and networks).
 *  These optimization modes can be set using --cfg=cpu/optim:... and --cfg=network/optim:...
 */
XBT_PUBLIC_DATA(s_surf_model_description_t) surf_optimization_mode_description[];

/** \ingroup SURF_models
 *  \brief The list of all available cpu model models
 */
XBT_PUBLIC_DATA(s_surf_model_description_t) surf_cpu_model_description[];

/**\brief create new host bypass the parser
 *
 */


/** \ingroup SURF_models
 *  \brief The network model
 *
 *  When creating a new API on top on SURF, you shouldn't use the
 *  network model unless you know what you are doing. Only the workstation
 *  model should be accessed because depending on the platform model,
 *  the network model can be NULL.
 */
XBT_PUBLIC_DATA(surf_network_model_t) surf_network_model;

/** \ingroup SURF_models
 *  \brief Same as network model 'LagrangeVelho', only with different correction factors.
 *
 * This model is proposed by Pierre-Nicolas Clauss and Martin Quinson and Stéphane Génaud
 * based on the model 'LV08' and different correction factors depending on the communication
 * size (< 1KiB, < 64KiB, >= 64KiB).
 * See comments in the code for more information.
 *
 *  \see surf_workstation_model_init_SMPI()
 */
XBT_PUBLIC(void) surf_network_model_init_SMPI(void);

/** \ingroup SURF_models
 *  \brief Initializes the platform with the network model 'LegrandVelho'
 *
 * This model is proposed by Arnaud Legrand and Pedro Velho based on
 * the results obtained with the GTNets simulator for onelink and
 * dogbone sharing scenarios. See comments in the code for more information.
 *
 *  \see surf_workstation_model_init_LegrandVelho()
 */
XBT_PUBLIC(void) surf_network_model_init_LegrandVelho(void);

/** \ingroup SURF_models
 *  \brief Initializes the platform with the network model 'Constant'
 *
 *  In this model, the communication time between two network cards is
 *  constant, hence no need for a routing table. This is particularly
 *  usefull when simulating huge distributed algorithms where
 *  scalability is really an issue. This function is called in
 *  conjunction with surf_workstation_model_init_compound.
 *
 *  \see surf_workstation_model_init_compound()
 */
XBT_PUBLIC(void) surf_network_model_init_Constant(void);

/** \ingroup SURF_models
 *  \brief Initializes the platform with the network model CM02
 *
 *  You sould call this function by yourself only if you plan using
 *  surf_workstation_model_init_compound.
 *  See comments in the code for more information.
 */
XBT_PUBLIC(void) surf_network_model_init_CM02(void);

#ifdef HAVE_GTNETS
/** \ingroup SURF_models
 *  \brief Initializes the platform with the network model GTNETS
 *  \param filename XML platform file name
 *
 *  This function is called by surf_workstation_model_init_GTNETS
 *  or by yourself only if you plan using surf_workstation_model_init_compound
 *
 *  \see surf_workstation_model_init_GTNETS()
 */
XBT_PUBLIC(void) surf_network_model_init_GTNETS(void);
#endif

#ifdef HAVE_NS3
/** \ingroup SURF_models
 *  \brief Initializes the platform with the network model NS3
 *  \param filename XML platform file name
 *
 *  This function is called by surf_workstation_model_init_NS3
 *  or by yourself only if you plan using surf_workstation_model_init_compound
 *
 *  \see surf_workstation_model_init_NS3()
 */
XBT_PUBLIC(void) surf_network_model_init_NS3(void);
#endif

/** \ingroup SURF_models
 *  \brief Initializes the platform with the network model Reno
 *  \param filename XML platform file name
 *
 *  The problem is related to max( sum( arctan(C * Df * xi) ) ).
 *
 *  Reference:
 *  [LOW03] S. H. Low. A duality model of TCP and queue management algorithms.
 *  IEEE/ACM Transaction on Networking, 11(4):525-536, 2003.
 *
 *  Call this function only if you plan using surf_workstation_model_init_compound.
 *
 */
XBT_PUBLIC(void) surf_network_model_init_Reno(void);

/** \ingroup SURF_models
 *  \brief Initializes the platform with the network model Reno2
 *  \param filename XML platform file name
 *
 *  The problem is related to max( sum( arctan(C * Df * xi) ) ).
 *
 *  Reference:
 *  [LOW01] S. H. Low. A duality model of TCP and queue management algorithms.
 *  IEEE/ACM Transaction on Networking, 11(4):525-536, 2003.
 *
 *  Call this function only if you plan using surf_workstation_model_init_compound.
 *
 */
XBT_PUBLIC(void) surf_network_model_init_Reno2(void);

/** \ingroup SURF_models
 *  \brief Initializes the platform with the network model Vegas
 *  \param filename XML platform file name
 *
 *  This problem is related to max( sum( a * Df * ln(xi) ) ) which is equivalent
 *  to the proportional fairness.
 *
 *  Reference:
 *  [LOW03] S. H. Low. A duality model of TCP and queue management algorithms.
 *  IEEE/ACM Transaction on Networking, 11(4):525-536, 2003.
 *
 *  Call this function only if you plan using surf_workstation_model_init_compound.
 *
 */
XBT_PUBLIC(void) surf_network_model_init_Vegas(void);

/** \ingroup SURF_models
 *  \brief The list of all available network model models
 */
XBT_PUBLIC_DATA(s_surf_model_description_t)
    surf_network_model_description[];

/** \ingroup SURF_models
 *  \brief The storage model
 */
XBT_PUBLIC(void) surf_storage_model_init_default(void);

/** \ingroup SURF_models
 *  \brief The list of all available storage modes.
 *  This storage mode can be set using --cfg=storage/model:...
 */
XBT_PUBLIC_DATA(s_surf_model_description_t) surf_storage_model_description[];

/** \ingroup SURF_models
 *  \brief The workstation model
 *
 *  Note that when you create an API on top of SURF,
 *  the workstation model should be the only one you use
 *  because depending on the platform model, the network model and the CPU model
 *  may not exist.
 */
XBT_PUBLIC_DATA(surf_workstation_model_t) surf_workstation_model;

/** \ingroup SURF_models
 *  \brief Initializes the platform with a compound workstation model
 *
 *  This function should be called after a cpu_model and a
 *  network_model have been set up.
 *
 */
XBT_PUBLIC(void) surf_workstation_model_init_compound(void);

/** \ingroup SURF_models
 *  \brief Initializes the platform with the current best network and cpu models at hand
 *
 *  This platform model seperates the workstation model and the network model.
 *  The workstation model will be initialized with the model compound, the network
 *  model with the model LV08 (with cross traffic support) and the CPU model with
 *  the model Cas01.
 *  Such model is subject to modification with warning in the ChangeLog so monitor it!
 *
 */
XBT_PUBLIC(void) surf_workstation_model_init_current_default(void);

/** \ingroup SURF_models
 *  \brief Initializes the platform with the model KCCFLN05
 *
 *  With this model, only parallel tasks can be used. Resource sharing
 *  is done by identifying bottlenecks and giving an equal share of
 *  the model to each action.
 *
 */
XBT_PUBLIC(void) surf_workstation_model_init_ptask_L07(void);

/** \ingroup SURF_models
 *  \brief The list of all available workstation model models
 */
XBT_PUBLIC_DATA(s_surf_model_description_t)
    surf_workstation_model_description[];

/*******************************************
 *  TUTORIAL: New model
 */
XBT_PUBLIC(void) surf_new_model_init_default(void);

XBT_PUBLIC_DATA(s_surf_model_description_t) surf_new_model_description[];

/*******************************************/

/** \ingroup SURF_models
 *  \brief List of initialized models
 */
XBT_PUBLIC_DATA(xbt_dynar_t) model_list;

/*******************************************/
/*** SURF Platform *************************/
/*******************************************/
#ifdef __cplusplus
extern "C" {
#endif
XBT_PUBLIC_DATA(AS_t) surf_AS_get_routing_root(void); 
XBT_PUBLIC_DATA(const char *) surf_AS_get_name(AS_t as);
XBT_PUBLIC_DATA(xbt_dict_t) surf_AS_get_routing_sons(AS_t as);
XBT_PUBLIC_DATA(const char *) surf_AS_get_model(AS_t as);
XBT_PUBLIC_DATA(xbt_dynar_t) surf_AS_get_hosts(AS_t as);
XBT_PUBLIC_DATA(void) surf_AS_get_graph(AS_t as, xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges);
XBT_PUBLIC_DATA(AS_t) surf_platf_get_root(routing_platf_t platf);
XBT_PUBLIC_DATA(e_surf_network_element_type_t) surf_routing_edge_get_rc_type(sg_routing_edge_t edge);
#ifdef __cplusplus
}
#endif

/*******************************************/
/*** SURF Globals **************************/
/*******************************************/

/** \ingroup SURF_simulation
 *  \brief Initialize SURF
 *  \param argc argument number
 *  \param argv arguments
 *
 *  This function has to be called to initialize the common
 *  structures.  Then you will have to create the environment by
 *  calling 
 *  e.g. surf_workstation_model_init_CM02()
 *
 *  \see surf_workstation_model_init_CM02(), surf_workstation_model_init_compound(), surf_exit()
 */
XBT_PUBLIC(void) surf_init(int *argc, char **argv);     /* initialize common structures */

/** \ingroup SURF_simulation
 *  \brief Finish simulation initialization
 *
 *  This function must be called before the first call to surf_solve()
 */
XBT_PUBLIC(void) surf_presolve(void);

/** \ingroup SURF_simulation
 *  \brief Performs a part of the simulation
 *  \param max_date Maximum date to update the simulation to, or -1
 *  \return the elapsed time, or -1.0 if no event could be executed
 *
 *  This function execute all possible events, update the action states
 *  and returns the time elapsed.
 *  When you call execute or communicate on a model, the corresponding actions
 *  are not executed immediately but only when you call surf_solve.
 *  Note that the returned elapsed time can be zero.
 */
XBT_PUBLIC(double) surf_solve(double max_date);

/** \ingroup SURF_simulation
 *  \brief Return the current time
 *
 *  Return the current time in millisecond.
 */
XBT_PUBLIC(double) surf_get_clock(void);

/** \ingroup SURF_simulation
 *  \brief Exit SURF
 *
 *  Clean everything.
 *
 *  \see surf_init()
 */
XBT_PUBLIC(void) surf_exit(void);

/* Prototypes of the functions that handle the properties */
XBT_PUBLIC_DATA(xbt_dict_t) current_property_set;       /* the prop set for the currently parsed element (also used in SIMIX) */

/* surf parse file related (public because called from a test suite) */
XBT_PUBLIC(void) parse_platform_file(const char *file);

/* For the trace and trace:connect tag (store their content till the end of the parsing) */
XBT_PUBLIC_DATA(xbt_dict_t) traces_set_list;
XBT_PUBLIC_DATA(xbt_dict_t) trace_connect_list_host_avail;
XBT_PUBLIC_DATA(xbt_dict_t) trace_connect_list_power;
XBT_PUBLIC_DATA(xbt_dict_t) trace_connect_list_link_avail;
XBT_PUBLIC_DATA(xbt_dict_t) trace_connect_list_bandwidth;
XBT_PUBLIC_DATA(xbt_dict_t) trace_connect_list_latency;


XBT_PUBLIC(double) get_cpu_power(const char *power);

XBT_PUBLIC(xbt_dict_t) get_as_router_properties(const char* name);

int surf_get_nthreads(void);
void surf_set_nthreads(int nthreads);

void surf_watched_hosts(void);

/*
 * Returns the initial path. On Windows the initial path is
 * the current directory for the current process in the other
 * case the function returns "./" that represents the current
 * directory on Unix/Linux platforms.
 */
const char *__surf_get_initial_path(void);

/********** Tracing **********/
/* from surf_instr.c */
void TRACE_surf_action(surf_action_t surf_action, const char *category);
void TRACE_surf_alloc(void);
void TRACE_surf_release(void);

/* instr_routing.c */
void instr_routing_define_callbacks (void);
void instr_new_variable_type (const char *new_typename, const char *color);
void instr_new_user_variable_type  (const char *father_type, const char *new_typename, const char *color);
void instr_new_user_state_type (const char *father_type, const char *new_typename);
void instr_new_value_for_user_state_type (const char *_typename, const char *value, const char *color);
int instr_platform_traced (void);
xbt_graph_t instr_routing_platform_graph (void);
void instr_routing_platform_graph_export_graphviz (xbt_graph_t g, const char *filename);

SG_END_DECL()
#endif                          /* _SURF_SURF_H */
