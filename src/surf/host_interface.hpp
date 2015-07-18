/* Copyright (c) 2004-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_interface.hpp"
#include "storage_interface.hpp"
#include "cpu_interface.hpp"
#include "network_interface.hpp"

#ifndef SURF_HOST_INTERFACE_HPP_
#define SURF_HOST_INTERFACE_HPP_

/***********
 * Classes *
 ***********/

class HostModel;
typedef HostModel *HostModelPtr;

class Host;
typedef Host *HostPtr;

class HostAction;
typedef HostAction *HostActionPtr;

/*************
 * Callbacks *
 *************/

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emit the callbacks after Host creation *
 * @details Callback functions have the following signature: `void(HostPtr)`
 */
XBT_PUBLIC_DATA(surf_callback(void, HostPtr)) hostCreatedCallbacks;

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emit the callbacks after Host destruction *
 * @details Callback functions have the following signature: `void(HostPtr)`
 */
XBT_PUBLIC_DATA(surf_callback(void, HostPtr)) hostDestructedCallbacks;

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emit the callbacks after Host State changed *
 * @details Callback functions have the following signature: `void(HostActionPtr action, e_surf_resource_state_t old, e_surf_resource_state_t current)`
 */
XBT_PUBLIC_DATA(surf_callback(void, HostPtr, e_surf_resource_state_t, e_surf_resource_state_t)) hostStateChangedCallbacks;

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emit the callbacks after HostAction State changed *
 * @details Callback functions have the following signature: `void(HostActionPtr action, e_surf_resource_state_t old, e_surf_resource_state_t current)`
 */
XBT_PUBLIC_DATA(surf_callback(void, HostActionPtr, e_surf_action_state_t, e_surf_action_state_t)) hostActionStateChangedCallbacks;

/*********
 * Tools *
 *********/
XBT_PUBLIC_DATA(HostModelPtr) surf_host_model;
XBT_PUBLIC(void) host_parse_init(sg_platf_host_cbarg_t host);
XBT_PUBLIC(void) host_add_traces();

/*********
 * Model *
 *********/
/** @ingroup SURF_host_interface
 * @brief SURF Host model interface class
 * @details A model is an object which handle the interactions between its Resources and its Actions
 */
class HostModel : public Model {
public:
    /**
   * @brief HostModel constructor
   *
   * @param name the name of the model
   */
  HostModel(const char *name);

  /** @brief HostModel constructor */
  HostModel();

  /** @brief HostModel destructor */
  ~HostModel();

  virtual HostPtr createHost(const char *name)=0;
  void addTraces(){DIE_IMPOSSIBLE;}

  /**
   * @brief [brief description]
   * @details [long description]
   */
  virtual void adjustWeightOfDummyCpuActions();

  /**
   * @brief [brief description]
   * @details [long description]
   *
   * @param host_nb [description]
   * @param host_list [description]
   * @param flops_amount [description]
   * @param bytes_amount [description]
   * @param rate [description]
   * @return [description]
   */
  virtual ActionPtr executeParallelTask(int host_nb,
                                        void **host_list,
                                        double *flops_amount,
                                        double *bytes_amount,
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
 virtual ActionPtr communicate(HostPtr src, HostPtr dst, double size, double rate)=0;

 CpuModelPtr p_cpuModel;
};

/************
 * Resource *
 ************/
/** @ingroup SURF_host_interface
 * @brief SURF Host interface class
 * @details An host represents a machine with a aggregation of a Cpu, a Link and a Storage
 */
class Host : public Resource {
public:
  /**
   * @brief Host constructor
   *
   * @param model HostModel associated to this Host
   * @param name The name of the Host
   * @param props Dictionary of properties associated to this Host
   * @param storage The Storage associated to this Host
   * @param netElm The RoutingEdge associated to this Host
   * @param cpu The Cpu associated to this Host
   */
  Host(ModelPtr model, const char *name, xbt_dict_t props,
		      xbt_dynar_t storage, RoutingEdgePtr netElm, CpuPtr cpu);

  /**
   * @brief Host constructor
   *
   * @param model HostModel associated to this Host
   * @param name The name of the Host
   * @param props Dictionary of properties associated to this Host
   * @param constraint The lmm constraint associated to this Host if it is part of a LMM component
   * @param storage The Storage associated to this Host
   * @param netElm The RoutingEdge associated to this Host
   * @param cpu The Cpu associated to this Host
   */
  Host(ModelPtr model, const char *name, xbt_dict_t props,
      lmm_constraint_t constraint, xbt_dynar_t storage, RoutingEdgePtr netElm,
      CpuPtr cpu);

  /** @brief Host destructor */
  ~ Host();

  void setState(e_surf_resource_state_t state);

  /**
   * @brief Get the properties of the current Host
   *
   * @return The properties of the current Host
   */
  xbt_dict_t getProperties();

  /**
   * @brief Execute some quantity of computation
   *
   * @param flops_amount The value of the processing amount (in flop) needed to process
   * @return The CpuAction corresponding to the processing
   * @see Cpu
   */
  virtual ActionPtr execute(double flops_amount)=0;

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
  virtual void setPstate(int pstate_index);
  virtual int  getPstate();

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
   * @brief Get the xbt_dynar_t of storages attached to the Host
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
   * @brief Set the position indicator associated with the file descriptor to a new position
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
  virtual int fileSeek(surf_file_t fd, sg_offset_t offset, int origin);

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
  LinkPtr p_network;

  /**
   * @brief Get the list of virtual machines on the current Host
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

/** @ingroup SURF_host_interface
 * @brief SURF host action interface class
 */
class HostAction : public Action {
public:
  /**
   * @brief HostAction constructor
   *
   * @param model The HostModel associated to this HostAction
   * @param cost The cost of this HostAction in [TODO]
   * @param failed [description]
   */
  HostAction(ModelPtr model, double cost, bool failed)
  : Action(model, cost, failed) {}

  /**
   * @brief HostAction constructor
   *
   * @param model The HostModel associated to this HostAction
   * @param cost The cost of this HostAction in [TODO]
   * @param failed [description]
   * @param var The lmm variable associated to this StorageAction if it is part of a LMM component
   */
  HostAction(ModelPtr model, double cost, bool failed, lmm_variable_t var)
  : Action(model, cost, failed, var) {}

  void setState(e_surf_action_state_t state);
};


#endif /* SURF_Host_INTERFACE_HPP_ */
