/* Copyright (c) 2004-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/resource/Action.hpp"
#include "simgrid/kernel/resource/Model.hpp"
#include "simgrid/kernel/resource/Resource.hpp"
#include "simgrid/s4u/Io.hpp"
#include "simgrid/s4u/Storage.hpp"
#include "src/surf/PropertyHolder.hpp"
#include "src/surf/trace_mgr.hpp"
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
/** @ingroup SURF_storage_interface
 * @brief The possible type of action for the storage component
 */
/*************
 * Callbacks *
 *************/

/** @ingroup SURF_callbacks
 * @brief Callbacks handler which emit the callbacks after StorageAction State changed *
 * @details Callback functions have the following signature: `void(StorageAction *action,
 * simgrid::kernel::resource::Action::State old, simgrid::kernel::resource::Action::State current)`
 */
XBT_PUBLIC_DATA
simgrid::xbt::signal<void(StorageAction*, kernel::resource::Action::State, kernel::resource::Action::State)>
    on_state_change;

/*********
 * Model *
 *********/
/** @ingroup SURF_storage_interface
 * @brief SURF storage model interface class
 * @details A model is an object which handle the interactions between its Resources and its Actions
 */
class StorageModel : public kernel::resource::Model {
public:
  StorageModel();
  ~StorageModel();

  virtual StorageImpl* createStorage(std::string id, std::string type_id, std::string content_name,
                                     std::string attach) = 0;
};

/************
 * Resource *
 ************/
/** @ingroup SURF_storage_interface
 * @brief SURF storage interface class
 * @details A Storage represent a storage unit (e.g.: hard drive, usb key)
 */
class StorageImpl : public kernel::resource::Resource, public PropertyHolder {
public:
  /** @brief Storage constructor */
  StorageImpl(kernel::resource::Model* model, std::string name, kernel::lmm::System* maxmin_system, double bread,
              double bwrite, std::string type_id, std::string content_name, sg_size_t size, std::string attach);

  ~StorageImpl() override;

  /** @brief Public interface */
  s4u::Storage piface_;

  /** @brief Check if the Storage is used (if an action currently uses its resources) */
  bool is_used() override;

  void apply_event(tmgr_trace_event_t event, double value) override;

  void turn_on() override;
  void turn_off() override;

  void destroy(); // Must be called instead of the destructor
  virtual simgrid::kernel::resource::Action* io_start(sg_size_t size, s4u::Io::OpType type) = 0;
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

  kernel::lmm::Constraint* constraintWrite_; /* Constraint for maximum write bandwidth*/
  kernel::lmm::Constraint* constraintRead_;  /* Constraint for maximum write bandwidth*/

  std::string typeId_;
  std::string content_name; // Only used at parsing time then goes to the FileSystemExtension
  sg_size_t size_;          // Only used at parsing time then goes to the FileSystemExtension

private:
  bool currentlyDestroying_ = false;
  // Name of the host to which this storage is attached. Only used at platform parsing time, then the interface stores
  // the Host directly.
  std::string attach_;
};

/**********
 * Action *
 **********/

/** @ingroup SURF_storage_interface
 * @brief SURF storage action interface class
 */
class StorageAction : public kernel::resource::Action {
public:
  static xbt::signal<void(StorageAction*, kernel::resource::Action::State, kernel::resource::Action::State)>
      on_state_change;

  /**
   * @brief StorageAction constructor
   *
   * @param model The StorageModel associated to this StorageAction
   * @param cost The cost of this StorageAction in bytes
   * @param failed [description]
   * @param storage The Storage associated to this StorageAction
   * @param type [description]
   */
  StorageAction(kernel::resource::Model* model, double cost, bool failed, StorageImpl* storage, s4u::Io::OpType type)
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
  StorageAction(kernel::resource::Model* model, double cost, bool failed, kernel::lmm::Variable* var,
                StorageImpl* storage, s4u::Io::OpType type)
      : Action(model, cost, failed, var), type_(type), storage_(storage){};

  void set_state(simgrid::kernel::resource::Action::State state) override;

  s4u::Io::OpType type_;
  StorageImpl* storage_;
};

class StorageType {
public:
  std::string id;
  std::string model;
  std::string content;
  std::unordered_map<std::string, std::string>* properties;
  std::unordered_map<std::string, std::string>* model_properties;
  sg_size_t size;
  StorageType(std::string id, std::string model, std::string content,
              std::unordered_map<std::string, std::string>* properties,
              std::unordered_map<std::string, std::string>* model_properties, sg_size_t size)
      : id(id), model(model), content(content), properties(properties), model_properties(model_properties), size(size)
  {
  }
};
}
}

#endif /* STORAGE_INTERFACE_HPP_ */
