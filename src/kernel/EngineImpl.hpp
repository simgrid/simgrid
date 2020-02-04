/* Copyright (c) 2016-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <map>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/NetZone.hpp>
#include <simgrid/simix.hpp>
#include <string>
#include <unordered_map>

namespace simgrid {
namespace kernel {

class EngineImpl {
  std::map<std::string, s4u::Host*> hosts_;
  std::map<std::string, resource::LinkImpl*> links_;
  std::map<std::string, resource::StorageImpl*> storages_;
  std::unordered_map<std::string, routing::NetPoint*> netpoints_;
  std::unordered_map<std::string, actor::ActorCodeFactory> registered_functions; // Maps function names to actor code
  actor::ActorCodeFactory default_function; // Function to use as a fallback when the provided name matches nothing

  friend s4u::Engine;

public:
  EngineImpl() = default;

  EngineImpl(const EngineImpl&) = delete;
  EngineImpl& operator=(const EngineImpl&) = delete;
  virtual ~EngineImpl();

  void load_deployment(const std::string& file);
  void register_function(const std::string& name, actor::ActorCodeFactory code);
  void register_default(actor::ActorCodeFactory code);

  routing::NetZoneImpl* netzone_root_ = nullptr;
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
