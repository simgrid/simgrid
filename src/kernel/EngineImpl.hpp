/* Copyright (c) 2016. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u/forward.hpp>
#include <xbt/dict.h>

namespace simgrid {
namespace kernel {
namespace routing {
class AsImpl;
}

class EngineImpl {
public:
  EngineImpl();
  virtual ~EngineImpl();
  kernel::routing::AsImpl* rootAs_ = nullptr;

protected:
  friend simgrid::s4u::Engine;
};
}
}
