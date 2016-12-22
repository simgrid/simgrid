/* Copyright (c) 2016. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u/forward.hpp>
#include <xbt/dict.h>

#include <unordered_map>

namespace simgrid {
namespace kernel {
namespace routing {
class NetZoneImpl;
class NetCard;
}

class EngineImpl {
public:
  EngineImpl();
  virtual ~EngineImpl();
  kernel::routing::NetZoneImpl* netRoot_ = nullptr;

protected:
  std::unordered_map<std::string,simgrid::kernel::routing::NetCard*> netcards_;
  friend simgrid::s4u::Engine;
};
}
}
