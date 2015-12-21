/* Copyright (c) 2004-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_interface.hpp"
#include "storage_interface.hpp"
#include "cpu_interface.hpp"
#include "network_interface.hpp"
#include "src/surf/PropertyHolder.hpp"

#include <xbt/base.h>

#ifndef SURF_HOST_INTERFACE_HPP_
#define SURF_HOST_INTERFACE_HPP_

/***********
 * Classes *
 ***********/

namespace simgrid {
namespace surf {

class XBT_PRIVATE HostModel;
class XBT_PRIVATE Host;
class XBT_PRIVATE HostAction;


}
}

/*********
 * Tools *
 *********/
XBT_PUBLIC_DATA(simgrid::surf::HostModel*) surf_host_model;
XBT_PUBLIC(void) host_add_traces();

/*********
 * Model *
 *********/

namespace simgrid {
namespace surf {

/** @ingroup SURF_host_interface
 * @brief SURF Host model interface class
 * @details A model is an object which handle the interactions between its Resources and its Actions
 */
class HostModel : public Model{
public:
  HostModel() : Model() {}
  ~HostModel() {}

  virtual Host *createHost(const char *name, RoutingEdge *net, Cpu *cpu, xbt_dict_t props)=0;
  void addTraces() override {DIE_IMPOSSIBLE;}

  virtual void adjustWeightOfDummyCpuActions();
  virtual Action *executeParallelTask(int host_nb,
                                      sg_host_t *host_list,
									  double *flops_amount,
									  double *bytes_amount,
									  double rate)=0;

  bool shareResourcesIsIdempotent() override {return true;}
};

/************
 * Resource *
 ************/
/** @ingroup SURF_host_interface
 * @brief SURF Host interface class
 * @details An host represents a machine with a aggregation of a Cpu, a RoutingEdge and a Storage
 */
class Host : public simgrid::surf::Resource,
	         public simgrid::surf::PropertyHolder {
public:
  static simgrid::xbt::Extension<simgrid::Host, Host> EXTENSION_ID;

  /* callbacks */
  static simgrid::surf::signal<void(Host*)> onCreation;    /** Called on each newly created object */
  static simgrid::surf::signal<void(Host*)> onDestruction; /** Called just before destructing an object */
  static simgrid::surf::signal<void(simgrid::surf::Host*, e_surf_resource_state_t, e_surf_resource_state_t)> onStateChange;

public:
  static void classInit();
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
  Host(simgrid::surf::Model *model, const char *name, xbt_dict_t props,
		      xbt_dynar_t storage, RoutingEdge *netElm, Cpu *cpu);

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
  Host(simgrid::surf::Model *model, const char *name, xbt_dict_t props,
      lmm_constraint_t constraint, xbt_dynar_t storage, RoutingEdge *netElm,
      Cpu *cpu);

  /* Host destruction logic */
  /**************************/
protected:
  ~Host();
public:
	void destroy(); // Must be called instead of the destructor
private:
	bool currentlyDestroying_ = false;


public:
  void attach(simgrid::Host* host);

  e_surf_resource_state_t getState();
  void setState(e_surf_resource_state_t state);

  /**
   * @brief Execute some quantity of computation
   *
   * @param flops_amount The value of the processing amount (in flop) needed to process
   * @return The CpuAction corresponding to the processing
   * @see Cpu
   */
  virtual Action *execute(double flops_amount)=0;

  /**
   * @brief Make a process sleep for duration seconds
   *
   * @param duration The number of seconds to sleep
   * @return The CpuAction corresponding to the sleeping
   * @see Cpu
   */
  virtual Action *sleep(double duration)=0;

  /** @brief Return the storage of corresponding mount point */
  virtual simgrid::surf::Storage *findStorageOnMountList(const char* storage);

  /** @brief Get the xbt_dict_t of mount_point: Storage */
  virtual xbt_dict_t getMountedStorageList();

  /** @brief Get the xbt_dynar_t of storages attached to the Host */
  virtual xbt_dynar_t getAttachedStorageList();

  /**
   * @brief Open a file
   *
   * @param fullpath The full path to the file
   * @return The StorageAction corresponding to the opening
   */
  virtual Action *open(const char* fullpath);

  /**
   * @brief Close a file
   *
   * @param fd The file descriptor to close
   * @return The StorageAction corresponding to the closing
   */
  virtual Action *close(surf_file_t fd);

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
  virtual Action *read(surf_file_t fd, sg_size_t size);

  /**
   * @brief Write a file
   *
   * @param fd The file descriptor to write
   * @param size The size in bytes to write
   * @return The StorageAction corresponding to the writing
   */
  virtual Action *write(surf_file_t fd, sg_size_t size);

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

public:
  xbt_dynar_t p_storage;
  RoutingEdge *p_netElm;
  Cpu *p_cpu;
  simgrid::Host* p_host = nullptr;

  /** @brief Get the list of virtual machines on the current Host */
  xbt_dynar_t getVms();

  /* common with vm */
  /** @brief Retrieve a copy of the parameters of that VM/PM
   *  @details The ramsize and overcommit fields are used on the PM too */
  void getParams(vm_params_t params);
  /** @brief Sets the params of that VM/PM */
  void setParams(vm_params_t params);
  simgrid::Host* getHost() { return p_host; }
private:
  s_vm_params_t p_params;
};

/**********
 * Action *
 **********/

/** @ingroup SURF_host_interface
 * @brief SURF host action interface class
 */
class HostAction : public Action {
public:
	static simgrid::surf::signal<void(simgrid::surf::HostAction*, e_surf_action_state_t, e_surf_action_state_t)> onStateChange;

  /**
   * @brief HostAction constructor
   *
   * @param model The HostModel associated to this HostAction
   * @param cost The cost of this HostAction in [TODO]
   * @param failed [description]
   */
  HostAction(simgrid::surf::Model *model, double cost, bool failed)
  : Action(model, cost, failed) {}

  /**
   * @brief HostAction constructor
   *
   * @param model The HostModel associated to this HostAction
   * @param cost The cost of this HostAction in [TODO]
   * @param failed [description]
   * @param var The lmm variable associated to this StorageAction if it is part of a LMM component
   */
  HostAction(simgrid::surf::Model *model, double cost, bool failed, lmm_variable_t var)
  : Action(model, cost, failed, var) {}

  void setState(e_surf_action_state_t state);
};

}
}

#endif /* SURF_Host_INTERFACE_HPP_ */
