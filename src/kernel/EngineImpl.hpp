/* Copyright (c) 2016-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <map>
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
  friend s4u::Engine;

public:
  EngineImpl() = default;

  EngineImpl(const EngineImpl&) = delete;
  EngineImpl& operator=(const EngineImpl&) = delete;
  virtual ~EngineImpl();

  void load_deployment(const std::string& file);
  void register_function(const std::string& name, int (*code)(int, char**)); // deprecated
  void register_function(const std::string& name, xbt_main_func_t code);
  void register_function(const std::string& name, void (*code)(std::vector<std::string>));
  void register_default(int (*code)(int, char**)); // deprecated
  void register_default(xbt_main_func_t code);

  routing::NetZoneImpl* netzone_root_ = nullptr;
};

} // namespace kernel
} // namespace simgrid
