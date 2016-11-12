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

  virtual void adjustWeightOfDummyCpuActions();
  virtual Action *executeParallelTask(int host_nb, sg_host_t *host_list,
      double *flops_amount, double *bytes_amount, double rate);
};

/************
 * Resource *
 ************/
/** @ingroup SURF_host_interface
 * @brief SURF Host interface class
 * @details An host represents a machine with a aggregation of a Cpu, a RoutingEdge and a Storage
 */
class HostImpl : public simgrid::surf::PropertyHolder {

public:
  /**
   * @brief Host constructor
   *
   * @param storage The Storage associated to this Host
   */
  HostImpl(s4u::Host* host, xbt_dynar_t storage);

  /* Host destruction logic */
  /**************************/
  virtual ~HostImpl();

public:
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
   * @brief Get the information of a file descriptor
   * @details The returned xbt_dynar_t contains:
   *  - the size of the file,
   *  - the mount point,
   *  - the storage name,
   *  - the storage typeId,
   *  - the storage content type
   *
   * @param fd The file descriptor
   * @return An xbt_dynar_t with the file information
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
  xbt_dynar_t storage_        = nullptr;
  simgrid::s4u::Host* piface_ = nullptr;

  /** @brief Get the list of virtual machines on the current Host */
  xbt_dynar_t getVms();

  simgrid::s4u::Host* getHost() { return piface_; }
};

}
}

#endif /* SURF_Host_INTERFACE_HPP_ */
