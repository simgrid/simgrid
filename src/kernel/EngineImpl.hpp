/* Copyright (c) 2016-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u/NetZone.hpp>

#include <map>
#include <string>
#include <unordered_map>

namespace simgrid {
namespace kernel {

class EngineImpl {
public:
  EngineImpl();
  virtual ~EngineImpl();
  kernel::routing::NetZoneImpl* netzone_root_ = nullptr;

private:
  std::map<std::string, simgrid::s4u::Host*> hosts_;
  std::map<std::string, simgrid::s4u::Link*> links_;
  std::map<std::string, simgrid::s4u::Storage*> storages_;
  std::unordered_map<std::string, simgrid::kernel::routing::NetPoint*> netpoints_;
  friend simgrid::s4u::Engine;
};
}
}
