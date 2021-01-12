/* Copyright (c) 2004-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/resource/Action.hpp"
#include "simgrid/kernel/resource/Model.hpp"
#include "simgrid/kernel/resource/Resource.hpp"
#include "simgrid/s4u/Io.hpp"
#include "simgrid/s4u/Storage.hpp"
#include "surf_interface.hpp"
#include "xbt/PropertyHolder.hpp"

#include <map>

#ifndef STORAGE_INTERFACE_HPP_
#define STORAGE_INTERFACE_HPP_

/*********
 * Model *
 *********/

XBT_PUBLIC_DATA simgrid::kernel::resource::StorageModel* surf_storage_model;

namespace simgrid {
namespace kernel {
namespace resource {
/***********
 * Classes *
 ***********/

class StorageAction;
/** @ingroup SURF_storage_interface
 * @brief The possible type of action for the storage component
 */

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
  StorageModel(const StorageModel&) = delete;
  StorageModel& operator=(const StorageModel&) = delete;
  ~StorageModel() override;

  virtual StorageImpl* createStorage(std::string& filename, int lineno, const std::string& id,
                                     const std::string& type_id, const std::string& content_name,
                                     const std::string& attach) = 0;
};

/************
 * Resource *
 ************/
/** @ingroup SURF_storage_interface
 * @brief SURF storage interface class
 * @details A Storage represent a storage unit (e.g.: hard drive, usb key)
 */
class StorageImpl : public Resource, public xbt::PropertyHolder {
  s4u::Storage piface_;
  lmm::Constraint* constraint_read_;  /* Constraint for maximum read bandwidth*/
  lmm::Constraint* constraint_write_; /* Constraint for maximum write bandwidth*/

  std::string typeId_;
  bool currently_destroying_ = false;
  // Name of the host to which this storage is attached. Only used at platform parsing time, then the interface stores
  // the Host directly.
  std::string attach_;

public:
  const std::string content_name_; // Only used at parsing time then goes to the FileSystemExtension
  const sg_size_t size_;           // Only used at parsing time then goes to the FileSystemExtension
  /** @brief Storage constructor */
  StorageImpl(Model* model, const std::string& name, kernel::lmm::System* maxmin_system, double bread, double bwrite,
              const std::string& type_id, const std::string& content_name, sg_size_t size, const std::string& attach);
  StorageImpl(const StorageImpl&) = delete;
  StorageImpl& operator=(const StorageImpl&) = delete;

  ~StorageImpl() override;

  const s4u::Storage* get_iface() const { return &piface_; }
  s4u::Storage* get_iface() { return &piface_; }
  const char* get_type() const { return typeId_.c_str(); }
  lmm::Constraint* get_read_constraint() const { return constraint_read_; }
  lmm::Constraint* get_write_constraint() const { return constraint_write_; }
  /** @brief Check if the Storage is used (if an action currently uses its resources) */
  bool is_used() const override;

  void apply_event(profile::Event* event, double value) override;

  void turn_on() override;
  void turn_off() override;

  void destroy(); // Must be called instead of the destructor
  virtual StorageAction* io_start(sg_size_t size, s4u::Io::OpType type) = 0;
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
  const std::string& get_host() const { return attach_; }
};

/**********
 * Action *
 **********/

/** @ingroup SURF_storage_interface
 * @brief SURF storage action interface class
 */
class StorageAction : public Action {
public:
  /**
   * @brief Callbacks handler which emit the callbacks after StorageAction State changed *
   * @details Callback functions have the following signature: `void(StorageAction& action,
   * simgrid::kernel::resource::Action::State old, simgrid::kernel::resource::Action::State current)`
   */
  static xbt::signal<void(StorageAction const&, Action::State, Action::State)> on_state_change;

  /**
   * @brief StorageAction constructor
   *
   * @param model The StorageModel associated to this StorageAction
   * @param cost The cost of this StorageAction in bytes
   * @param failed [description]
   * @param storage The Storage associated to this StorageAction
   * @param type [description]
   */
  StorageAction(Model* model, double cost, bool failed, StorageImpl* storage, s4u::Io::OpType type)
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
  StorageType(const std::string& id, const std::string& model, const std::string& content,
              std::unordered_map<std::string, std::string>* properties,
              std::unordered_map<std::string, std::string>* model_properties, sg_size_t size)
      : id(id), model(model), content(content), properties(properties), model_properties(model_properties), size(size)
  {
  }
};

} // namespace resource
} // namespace kernel
} // namespace simgrid
#endif /* STORAGE_INTERFACE_HPP_ */
