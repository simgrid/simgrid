/* Copyright (c) 2004-2015. The SimGrid Team.
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
#include "xbt/config.h"
#include "src/internal_config.h"
#include "surf/surf_routing.h"
#include "surf/datatypes.h"
#include "xbt/lib.h"
#include "surf/surf_routing.h"
#include "simgrid/datatypes.h"
#include "simgrid/forward.h"

SG_BEGIN_DECL()
/* Actions and models are highly connected structures... */

/* user-visible parameters */
extern XBT_PRIVATE double sg_tcp_gamma;
extern XBT_PRIVATE double sg_sender_gap;
extern XBT_PRIVATE double sg_latency_factor;
extern XBT_PRIVATE double sg_bandwidth_factor;
extern XBT_PRIVATE double sg_weight_S_parameter;
extern XBT_PRIVATE int sg_network_crosstraffic;
extern XBT_PRIVATE xbt_dynar_t surf_path;

typedef enum {
  SURF_NETWORK_ELEMENT_HOST=1,     /* host type */
  SURF_NETWORK_ELEMENT_ROUTER,   /* router type */
  SURF_NETWORK_ELEMENT_AS        /* AS type */
} e_surf_network_element_type_t;

#ifdef __cplusplus

namespace simgrid {
namespace surf {

class Model;
class CpuModel;
class HostModel;
class VMModel;
class NetworkModel;
class StorageModel;
class Resource;
class ResourceLmm;
class HostImpl;
class HostCLM03;
class NetworkCm02Link;
class Action;
class ActionLmm;
class StorageActionLmm;
class RoutingPlatf;
}
}

typedef simgrid::surf::Model surf_Model;
typedef simgrid::surf::CpuModel surf_CpuModel;
typedef simgrid::surf::HostModel surf_HostModel;
typedef simgrid::surf::VMModel surf_VMModel;
typedef simgrid::surf::NetworkModel surf_NetworkModel;
typedef simgrid::surf::StorageModel surf_StorageModel;
typedef simgrid::surf::Resource surf_Resource;
typedef simgrid::surf::ResourceLmm surf_ResourceLmm;
typedef simgrid::surf::HostImpl surf_Host;
typedef simgrid::surf::HostCLM03 surf_HostCLM03;
typedef simgrid::surf::NetworkCm02Link surf_NetworkCm02Link;
typedef simgrid::surf::Action surf_Action;
typedef simgrid::surf::ActionLmm surf_ActionLmm;
typedef simgrid::surf::StorageActionLmm surf_StorageActionLmm;
typedef simgrid::surf::RoutingPlatf surf_RoutingPlatf;

#else

typedef struct surf_Model surf_Model;
typedef struct surf_CpuModel surf_CpuModel;
typedef struct surf_HostModel surf_HostModel;
typedef struct surf_VMModel surf_VMModel;
typedef struct surf_NetworkModel surf_NetworkModel;
typedef struct surf_StorageModel surf_StorageModel;
typedef struct surf_Resource surf_Resource;
typedef struct surf_ResourceLmm surf_ResourceLmm;
typedef struct surf_HostCLM03 surf_HostCLM03;
typedef struct surf_Host surf_Host;
typedef struct surf_NetworkCm02Link surf_NetworkCm02Link;
typedef struct surf_Action surf_Action;
typedef struct surf_ActionLmm surf_ActionLmm;
typedef struct surf_StorageActionLmm surf_StorageActionLmm;
typedef struct surf_RoutingPlatf surf_RoutingPlatf;

#endif

/** @ingroup SURF_c_bindings
 *  \brief Model datatype
 *
 *  Generic data structure for a model. The hosts,
 *  the CPUs and the network links are examples of models.
 */
typedef surf_Model *surf_model_t;
typedef surf_CpuModel *surf_cpu_model_t;
typedef surf_HostModel *surf_host_model_t;
typedef surf_VMModel *surf_vm_model_t;
typedef surf_NetworkModel *surf_network_model_t;
typedef surf_StorageModel *surf_storage_model_t;

typedef xbt_dictelm_t surf_resource_t;
typedef surf_Resource *surf_cpp_resource_t;
typedef surf_Host *surf_host_t;
typedef surf_Cpu *surf_cpu_t;

/** @ingroup SURF_c_bindings
 *  \brief Action structure
 *
 *  Never create s_surf_action_t by yourself ! The actions are created
 *  on the fly when you call execute or communicate on a model.
 *
 *  \see e_surf_action_state_t
 */
typedef surf_Action *surf_action_t;
typedef surf_RoutingPlatf *routing_platf_t;

typedef struct surf_file *surf_file_t;

/** \brief Resource model description
 */
typedef struct surf_model_description {
  const char *name;
  const char *description;
  void_f_void_t model_init_preparse;
} s_surf_model_description_t, *surf_model_description_t;

XBT_PUBLIC(int) find_model_description(s_surf_model_description_t * table, const char *name);
XBT_PUBLIC(void) model_help(const char *category, s_surf_model_description_t * table);

/** @ingroup SURF_interface
 *  @brief Action states
 *
 *  @see Action
 */
typedef enum {
  SURF_ACTION_READY = 0,        /**< Ready        */
  SURF_ACTION_RUNNING,          /**< Running      */
  SURF_ACTION_FAILED,           /**< Task Failure */
  SURF_ACTION_DONE,             /**< Completed    */
  SURF_ACTION_TO_FREE,          /**< Action to free in next cleanup */
  SURF_ACTION_NOT_IN_THE_SYSTEM /**< Not in the system anymore. Why did you ask ? */
} e_surf_action_state_t;

/** @ingroup SURF_vm_interface
 *
 *
 */
/* FIXME: Where should the VM state be defined? */
typedef enum {
  SURF_VM_STATE_CREATED, /**< created, but not yet started */
  SURF_VM_STATE_RUNNING,
  SURF_VM_STATE_SUSPENDED, /**< Suspend/resume does not involve disk I/O, so we assume there is no transition states. */

  SURF_VM_STATE_SAVING, /**< Save/restore involves disk I/O, so there should be transition states. */
  SURF_VM_STATE_SAVED,
  SURF_VM_STATE_RESTORING,
} e_surf_vm_state_t;

/***************************/
/* Generic model object */
/***************************/

XBT_PUBLIC_DATA(routing_platf_t) routing_platf;

static inline void *surf_storage_resource_priv(const void *storage){
  return (void*)xbt_lib_get_level((xbt_dictelm_t)storage, SURF_STORAGE_LEVEL);
}

static inline void *surf_storage_resource_by_name(const char *name){
  return xbt_lib_get_elm_or_null(storage_lib, name);
}

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

/** @brief Create a new VM on the specified host */
XBT_PUBLIC(void) surf_vm_model_create(const char *name, sg_host_t host_PM);

/** @brief Create a communication between two hosts
 *
 * @param model The model which handle the communication
 * @param src The source host
 * @param dst The destination host
 * @param size The amount of data (in bytes) needed to transfer
 * @param rate [description]
 * @return The action corresponding to the communication
 */
XBT_PUBLIC(surf_action_t) surf_network_model_communicate(surf_network_model_t model, sg_host_t src, sg_host_t dst, double size, double rate);

/**
 * @brief Get the name of a surf resource (cpu, host, network, …)
 *
 * @param resource The surf resource
 * @return The name of the surf resource
 */
XBT_PUBLIC(const char * ) surf_resource_name(surf_cpp_resource_t resource);

/** @brief Get the available speed of cpu associated to a host */
XBT_PUBLIC(double) surf_host_get_available_speed(sg_host_t host);

/** @brief Create a sleep action on the given host */
XBT_PUBLIC(surf_action_t) surf_host_sleep(sg_host_t host, double duration);

/** @brief Create a file opening action on the given host */
XBT_PUBLIC(surf_action_t) surf_host_open(sg_host_t host, const char* fullpath);

/** @brief Create a file closing action on the given host */
XBT_PUBLIC(surf_action_t) surf_host_close(sg_host_t host, surf_file_t fd);

/** @brief Create a file reading action on the given host */
XBT_PUBLIC(surf_action_t) surf_host_read(sg_host_t host, surf_file_t fd, sg_size_t size);

/** @brief Create a file writing action on the given host  */
XBT_PUBLIC(surf_action_t) surf_host_write(sg_host_t host, surf_file_t fd, sg_size_t size);

/**
 * @brief Get the informations of a file descriptor
 * @details The returned xbt_dynar_t contains:
 *  - the size of the file,
 *  - the mount point,
 *  - the storage name,
 *  - the storage typeId,
 *  - the storage content type
 *
 * @param host The surf host
 * @param fd The file descriptor
 * @return An xbt_dynar_t with the file informations
 */
XBT_PUBLIC(xbt_dynar_t) surf_host_get_info(sg_host_t host, surf_file_t fd);

/**
 * @brief Get the available space of the storage at the mount point
 *
 * @param resource The surf host
 * @param name The mount point
 * @return The amount of available space in bytes
 */
XBT_PUBLIC(sg_size_t) surf_host_get_free_size(sg_host_t resource, const char* name);

/**
 * @brief Get the used space of the storage at the mount point
 *
 * @param resource The surf host
 * @param name The mount point
 * @return The amount of used space in bytes
 */
XBT_PUBLIC(sg_size_t) surf_host_get_used_size(sg_host_t resource, const char* name);

/**
 * @brief Destroy a VM
 *
 * @param resource The surf vm
 */
XBT_PUBLIC(void) surf_vm_destroy(sg_host_t resource);

/** @brief Suspend a VM */
XBT_PUBLIC(void) surf_vm_suspend(sg_host_t resource);

/** @brief Resume a VM */
XBT_PUBLIC(void) surf_vm_resume(sg_host_t resource);

/**
 * @brief Save the VM (Not yet implemented)
 *
 * @param resource The surf vm
 */
XBT_PUBLIC(void) surf_vm_save(sg_host_t resource);

/**
 * @brief Restore the VM (Not yet implemented)
 *
 * @param resource The surf vm
 */
XBT_PUBLIC(void) surf_vm_restore(sg_host_t resource);

/**
 * @brief Migrate the VM to the destination host
 *
 * @param resource The surf vm
 * @param ind_vm_ws_dest The destination host
 */
XBT_PUBLIC(void) surf_vm_migrate(sg_host_t resource, sg_host_t ind_vm_ws_dest);

/**
 * @brief Get the physical machine hosting the VM
 *
 * @param resource The surf vm
 * @return The physical machine hosting the VM
 */
XBT_PUBLIC(sg_host_t) surf_vm_get_pm(sg_host_t resource);

/**
 * @brief [brief description]
 * @details [long description]
 *
 * @param resource [description]
 * @param bound [description]
 */
XBT_PUBLIC(void) surf_vm_set_bound(sg_host_t resource, double bound);

/**
 * @brief [brief description]
 * @details [long description]
 *
 * @param resource [description]
 * @param cpu [description]
 * @param mask [description]
 */
XBT_PUBLIC(void) surf_vm_set_affinity(sg_host_t resource, sg_host_t cpu, unsigned long mask);

/**
 * @brief Unlink a file descriptor
 *
 * @param host The surf host
 * @param fd The file descriptor
 *
 * @return 0 if failed to unlink, 1 otherwise
 */
XBT_PUBLIC(int) surf_host_unlink(sg_host_t host, surf_file_t fd);

/**
 * @brief Get the size of a file on a host
 *
 * @param host The surf host
 * @param fd The file descriptor
 *
 * @return The size in bytes of the file
 */
XBT_PUBLIC(size_t) surf_host_get_size(sg_host_t host, surf_file_t fd);

/**
 * @brief Get the current position of the file descriptor
 *
 * @param host The surf host
 * @param fd The file descriptor
 * @return The current position of the file descriptor
 */
XBT_PUBLIC(size_t) surf_host_file_tell(sg_host_t host, surf_file_t fd);

/**
 * @brief Move a file to another location on the *same mount point*.
 * @details [long description]
 *
 * @param host The surf host
 * @param fd The file descriptor
 * @param fullpath The new full path
 *
 * @return MSG_OK if successful, otherwise MSG_TASK_CANCELED
 */
XBT_PUBLIC(int) surf_host_file_move(sg_host_t host, surf_file_t fd, const char* fullpath);

/**
 * @brief Set the position indictator assiociated with the file descriptor to a new position
 * @details [long description]
 *
 * @param host The surf host
 * @param fd The file descriptor
 * @param offset The offset from the origin
 * @param origin Position used as a reference for the offset
 *  - SEEK_SET: beginning of the file
 *  - SEEK_CUR: current position indicator
 *  - SEEK_END: end of the file
 * @return MSG_OK if successful, otherwise MSG_TASK_CANCELED
 */
XBT_PUBLIC(int) surf_host_file_seek(sg_host_t host,
                                           surf_file_t fd, sg_offset_t offset,
                                           int origin);

/**
 * @brief Get the content of a storage
 *
 * @param resource The surf storage
 * @return A xbt_dict_t with path as keys and size in bytes as values
 */
XBT_PUBLIC(xbt_dict_t) surf_storage_get_content(surf_resource_t resource);

/**
 * @brief Get the size in bytes of a storage
 *
 * @param resource The surf storage
 * @return The size in bytes of the storage
 */
XBT_PUBLIC(sg_size_t) surf_storage_get_size(surf_resource_t resource);

/**
 * @brief Get the available size in bytes of a storage
 *
 * @param resource The surf storage
 * @return The available size in bytes of the storage
 */
XBT_PUBLIC(sg_size_t) surf_storage_get_free_size(surf_resource_t resource);

/**
 * @brief Get the size in bytes of a storage
 *
 * @param resource The surf storage
 * @return The used size in bytes of the storage
 */
XBT_PUBLIC(sg_size_t) surf_storage_get_used_size(surf_resource_t resource);

/** @brief return the properties set associated to that storage */
XBT_PUBLIC(xbt_dict_t) surf_storage_get_properties(surf_resource_t resource);

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

/**
 * @brief Get the file associated to a storage action
 *
 * @param action The surf storage action
 * @return The file associated to a storage action
 */
XBT_PUBLIC(surf_file_t) surf_storage_action_get_file(surf_action_t action);

/**
 * @brief Get the result dictionary of an ls action
 *
 * @param action The surf storage action
 * @return The dictionry listing a path
 */
XBT_PUBLIC(xbt_dict_t) surf_storage_action_get_ls_dict(surf_action_t action);


/**
 * @brief Get the host the storage is attached to
 *
 * @param resource The surf storage
 * @return The host name
 * may not exist.
 */
XBT_PUBLIC(const char * ) surf_storage_get_host(surf_resource_t resource);

/** @} */

/**************************************/
/* Implementations of model object */
/**************************************/

XBT_PUBLIC_DATA(int) autoload_surf_cpu_model;
XBT_PUBLIC_DATA(void_f_void_t) surf_cpu_model_init_preparse;

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
XBT_PUBLIC(void) surf_network_model_init_SMPI(void);

/** \ingroup SURF_models
 *  \brief Same as network model 'LagrangeVelho', only with different correction factors.
 *
 * This model impelments a variant of the contention model on Infinband networks based on
 * the works of Jérôme Vienne : http://mescal.imag.fr/membres/jean-marc.vincent/index.html/PhD/Vienne.pdf
 *
 *  \see surf_host_model_init_IB()
 */
XBT_PUBLIC(void) surf_network_model_init_IB(void);

/** \ingroup SURF_models
 *  \brief Initializes the platform with the network model 'LegrandVelho'
 *
 * This model is proposed by Arnaud Legrand and Pedro Velho based on the results obtained with the GTNets simulator for
 * onelink and dogbone sharing scenarios. See comments in the code for more information.
 *
 *  \see surf_host_model_init_LegrandVelho()
 */
XBT_PUBLIC(void) surf_network_model_init_LegrandVelho(void);

/** \ingroup SURF_models
 *  \brief Initializes the platform with the network model 'Constant'
 *
 *  In this model, the communication time between two network cards is constant, hence no need for a routing table.
 *  This is particularly useful when simulating huge distributed algorithms where scalability is really an issue. This
 *  function is called in conjunction with surf_host_model_init_compound.
 *
 *  \see surf_host_model_init_compound()
 */
XBT_PUBLIC(void) surf_network_model_init_Constant(void);

/** \ingroup SURF_models
 *  \brief Initializes the platform with the network model CM02
 *
 *  You sould call this function by yourself only if you plan using surf_host_model_init_compound.
 *  See comments in the code for more information.
 */
XBT_PUBLIC(void) surf_network_model_init_CM02(void);

#if HAVE_NS3
/** \ingroup SURF_models
 *  \brief Initializes the platform with the network model NS3
 *
 *  This function is called by surf_host_model_init_NS3 or by yourself only if you plan using
 *  surf_host_model_init_compound
 *
 *  \see surf_host_model_init_NS3()
 */
XBT_PUBLIC(void) surf_network_model_init_NS3(void);
#endif

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
XBT_PUBLIC(void) surf_network_model_init_Reno(void);

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
XBT_PUBLIC(void) surf_network_model_init_Reno2(void);

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
XBT_PUBLIC(void) surf_network_model_init_Vegas(void);

/** \ingroup SURF_models
 *  \brief The list of all available network model models
 */
XBT_PUBLIC_DATA(s_surf_model_description_t) surf_network_model_description[];

/** \ingroup SURF_models
 *  \brief The storage model
 */
XBT_PUBLIC(void) surf_storage_model_init_default(void);

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
 *  \brief The vm model
 *
 *  Note that when you create an API on top of SURF,the vm model should be the only one you use
 *  because depending on the platform model, the network model and the CPU model may not exist.
 */
XBT_PUBLIC_DATA(surf_vm_model_t) surf_vm_model;

/** \ingroup SURF_models
 *  \brief Initializes the platform with a compound host model
 *
 *  This function should be called after a cpu_model and a network_model have been set up.
 */
XBT_PUBLIC(void) surf_host_model_init_compound(void);

/** \ingroup SURF_models
 *  \brief Initializes the platform with the current best network and cpu models at hand
 *
 *  This platform model separates the host model and the network model.
 *  The host model will be initialized with the model compound, the network model with the model LV08 (with cross
 *  traffic support) and the CPU model with the model Cas01.
 *  Such model is subject to modification with warning in the ChangeLog so monitor it!
 */
XBT_PUBLIC(void) surf_host_model_init_current_default(void);

/** \ingroup SURF_models
 *  \brief Initializes the platform with the model L07
 *
 *  With this model, only parallel tasks can be used. Resource sharing is done by identifying bottlenecks and giving an
 *  equal share of the model to each action.
 */
XBT_PUBLIC(void) surf_host_model_init_ptask_L07(void);

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
XBT_PUBLIC(void) surf_vm_model_init_HL13(void);

/** \ingroup SURF_models
 *  \brief The list of all available vm model models
 */
XBT_PUBLIC_DATA(s_surf_model_description_t) surf_vm_model_description[];

/** \ingroup SURF_models
 *  \brief List of initialized models
 */
XBT_PUBLIC_DATA(xbt_dynar_t) all_existing_models;

/** \ingroup SURF_simulation
 *  \brief List of hosts that have just restarted and whose autorestart process should be restarted.
 */
XBT_PUBLIC_DATA(xbt_dynar_t) host_that_restart;

/** \ingroup SURF_simulation
 *  \brief List of hosts for which one want to be notified if they ever restart.
 */
XBT_PUBLIC_DATA(xbt_dict_t) watched_hosts_lib;

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
XBT_PUBLIC_DATA(xbt_dict_t) current_property_set;// the prop set for the currently parsed element (also used in SIMIX)
/* The same for model_prop set*/
XBT_PUBLIC_DATA(xbt_dict_t) current_model_property_set;

/* surf parse file related (public because called from a test suite) */
XBT_PUBLIC(void) parse_platform_file(const char *file);

/* For the trace and trace:connect tag (store their content till the end of the parsing) */
XBT_PUBLIC_DATA(xbt_dict_t) traces_set_list;

XBT_PUBLIC(xbt_dict_t) get_as_router_properties(const char* name);

/*
 * Returns the initial path. On Windows the initial path is the current directory for the current process in the other
 * case the function returns "./" that represents the current directory on Unix/Linux platforms.
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
