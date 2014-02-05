/* Copyright (c) 2004-2014. The SimGrid Team.
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
#include "simgrid/datatypes.h"
#include "simgrid/plugins.h"

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
class WorkstationVMModel;
class NetworkModel;
class StorageModel;
class Resource;
class ResourceLmm;
class WorkstationCLM03;
class NetworkCm02Link;
class Cpu;
class Action;
class ActionLmm;
class StorageActionLmm;
struct As;
struct RoutingEdge;
class RoutingPlatf;
#else
typedef struct Model Model;
typedef struct CpuModel CpuModel;
typedef struct WorkstationModel WorkstationModel;
typedef struct WorkstationVMModel WorkstationVMModel;
typedef struct NetworkModel NetworkModel;
typedef struct StorageModel StorageModel;
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
typedef WorkstationVMModel *surf_vm_workstation_model_t;

typedef NetworkModel *surf_network_model_t;
typedef StorageModel *surf_storage_model_t;

typedef xbt_dictelm_t surf_resource_t;
typedef Resource *surf_cpp_resource_t;
typedef WorkstationCLM03 *surf_workstation_CLM03_t;
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

/** @ingroup SURF_interface
 *  @brief Action states
 *
 *  Action states.
 *
 *  @see Action
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

/** @ingroup SURF_vm_interface
 * 
 * 
 */
/* FIXME: Where should the VM state be defined? */
typedef enum {
  SURF_VM_STATE_CREATED, /**< created, but not yet started */

  SURF_VM_STATE_RUNNING,
  SURF_VM_STATE_MIGRATING,

  SURF_VM_STATE_SUSPENDED, /**< Suspend/resume does not involve disk I/O, so we assume there is no transition states. */

  SURF_VM_STATE_SAVING, /**< Save/restore involves disk I/O, so there should be transition states. */
  SURF_VM_STATE_SAVED,
  SURF_VM_STATE_RESTORING,

} e_surf_vm_state_t;

/***************************/
/* Generic model object */
/***************************/
//FIXME:REMOVE typedef struct s_routing_platf s_routing_platf_t, *routing_platf_t;
XBT_PUBLIC_DATA(routing_platf_t) routing_platf;

static inline void *surf_cpu_resource_priv(const void *host) {
  return xbt_lib_get_level((xbt_dictelm_t)host, SURF_CPU_LEVEL);
}
static inline void *surf_workstation_resource_priv(const void *host){
  return (void*)xbt_lib_get_level((xbt_dictelm_t)host, SURF_WKS_LEVEL);
}
static inline void *surf_storage_resource_priv(const void *storage){
  return (void*)xbt_lib_get_level((xbt_dictelm_t)storage, SURF_STORAGE_LEVEL);
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


XBT_PUBLIC(char *) surf_routing_edge_name(sg_routing_edge_t edge);
XBT_PUBLIC(void *) surf_as_cluster_get_backbone(AS_t as);
XBT_PUBLIC(void) surf_as_cluster_set_backbone(AS_t as, void* backbone);

/** @{ @ingroup SURF_c_bindings */

/** 
 * @brief Get the name of a surf model
 * 
 * @param model A model
 * @return The name of the model
 */
XBT_PUBLIC(const char *) surf_model_name(surf_model_t model);

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
 * @brief Pop an action from the ready actions set
 * 
 * @param model The model from which the action is extracted
 * @return An action in ready state
 */
XBT_PUBLIC(surf_action_t) surf_model_extract_ready_action_set(surf_model_t model);

/**
 * @brief Pop an action from the running actions set
 * 
 * @param model The model from which the action is extracted
 * @return An action in running state
 */
XBT_PUBLIC(surf_action_t) surf_model_extract_running_action_set(surf_model_t model);

/**
 * @brief Get the size of the running action set of a model
 * 
 * @param model The model
 * @return The size of the running action set
 */
XBT_PUBLIC(int) surf_model_running_action_set_size(surf_model_t model);

/**
 * @brief Execute a parallel task
 * @details [long description]
 * 
 * @param model The model which handle the parallelisation
 * @param workstation_nb The number of workstations 
 * @param workstation_list The list of workstations on which the task is executed
 * @param computation_amount The processing amount (in flop) needed to process
 * @param communication_amount The amount of data (in bytes) needed to transfer
 * @param rate [description]
 * @return The action corresponding to the parallele execution task
 */
XBT_PUBLIC(surf_action_t) surf_workstation_model_execute_parallel_task(surf_workstation_model_t model,
		                                    int workstation_nb,
                                            void **workstation_list,
                                            double *computation_amount,
                                            double *communication_amount,
                                            double rate);

/**
 * @brief Create a communication between two hosts
 * 
 * @param model The model which handle the communication
 * @param src The source host
 * @param dst The destination host
 * @param size The amount of data (in bytes) needed to transfer
 * @param rate [description]
 * @return The action corresponding to the communication
 */
XBT_PUBLIC(surf_action_t) surf_workstation_model_communicate(surf_workstation_model_t model, surf_resource_t src, surf_resource_t dst, double size, double rate);

/**
 * @brief Get the route between two hosts
 * @details [long description]
 * 
 * @param model The model which handle the routes
 * @param src The source host
 * @param dst The destination host
 * @return The list of [TODO] from the source to the host
 */
XBT_PUBLIC(xbt_dynar_t) surf_workstation_model_get_route(surf_workstation_model_t model, surf_resource_t src, surf_resource_t dst);

/**
 * @brief Create a new VM on the specified host
 * 
 * @param name The name of the workstation
 * @param ind_phys_host The host on which the VM is created
 */
XBT_PUBLIC(void) surf_vm_workstation_model_create(const char *name, surf_resource_t ind_phys_host);

/**
 * @brief Create a communication between two routing edges [TODO]
 * @details [long description]
 * 
 * @param model The model which handle the communication
 * @param src The source host
 * @param dst The destination host
 * @param size The amount of data (in bytes) needed to transfer
 * @param rate [description]
 * @return The action corresponding to the communication
 */
XBT_PUBLIC(surf_action_t) surf_network_model_communicate(surf_network_model_t model, sg_routing_edge_t src, sg_routing_edge_t dst, double size, double rate);

/**
 * @brief Get the name of a surf resource (cpu, workstation, network, …)
 * 
 * @param resource The surf resource
 * @return The name of the surf resource
 */
XBT_PUBLIC(const char * ) surf_resource_name(surf_cpp_resource_t resource);

/**
 * @brief Get the properties of a surf resource (cpu, workstation, network, …)
 * 
 * @param resource The surf resource
 * @return The properties of the surf resource
 */
XBT_PUBLIC(xbt_dict_t) surf_resource_get_properties(surf_cpp_resource_t resource);

/**
 * @brief Get the state of a surf resource (cpu, workstation, network, …)
 * 
 * @param resource The surf resource
 * @return The state of the surf resource
 */
XBT_PUBLIC(e_surf_resource_state_t) surf_resource_get_state(surf_cpp_resource_t resource);

/**
 * @brief Set the state of a surf resource (cpu, workstation, network, …)
 * 
 * @param resource The surf resource
 * @param state The new state of the surf resource
 */
XBT_PUBLIC(void) surf_resource_set_state(surf_cpp_resource_t resource, e_surf_resource_state_t state);

/**
 * @brief Get the speed of the cpu associtated to a workstation
 * 
 * @param resource The surf workstation
 * @param load [description]
 * 
 * @return [description]
 */
XBT_PUBLIC(double) surf_workstation_get_speed(surf_resource_t resource, double load);

/**
 * @brief Get the available speed of cpu associtated to a workstation
 * 
 * @param resource The surf workstation
 * @return [description]
 */
XBT_PUBLIC(double) surf_workstation_get_available_speed(surf_resource_t resource);

/**
 * @brief Get the number of cores of the cpu associated to a workstation
 * 
 * @param resource The surf workstation
 * @return The number of cores
 */
XBT_PUBLIC(int) surf_workstation_get_core(surf_resource_t resource);

/**
 * @brief Execute some quantity of computation
 *
 * @param resource The surf workstation
 * @param size The value of the processing amount (in flop) needed to process
 * 
 * @return The surf action corresponding to the processing
 */
XBT_PUBLIC(surf_action_t) surf_workstation_execute(surf_resource_t resource, double size);

/**
 * @brief Make the workstation sleep
 * 
 * @param resource The surf workstation
 * @param duration The number of seconds to sleep
 * @return The surf action corresponding to the sleep
 */
XBT_PUBLIC(surf_action_t) surf_workstation_sleep(surf_resource_t resource, double duration);

/**
 * @brief Open a file on a workstation
 * 
 * @param workstation The surf workstation
 * @param mount The mount point
 * @param path The path to the file
 * @return The surf action corresponding to the openning
 */
XBT_PUBLIC(surf_action_t) surf_workstation_open(surf_resource_t workstation, const char* mount, const char* path);

/**
 * @brief Close a file descriptor on a workstation
 * 
 * @param workstation The surf workstation
 * @param fd The file descriptor
 * 
 * @return The surf action corresponding to the closing
 */
XBT_PUBLIC(surf_action_t) surf_workstation_close(surf_resource_t workstation, surf_file_t fd);

/**
 * @brief Read a file
 * 
 * @param resource The surf workstation
 * @param fd The file descriptor to read
 * @param size The size in bytes to read
 * @return The surf action corresponding to the reading
 */
XBT_PUBLIC(surf_action_t) surf_workstation_read(surf_resource_t resource, surf_file_t fd, sg_size_t size);

/**
 * @brief Write a file
 * 
 * @param resource The surf workstation
 * @param fd The file descriptor to write
 * @param size The size in bytes to write
 * @return The surf action corresponding to the writing
 */
XBT_PUBLIC(surf_action_t) surf_workstation_write(surf_resource_t resource, surf_file_t fd, sg_size_t size);

/**
 * @brief Get the informations of a file descriptor
 * @details The returned xbt_dynar_t contains:
 *  - the size of the file,
 *  - the mount point,
 *  - the storage name,
 *  - the storage typeId,
 *  - the storage content type
 * 
 * @param resource The surf workstation
 * @param fd The file descriptor
 * @return An xbt_dynar_t with the file informations
 */
XBT_PUBLIC(xbt_dynar_t) surf_workstation_get_info(surf_resource_t resource, surf_file_t fd);

/**
 * @brief Get the available space of the storage at the mount point
 * 
 * @param resource The surf workstation
 * @param name The mount point
 * @return The amount of availble space in bytes
 */
XBT_PUBLIC(sg_size_t) surf_workstation_get_free_size(surf_resource_t resource, const char* name);

/**
 * @brief Get the used space of the storage at the mount point
 * 
 * @param resource The surf workstation
 * @param name The mount point
 * @return The amount of used space in bytes
 */
XBT_PUBLIC(sg_size_t) surf_workstation_get_used_size(surf_resource_t resource, const char* name);

/**
 * @brief Get the VMs hosted on the workstation
 * 
 * @param resource The surf workstation
 * @return The list of VMs on the workstation
 */
XBT_PUBLIC(xbt_dynar_t) surf_workstation_get_vms(surf_resource_t resource);

/**
 * @brief [brief description]
 * @details [long description]
 * 
 * @param resource [description]
 * @param params [description]
 */
XBT_PUBLIC(void) surf_workstation_get_params(surf_resource_t resource, ws_params_t params);

/**
 * @brief [brief description]
 * @details [long description]
 * 
 * @param resource [description]
 * @param params [description]
 */
XBT_PUBLIC(void) surf_workstation_set_params(surf_resource_t resource, ws_params_t params);

/**
 * @brief Destroy a Workstation VM
 * 
 * @param resource The surf workstation vm
 */
XBT_PUBLIC(void) surf_vm_workstation_destroy(surf_resource_t resource);

/**
 * @brief Suspend a Workstation VM
 * 
 * @param resource The surf workstation vm
 */
XBT_PUBLIC(void) surf_vm_workstation_suspend(surf_resource_t resource);

/**
 * @brief Resume a Workstation VM
 * 
 * @param resource The surf workstation vm
 */
XBT_PUBLIC(void) surf_vm_workstation_resume(surf_resource_t resource);

/**
 * @brief Save the Workstation VM (Not yet implemented)
 * 
 * @param resource The surf workstation vm
 */
XBT_PUBLIC(void) surf_vm_workstation_save(surf_resource_t resource);

/**
 * @brief Restore the Workstation VM (Not yet implemented)
 * 
 * @param resource The surf workstation vm
 */
XBT_PUBLIC(void) surf_vm_workstation_restore(surf_resource_t resource);

/**
 * @brief Migrate the VM to the destination host
 * 
 * @param resource The surf workstation vm
 * @param ind_vm_ws_dest The destination host
 */
XBT_PUBLIC(void) surf_vm_workstation_migrate(surf_resource_t resource, surf_resource_t ind_vm_ws_dest);

/**
 * @brief Get the physical machine hosting the VM
 * 
 * @param resource The surf workstation vm
 * @return The physical machine hosting the VM
 */
XBT_PUBLIC(surf_resource_t) surf_vm_workstation_get_pm(surf_resource_t resource);

/**
 * @brief [brief description]
 * @details [long description]
 * 
 * @param resource [description]
 * @param bound [description]
 */
XBT_PUBLIC(void) surf_vm_workstation_set_bound(surf_resource_t resource, double bound);

/**
 * @brief [brief description]
 * @details [long description]
 * 
 * @param resource [description]
 * @param cpu [description]
 * @param mask [description]
 */
XBT_PUBLIC(void) surf_vm_workstation_set_affinity(surf_resource_t resource, surf_resource_t cpu, unsigned long mask);

/**
 * @brief Execute some quantity of computation
 * 
 * @param cpu The surf cpu
 * @param size The value of the processing amount (in flop) needed to process
 * @return The surf action corresponding to the processing
 */
XBT_PUBLIC(surf_action_t) surf_cpu_execute(surf_resource_t cpu, double size);

/**
 * @brief Make the cpu sleep for duration (in seconds)
 * @details [long description]
 * 
 * @param cpu The surf cpu
 * @param duration The number of seconds to sleep
 * @return The surf action corresponding to the sleeping
 */
XBT_PUBLIC(surf_action_t) surf_cpu_sleep(surf_resource_t cpu, double duration);

/**
 * @brief Get the workstation power peak
 * @details [long description]
 * 
 * @param host The surf workstation
 * @return The power peak
 */
XBT_PUBLIC(double) surf_workstation_get_current_power_peak(surf_resource_t host);

/**
 * @brief [brief description]
 * @details [long description]
 * 
 * @param host [description]
 * @param pstate_index [description]
 * 
 * @return [description]
 */
XBT_PUBLIC(double) surf_workstation_get_power_peak_at(surf_resource_t host, int pstate_index);

/**
 * @brief [brief description]
 * @details [long description]
 * 
 * @param host [description]
 * @return [description]
 */
XBT_PUBLIC(int) surf_workstation_get_nb_pstates(surf_resource_t host);

/**
 * @brief [brief description]
 * @details [long description]
 * 
 * @param host [description]
 * @param pstate_index [description]
 */
XBT_PUBLIC(void) surf_workstation_set_power_peak_at(surf_resource_t host, int pstate_index);

/**
 * @brief Get the consumed energy (in joules) of a workstation
 * 
 * @param host The surf workstation
 * @return The consumed energy
 */
XBT_PUBLIC(double) surf_workstation_get_consumed_energy(surf_resource_t host);

/**
 * @brief Get the list of storages of a workstation
 * 
 * @param workstation The surf workstation
 * @return Dictionary of mount point, Storage
 */
XBT_PUBLIC(xbt_dict_t) surf_workstation_get_storage_list(surf_resource_t workstation);

/**
 * @brief Unlink a file descriptor
 * 
 * @param workstation The surf workstation
 * @param fd The file descriptor
 * 
 * @return 0 if failed to unlink, 1 otherwise
 */
XBT_PUBLIC(int) surf_workstation_unlink(surf_resource_t workstation, surf_file_t fd);

/**
 * @brief List directory contents of a path
 * @details [long description]
 * 
 * @param workstation The surf workstation
 * @param mount The mount point
 * @param path The path to the directory
 * @return The surf action corresponding to the ls action
 */
XBT_PUBLIC(surf_action_t) surf_workstation_ls(surf_resource_t workstation, const char* mount, const char *path);

/**
 * @brief Get the size of a file on a workstation
 * 
 * @param workstation The surf workstation
 * @param fd The file descriptor
 * 
 * @return The size in bytes of the file
 */
XBT_PUBLIC(size_t) surf_workstation_get_size(surf_resource_t workstation, surf_file_t fd);

/**
 * @brief Get the current position of the file descriptor
 * 
 * @param workstation The surf workstation
 * @param fd The file descriptor
 * @return The current position of the file descriptor
 */
XBT_PUBLIC(size_t) surf_workstation_file_tell(surf_resource_t workstation, surf_file_t fd);

/**
 * @brief Set the position indictator assiociated with the file descriptor to a new position
 * @details [long description]
 * 
 * @param workstation The surf workstation
 * @param fd The file descriptor
 * @param offset The offset from the origin
 * @param origin Position used as a reference for the offset
 *  - SEEK_SET: beginning of the file
 *  - SEEK_CUR: current position indicator
 *  - SEEK_END: end of the file
 * @return MSG_OK if successful, otherwise MSG_TASK_CANCELED
 */
XBT_PUBLIC(int) surf_workstation_file_seek(surf_resource_t workstation, surf_file_t fd, sg_size_t offset, int origin);

/**
 * @brief [brief description]
 * @details [long description]
 * 
 * @param link [description]
 * @return [description]
 */
XBT_PUBLIC(int) surf_network_link_is_shared(surf_cpp_resource_t link);

/**
 * @brief Get the bandwidth of a link in bytes per second
 * 
 * @param link The surf link
 * @return The bandwidth in bytes per second
 */
XBT_PUBLIC(double) surf_network_link_get_bandwidth(surf_cpp_resource_t link);

/**
 * @brief Get the latency of a link in seconds
 * 
 * @param link The surf link
 * @return The latency in seconds
 */
XBT_PUBLIC(double) surf_network_link_get_latency(surf_cpp_resource_t link);

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
 * @brief Rename a path
 * 
 * @param resource The surf storage
 * @param src The old path
 * @param dest The new path
 */
XBT_PUBLIC(void) surf_storage_rename(surf_resource_t resource, const char* src, const char* dest);

/**
 * @brief Get the data associated to the action
 * 
 * @param action The surf action
 * @return The data associated to the action
 */
XBT_PUBLIC(void*) surf_action_get_data(surf_action_t action);

/**
 * @brief Set the data associated to the action
 * @details [long description]
 * 
 * @param action The surf action
 * @param data The new data associated to the action
 */
XBT_PUBLIC(void) surf_action_set_data(surf_action_t action, void *data);

/**
 * @brief Unreference an action
 * 
 * @param action The surf action
 */
XBT_PUBLIC(void) surf_action_unref(surf_action_t action);

/**
 * @brief Get the start time of an action
 * 
 * @param action The surf action
 * @return The start time in seconds from the beginning of the simulation
 */
XBT_PUBLIC(double) surf_action_get_start_time(surf_action_t action);

/**
 * @brief Get the finish time of an action
 * 
 * @param action The surf action
 * @return The finish time in seconds from the beginning of the simulation
 */
XBT_PUBLIC(double) surf_action_get_finish_time(surf_action_t action);

/**
 * @brief Get the remains amount of work to do of an action
 * 
 * @param action The surf action
 * @return  The remains amount of work to do
 */
XBT_PUBLIC(double) surf_action_get_remains(surf_action_t action);

/**
 * @brief Suspend an action
 * 
 * @param action The surf action
 */
XBT_PUBLIC(void) surf_action_suspend(surf_action_t action);

/**
 * @brief Resume an action
 * 
 * @param action The surf action
 */
XBT_PUBLIC(void) surf_action_resume(surf_action_t action);

/**
 * @brief Cancel an action
 * 
 * @param action The surf action
 */
XBT_PUBLIC(void) surf_action_cancel(surf_action_t action);

/**
 * @brief Set the priority of an action
 * @details [long description]
 * 
 * @param action The surf action
 * @param priority The new priority [TODO]
 */
XBT_PUBLIC(void) surf_action_set_priority(surf_action_t action, double priority);

/**
 * @brief Set the category of an action
 * @details [long description]
 * 
 * @param action The surf action
 * @param category The new category of the action
 */
XBT_PUBLIC(void) surf_action_set_category(surf_action_t action, const char *category);

/**
 * @brief Get the state of an action 
 * 
 * @param action The surf action
 * @return The state of the action
 */
XBT_PUBLIC(e_surf_action_state_t) surf_action_get_state(surf_action_t action);

/**
 * @brief Get the cost of an action
 * 
 * @param action The surf action
 * @return The cost of the action
 */
XBT_PUBLIC(double) surf_action_get_cost(surf_action_t action);

/**
 * @brief [brief desrciption]
 * @details [long description]
 * 
 * @param action The surf cpu action
 * @param cpu [description]
 * @param mask [description]
 */
XBT_PUBLIC(void) surf_cpu_action_set_affinity(surf_action_t action, surf_resource_t cpu, unsigned long mask);

/**
 * @brief [brief description]
 * @details [long description]
 * 
 * @param action The surf cpu action
 * @param bound [description]
 */
XBT_PUBLIC(void) surf_cpu_action_set_bound(surf_action_t action, double bound);

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

XBT_PUBLIC(surf_model_t) surf_resource_model(const void *host, int level);

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

/** \ingroup SURF_plugins
 *  \brief The list of all available surf plugins
 */
XBT_PUBLIC_DATA(s_surf_model_description_t) surf_plugin_description[];

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

XBT_PUBLIC_DATA(surf_storage_model_t) surf_storage_model;

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
 *  \brief The vm_workstation model
 *
 *  Note that when you create an API on top of SURF,
 *  the vm_workstation model should be the only one you use
 *  because depending on the platform model, the network model and the CPU model
 *  may not exist.
 */
XBT_PUBLIC_DATA(surf_vm_workstation_model_t) surf_vm_workstation_model;

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
XBT_PUBLIC(void) surf_vm_workstation_model_init_current_default(void);

/** \ingroup SURF_models
 *  \brief The list of all available vm workstation model models
 */
XBT_PUBLIC_DATA(s_surf_model_description_t)
    surf_vm_workstation_model_description[];

/*******************************************/

/** \ingroup SURF_models
 *  \brief List of initialized models
 */
XBT_PUBLIC_DATA(xbt_dynar_t) model_list;
XBT_PUBLIC_DATA(xbt_dynar_t) model_list_invoke;

/** \ingroup SURF_simulation
 *  \brief List of hosts that have juste restarted and whose autorestart process should be restarted.
 */
XBT_PUBLIC_DATA(xbt_dynar_t) host_that_restart;

/** \ingroup SURF_simulation
 *  \brief List of hosts for which one want to be notified if they ever restart.
 */
XBT_PUBLIC(xbt_dict_t) watched_hosts_lib;

/*******************************************/
/*** SURF Platform *************************/
/*******************************************/
XBT_PUBLIC_DATA(AS_t) surf_AS_get_routing_root(void); 
XBT_PUBLIC_DATA(const char *) surf_AS_get_name(AS_t as);
XBT_PUBLIC_DATA(xbt_dict_t) surf_AS_get_routing_sons(AS_t as);
XBT_PUBLIC_DATA(const char *) surf_AS_get_model(AS_t as);
XBT_PUBLIC_DATA(xbt_dynar_t) surf_AS_get_hosts(AS_t as);
XBT_PUBLIC_DATA(void) surf_AS_get_graph(AS_t as, xbt_graph_t graph, xbt_dict_t nodes, xbt_dict_t edges);
XBT_PUBLIC_DATA(AS_t) surf_platf_get_root(routing_platf_t platf);
XBT_PUBLIC_DATA(e_surf_network_element_type_t) surf_routing_edge_get_rc_type(sg_routing_edge_t edge);

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
