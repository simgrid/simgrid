/* Copyright (c) 2016-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u/NetZone.hpp>
#include <simgrid/s4u/forward.hpp>
#include <string>
#include <unordered_map>

namespace simgrid {
namespace kernel {

class EngineImpl {
public:
  EngineImpl();
  virtual ~EngineImpl();
  kernel::routing::NetZoneImpl* netRoot_ = nullptr;

private:
  std::unordered_map<std::string, simgrid::kernel::routing::NetPoint*> netpoints_;
  friend simgrid::s4u::Engine;
};
}
}
