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
class XBT_PRIVATE HostImpl;
class XBT_PRIVATE HostAction;


}
}

/*********
 * Tools *
 *********/

XBT_PUBLIC_DATA(simgrid::surf::HostModel*) surf_host_model;

/*********
 * Model *
 *********/

namespace simgrid {
namespace surf {

/** @ingroup SURF_host_interface
 * @brief SURF Host model interface class
 * @details A model is an object which handle the interactions between its Resources and its Actions
 */
class HostModel : public Model {
public:
  HostModel() : Model() {}
  ~HostModel() override {}

  HostImpl *createHost(const char *name, NetCard *net, Cpu *cpu, xbt_dict_t props);

  virtual void adjustWeightOfDummyCpuActions();
  virtual Action *executeParallelTask(int host_nb,
      sg_host_t *host_list,
      double *flops_amount,
      double *bytes_amount,
      double rate);

  bool next_occuring_event_isIdempotent() override {return true;}
};

/************
 * Resource *
 ************/
/** @ingroup SURF_host_interface
 * @brief SURF Host interface class
 * @details An host represents a machine with a aggregation of a Cpu, a RoutingEdge and a Storage
 */
class HostImpl
: public simgrid::surf::Resource,
  public simgrid::surf::PropertyHolder {
public:
  static simgrid::xbt::Extension<simgrid::s4u::Host, HostImpl> EXTENSION_ID;

public:
  static void classInit(); // must be called before the first use of that class
  /**
   * @brief Host constructor
   *
   * @param model HostModel associated to this Host
   * @param name The name of the Host
   * @param props Dictionary of properties associated to this Host
   * @param storage The Storage associated to this Host
   * @param cpu The Cpu associated to this Host
   */
  HostImpl(HostModel *model, const char *name, xbt_dict_t props,
          xbt_dynar_t storage, Cpu *cpu);

  /**
   * @brief Host constructor
   *
   * @param model HostModel associated to this Host
   * @param name The name of the Host
   * @param props Dictionary of properties associated to this Host
   * @param constraint The lmm constraint associated to this Host if it is part of a LMM component
   * @param storage The Storage associated to this Host
   * @param cpu The Cpu associated to this Host
   */
  HostImpl(HostModel *model, const char *name, xbt_dict_t props,
      lmm_constraint_t constraint, xbt_dynar_t storage, Cpu *cpu);

  /* Host destruction logic */
  /**************************/
  ~HostImpl();

public:
  HostModel *getModel()
  {
    return static_cast<HostModel*>(Resource::getModel());
  }
  void attach(simgrid::s4u::Host* host);

  bool isOn() override;
  bool isOff() override;
  void turnOn() override;
  void turnOff() override;

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

  bool isUsed() override {DIE_IMPOSSIBLE;} // FIXME: Host should not be a Resource
  void apply_event(tmgr_trace_iterator_t event, double value) override
    {THROW_IMPOSSIBLE;} // FIXME: Host should not be a Resource

public:
  xbt_dynar_t p_storage;
  Cpu *p_cpu;
  simgrid::s4u::Host* p_host = nullptr;

  /** @brief Get the list of virtual machines on the current Host */
  xbt_dynar_t getVms();

  /* common with vm */
  /** @brief Retrieve a copy of the parameters of that VM/PM
   *  @details The ramsize and overcommit fields are used on the PM too */
  void getParams(vm_params_t params);
  /** @brief Sets the params of that VM/PM */
  void setParams(vm_params_t params);
  simgrid::s4u::Host* getHost() { return p_host; }
private:
  s_vm_params_t p_params;
};

}
}

#endif /* SURF_Host_INTERFACE_HPP_ */
