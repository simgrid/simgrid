/* Copyright (c) 2004-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/base.h>
#include <xbt/signal.hpp>

#include "simgrid/s4u/Storage.hpp"
#include "src/surf/PropertyHolder.hpp"
#include "surf_interface.hpp"
#include <map>

#ifndef STORAGE_INTERFACE_HPP_
#define STORAGE_INTERFACE_HPP_

namespace simgrid {
namespace surf {

/***********
 * Classes *
 ***********/

class StorageAction;

/*************
 * Callbacks *
 *************/

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emit the callbacks after Storage creation *
 * @details Callback functions have the following signature: `void(Storage*)`
 */
XBT_PUBLIC_DATA(simgrid::xbt::signal<void(StorageImpl*)>) storageCreatedCallbacks;

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emit the callbacks after Storage destruction *
 * @details Callback functions have the following signature: `void(StoragePtr)`
 */
XBT_PUBLIC_DATA(simgrid::xbt::signal<void(StorageImpl*)>) storageDestructedCallbacks;

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emit the callbacks after Storage State changed *
 * @details Callback functions have the following signature: `void(StorageAction *action, int previouslyOn, int
 * currentlyOn)`
 */
XBT_PUBLIC_DATA(simgrid::xbt::signal<void(StorageImpl*, int, int)>) storageStateChangedCallbacks;

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emit the callbacks after StorageAction State changed *
 * @details Callback functions have the following signature: `void(StorageAction *action, simgrid::surf::Action::State
 * old, simgrid::surf::Action::State current)`
 */
XBT_PUBLIC_DATA(simgrid::xbt::signal<void(StorageAction*, Action::State, Action::State)>)
storageActionStateChangedCallbacks;

/*********
 * Model *
 *********/
/** @ingroup SURF_storage_interface
 * @brief SURF storage model interface class
 * @details A model is an object which handle the interactions between its Resources and its Actions
 */
class StorageModel : public Model {
public:
  StorageModel();
  ~StorageModel();

  virtual StorageImpl* createStorage(std::string id, std::string type_id, std::string content_name,
                                     std::string attach) = 0;

  std::vector<StorageImpl*> p_storageList;
};

/************
 * Resource *
 ************/
/** @ingroup SURF_storage_interface
 * @brief SURF storage interface class
 * @details A Storage represent a storage unit (e.g.: hard drive, usb key)
 */
class StorageImpl : public Resource, public PropertyHolder {
public:
  /** @brief Storage constructor */
  StorageImpl(Model* model, std::string name, lmm_system_t maxminSystem, double bread, double bwrite,
              std::string type_id, std::string content_name, sg_size_t size, std::string attach);

  ~StorageImpl() override;

  /** @brief Public interface */
  s4u::Storage piface_;
  static StorageImpl* byName(std::string name);

  /** @brief Check if the Storage is used (if an action currently uses its resources) */
  bool isUsed() override;

  void apply_event(tmgr_trace_event_t event, double value) override;

  void turnOn() override;
  void turnOff() override;

  /**
   * @brief Read a file
   *
   * @param size The size in bytes to read
   * @return The StorageAction corresponding to the reading
   */
  virtual StorageAction* read(sg_size_t size) = 0;

  /**
   * @brief Write a file
   *
   * @param size The size in bytes to write
   * @return The StorageAction corresponding to the writing
   */
  virtual StorageAction* write(sg_size_t size) = 0;
  virtual std::string getHost() { return attach_; }

  static std::unordered_map<std::string, StorageImpl*>* storagesMap() { return StorageImpl::storages; }

  lmm_constraint_t constraintWrite_; /* Constraint for maximum write bandwidth*/
  lmm_constraint_t constraintRead_;  /* Constraint for maximum write bandwidth*/

  std::string typeId_;
  std::string content_name; // Only used at parsing time then goes to the FileSystemExtension
  sg_size_t size_;          // Only used at parsing time then goes to the FileSystemExtension

private:
  static std::unordered_map<std::string, StorageImpl*>* storages;
  // Name of the host to which this storage is attached. Only used at platform parsing time, then the interface stores
  // the Host directly.
  std::string attach_;
};

/**********
 * Action *
 **********/

/** @ingroup SURF_storage_interface
 * @brief The possible type of action for the storage component
 */
enum e_surf_action_storage_type_t {
  READ = 0, /**< Read a file */
  WRITE     /**< Write in a file */
};

/** @ingroup SURF_storage_interface
 * @brief SURF storage action interface class
 */
class StorageAction : public Action {
public:
  /**
   * @brief StorageAction constructor
   *
   * @param model The StorageModel associated to this StorageAction
   * @param cost The cost of this  NetworkAction in [TODO]
   * @param failed [description]
   * @param storage The Storage associated to this StorageAction
   * @param type [description]
   */
  StorageAction(Model* model, double cost, bool failed, StorageImpl* storage, e_surf_action_storage_type_t type)
      : Action(model, cost, failed), type_(type), storage_(storage){};

  /**
 * @brief StorageAction constructor
 *
 * @param model The StorageModel associated to this StorageAction
 * @param cost The cost of this  StorageAction in [TODO]
 * @param failed [description]
 * @param var The lmm variable associated to this StorageAction if it is part of a LMM component
 * @param storage The Storage associated to this StorageAction
 * @param type [description]
 */
  StorageAction(Model* model, double cost, bool failed, lmm_variable_t var, StorageImpl* storage,
                e_surf_action_storage_type_t type)
      : Action(model, cost, failed, var), type_(type), storage_(storage){};

  void setState(simgrid::surf::Action::State state) override;

  e_surf_action_storage_type_t type_;
  StorageImpl* storage_;
};

class StorageType {
public:
  std::string id;
  std::string model;
  std::string content;
  std::map<std::string, std::string>* properties;
  std::map<std::string, std::string>* model_properties;
  sg_size_t size;
  StorageType(std::string id, std::string model, std::string content, std::map<std::string, std::string>* properties,
              std::map<std::string, std::string>* model_properties, sg_size_t size)
      : id(id), model(model), content(content), properties(properties), model_properties(model_properties), size(size)
  {
  }
};
}
}

#endif /* STORAGE_INTERFACE_HPP_ */
