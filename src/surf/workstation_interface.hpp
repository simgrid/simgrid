/* Copyright (c) 2004-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_interface.hpp"
#include "storage_interface.hpp"
#include "cpu_interface.hpp"
#include "network_interface.hpp"

#ifndef SURF_WORKSTATION_INTERFACE_HPP_
#define SURF_WORKSTATION_INTERFACE_HPP_

/***********
 * Classes *
 ***********/

class WorkstationModel;
typedef WorkstationModel *WorkstationModelPtr;

class Workstation;
typedef Workstation *WorkstationPtr;

class WorkstationAction;
typedef WorkstationAction *WorkstationActionPtr;

/*************
 * Callbacks *
 *************/

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emit the callbacks after Workstation creation *
 * @details Callback functions have the following signature: `void(WorkstationPtr)`
 */
extern surf_callback(void, WorkstationPtr) workstationCreatedCallbacks;

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emit the callbacks after Workstation destruction *
 * @details Callback functions have the following signature: `void(WorkstationPtr)`
 */
extern surf_callback(void, WorkstationPtr) workstationDestructedCallbacks;

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emit the callbacks after Workstation State changed *
 * @details Callback functions have the following signature: `void(WorkstationActionPtr action, e_surf_resource_state_t old, e_surf_resource_state_t current)`
 */
extern surf_callback(void, WorkstationPtr, e_surf_resource_state_t, e_surf_resource_state_t) workstationStateChangedCallbacks;

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emit the callbacks after WorkstationAction State changed *
 * @details Callback functions have the following signature: `void(WorkstationActionPtr action, e_surf_resource_state_t old, e_surf_resource_state_t current)`
 */
extern surf_callback(void, WorkstationActionPtr, e_surf_action_state_t, e_surf_action_state_t) workstationActionStateChangedCallbacks;

/*********
 * Tools *
 *********/
extern WorkstationModelPtr surf_workstation_model;

/*********
 * Model *
 *********/
/** @ingroup SURF_workstation_interface
 * @brief SURF Workstation model interface class
 * @details A model is an object which handle the interactions between its Resources and its Actions
 */
class WorkstationModel : public Model {
public:
    /**
   * @brief WorkstationModel constructor
   *
   * @param name the name of the model
   */
  WorkstationModel(const char *name);

  /**
   * @brief WorkstationModel constructor
   */
  WorkstationModel();

  /**
   * @brief WorkstationModel destructor
   */
  ~WorkstationModel();

  /**
   * @brief [brief description]
   * @details [long description]
   */
  virtual void adjustWeightOfDummyCpuActions();

  /**
   * @brief [brief description]
   * @details [long description]
   *
   * @param workstation_nb [description]
   * @param workstation_list [description]
   * @param computation_amount [description]
   * @param communication_amount [description]
   * @param rate [description]
   * @return [description]
   */
  virtual ActionPtr executeParallelTask(int workstation_nb,
                                        void **workstation_list,
                                        double *computation_amount,
                                        double *communication_amount,
                                        double rate)=0;

 /**
  * @brief [brief description]
  * @details [long description]
  *
  * @param src [description]
  * @param dst [description]
  * @param size [description]
  * @param rate [description]
  * @return [description]
  */
 virtual ActionPtr communicate(WorkstationPtr src, WorkstationPtr dst, double size, double rate)=0;

 CpuModelPtr p_cpuModel;
};

/************
 * Resource *
 ************/
/** @ingroup SURF_workstation_interface
 * @brief SURF Workstation interface class
 * @details A workstation VM represent an virtual machine with a aggregation of a Cpu, a NetworkLink and a Storage
 */
class Workstation : public Resource {
public:
  /**
   * @brief Workstation consrtuctor
   */
  Workstation();

  /**
   * @brief Workstation constructor
   *
   * @param model WorkstationModel associated to this Workstation
   * @param name The name of the Workstation
   * @param props Dictionary of properties associated to this Workstation
   * @param storage The Storage associated to this Workstation
   * @param netElm The RoutingEdge associated to this Workstation
   * @param cpu The Cpu associated to this Workstation
   */
  Workstation(ModelPtr model, const char *name, xbt_dict_t props,
		      xbt_dynar_t storage, RoutingEdgePtr netElm, CpuPtr cpu);

  /**
   * @brief Workstation constructor
   *
   * @param model WorkstationModel associated to this Workstation
   * @param name The name of the Workstation
   * @param props Dictionary of properties associated to this Workstation
   * @param constraint The lmm constraint associated to this Workstation if it is part of a LMM component
   * @param storage The Storage associated to this Workstation
   * @param netElm The RoutingEdge associated to this Workstation
   * @param cpu The Cpu associated to this Workstation
   */
  Workstation(ModelPtr model, const char *name, xbt_dict_t props, lmm_constraint_t constraint,
		      xbt_dynar_t storage, RoutingEdgePtr netElm, CpuPtr cpu);

  /**
   * @brief Workstation destructor
   */
  ~ Workstation();

  void setState(e_surf_resource_state_t state);

  /**
   * @brief Get the properties of the currenrt Workstation
   *
   * @return The properties of the current Workstation
   */
  xbt_dict_t getProperties();

  /**
   * @brief Execute some quantity of computation
   *
   * @param size The value of the processing amount (in flop) needed to process
   * @return The CpuAction corresponding to the processing
   * @see Cpu
   */
  virtual ActionPtr execute(double size)=0;

  /**
   * @brief Make a process sleep for duration seconds
   *
   * @param duration The number of seconds to sleep
   * @return The CpuAction corresponding to the sleeping
   * @see Cpu
   */
  virtual ActionPtr sleep(double duration)=0;

  /**
   * @brief Get the number of cores of the associated Cpu
   *
   * @return The number of cores of the associated Cpu
   * @see Cpu
   */
  virtual int getCore();

  /**
   * @brief Get the speed of the associated Cpu
   *
   * @param load [TODO]
   * @return The speed of the associated Cpu
   * @see Cpu
   */
  virtual double getSpeed(double load);

  /**
   * @brief Get the available speed of the associated Cpu
   * @details [TODO]
   *
   * @return The available speed of the associated Cpu
   * @see Cpu
   */
  virtual double getAvailableSpeed();

  /**
   * @brief Get the associated Cpu power peak
   *
   * @return The associated Cpu power peak
   * @see Cpu
   */
  virtual double getCurrentPowerPeak();

  virtual double getPowerPeakAt(int pstate_index);
  virtual int getNbPstates();
  virtual void setPowerPeakAt(int pstate_index);

  /**
   * @brief Return the storage of corresponding mount point
   *
   * @param storage The mount point
   * @return The corresponding Storage
   */
  virtual StoragePtr findStorageOnMountList(const char* storage);

  /**
   * @brief Get the xbt_dict_t of mount_point: Storage
   *
   * @return The xbt_dict_t of mount_point: Storage
   */
  virtual xbt_dict_t getMountedStorageList();

  /**
   * @brief Get the xbt_dynar_t of storages attached to the workstation
   *
   * @return The xbt_dynar_t of Storage names
   */
  virtual xbt_dynar_t getAttachedStorageList();

  /**
   * @brief Open a file
   *
   * @param fullpath The full path to the file
   *
   * @return The StorageAction corresponding to the opening
   */
  virtual ActionPtr open(const char* fullpath);

  /**
   * @brief Close a file
   *
   * @param fd The file descriptor to close
   * @return The StorageAction corresponding to the closing
   */
  virtual ActionPtr close(surf_file_t fd);

  /**
   * @brief Unlink a file
   * @details [long description]
   *
   * @param fd [description]
   * @return [description]
   */
  virtual int unlink(surf_file_t fd);

  /**
   * @brief Get the size in bytes of the file
   *
   * @param fd The file descriptor to read
   * @return The size in bytes of the file
   */
  virtual sg_size_t getSize(surf_file_t fd);

  /**
   * @brief Read a file
   *
   * @param fd The file descriptor to read
   * @param size The size in bytes to read
   * @return The StorageAction corresponding to the reading
   */
  virtual ActionPtr read(surf_file_t fd, sg_size_t size);

  /**
   * @brief Write a file
   *
   * @param fd The file descriptor to write
   * @param size The size in bytes to write
   * @return The StorageAction corresponding to the writing
   */
  virtual ActionPtr write(surf_file_t fd, sg_size_t size);

  /**
   * @brief Get the informations of a file descriptor
   * @details The returned xbt_dynar_t contains:
   *  - the size of the file,
   *  - the mount point,
   *  - the storage name,
   *  - the storage typeId,
   *  - the storage content type
   *
   * @param fd The file descriptor
   * @return An xbt_dynar_t with the file informations
   */
  virtual xbt_dynar_t getInfo(surf_file_t fd);

  /**
   * @brief Get the current position of the file descriptor
   *
   * @param fd The file descriptor
   * @return The current position of the file descriptor
   */
  virtual sg_size_t fileTell(surf_file_t fd);

  /**
   * @brief Get the available space of the storage at the mount point
   *
   * @param name The mount point
   * @return The amount of availble space in bytes
   */
  virtual sg_size_t getFreeSize(const char* name);

  /**
   * @brief Get the used space of the storage at the mount point
   *
   * @param name The mount point
   * @return The amount of used space in bytes
   */
  virtual sg_size_t getUsedSize(const char* name);

  /**
   * @brief Set the position indictator assiociated with the file descriptor to a new position
   * @details [long description]
   *
   * @param fd The file descriptor
   * @param offset The offset from the origin
   * @param origin Position used as a reference for the offset
   *  - SEEK_SET: beginning of the file
   *  - SEEK_CUR: current position indicator
   *  - SEEK_END: end of the file
   * @return MSG_OK if successful, otherwise MSG_TASK_CANCELED
   */
  virtual int fileSeek(surf_file_t fd, sg_size_t offset, int origin);

  /**
   * @brief Move a file to another location on the *same mount point*.
   * @details [long description]
   *
   * @param fd The file descriptor
   * @param fullpath The new full path
   * @return MSG_OK if successful, MSG_TASK_CANCELED and a warning if the new
   * full path is not on the same mount point
   */
  virtual int fileMove(surf_file_t fd, const char* fullpath);

  xbt_dynar_t p_storage;
  RoutingEdgePtr p_netElm;
  CpuPtr p_cpu;
  NetworkLinkPtr p_network;

  /**
   * @brief Get the list of virtual machines on the current Workstation
   *
   * @return The list of VMs
   */
  xbt_dynar_t getVms();

  /* common with vm */
  /**
   * @brief [brief description]
   * @details [long description]
   *
   * @param params [description]
   */
  void getParams(ws_params_t params);

  /**
   * @brief [brief description]
   * @details [long description]
   *
   * @param params [description]
   */
  void setParams(ws_params_t params);
  s_ws_params_t p_params;
};

/**********
 * Action *
 **********/

/** @ingroup SURF_workstation_interface
 * @brief SURF workstation action interface class
 */
class WorkstationAction : public Action {
public:
  /**
   * @brief WorkstationAction constructor
   *
   * @param model The WorkstationModel associated to this WorkstationAction
   * @param cost The cost of this WorkstationAction in [TODO]
   * @param failed [description]
   */
  WorkstationAction(ModelPtr model, double cost, bool failed)
  : Action(model, cost, failed) {}

  /**
   * @brief WorkstationAction constructor
   *
   * @param model The WorkstationModel associated to this WorkstationAction
   * @param cost The cost of this WorkstationAction in [TODO]
   * @param failed [description]
   * @param var The lmm variable associated to this StorageAction if it is part of a LMM component
   */
  WorkstationAction(ModelPtr model, double cost, bool failed, lmm_variable_t var)
  : Action(model, cost, failed, var) {}

  void setState(e_surf_action_state_t state);
};


#endif /* SURF_WORKSTATION_INTERFACE_HPP_ */
