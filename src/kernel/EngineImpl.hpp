/* Copyright (c) 2016-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_ENGINEIMPL_HPP
#define SIMGRID_KERNEL_ENGINEIMPL_HPP

#include <simgrid/kernel/resource/Model.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/NetZone.hpp>
#include <simgrid/simix.hpp>

#include <map>
#include <string>
#include <unordered_map>

namespace simgrid {
namespace kernel {

class EngineImpl {
  std::map<std::string, s4u::Host*, std::less<>> hosts_;
  std::map<std::string, resource::LinkImpl*, std::less<>> links_;
  std::unordered_map<std::string, routing::NetPoint*> netpoints_;
  std::unordered_map<std::string, actor::ActorCodeFactory> registered_functions; // Maps function names to actor code
  actor::ActorCodeFactory default_function; // Function to use as a fallback when the provided name matches nothing
  std::vector<resource::Model*> models_;
  struct ModelStruct {
    int prio;
    std::shared_ptr<resource::Model> ptr;
  };
  std::unordered_map<std::string, struct ModelStruct> models_prio_;
  routing::NetZoneImpl* netzone_root_ = nullptr;

  friend s4u::Engine;

public:
  EngineImpl() = default;

  EngineImpl(const EngineImpl&) = delete;
  EngineImpl& operator=(const EngineImpl&) = delete;
  virtual ~EngineImpl();

  void load_deployment(const std::string& file) const;
  void register_function(const std::string& name, const actor::ActorCodeFactory& code);
  void register_default(const actor::ActorCodeFactory& code);

  /**
   * @brief Add a model to engine list
   *
   * @param model Pointer to model
   * @param list  List of dependencies for this model
   */
  void add_model(std::shared_ptr<simgrid::kernel::resource::Model> model, std::vector<std::string>&& dep_models);

  /** @brief Get list of all models managed by this engine */
  const std::vector<resource::Model*>& get_all_models() const { return models_; }

  static EngineImpl* get_instance() { return simgrid::s4u::Engine::get_instance()->pimpl; }
  actor::ActorCodeFactory get_function(const std::string& name)
  {
    auto res = registered_functions.find(name);
    if (res == registered_functions.end())
      return default_function;
    else
      return res->second;
  }
};

} // namespace kernel
} // namespace simgrid

#endif
